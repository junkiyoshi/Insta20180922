#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetFrameRate(30);
	ofSetWindowTitle("InstaKinect");

	// Create Instance
	this->sensor = nullptr;
	HRESULT result = S_OK;
	result = NuiCreateSensorByIndex(0, &sensor);
	if (FAILED(result)) {
		cout << "Error : Create Sensor" << endl;
		return;
	}

	// Initialize
	result = sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH);
	if (FAILED(result)) {
		cout << "Error : Initialise COLOR" << endl;
		return;
	}

	// Color
	this->color_event = INVALID_HANDLE_VALUE;
	this->color_handle = INVALID_HANDLE_VALUE;
	color_event = CreateEventW(nullptr, true, false, nullptr);
	result = sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, color_event, &color_handle);
	if (FAILED(result)) {
		cout << "Error : Color Stream Open" << endl;
		return;
	}

	// Depth
	this->depth_event = INVALID_HANDLE_VALUE;
	this->depth_handle = INVALID_HANDLE_VALUE;
	depth_event = CreateEventW(nullptr, true, false, nullptr);
	result = sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH, NUI_IMAGE_RESOLUTION_640x480, 0, 2, depth_event, &depth_handle);
	if (FAILED(result)) {
		cout << "Error : Depth Stream Open" << endl;
		return;
	}

	// Size
	unsigned long refWidth = 0;
	unsigned long refHeight = 0;
	NuiImageResolutionToSize(NUI_IMAGE_RESOLUTION_640x480, refWidth, refHeight);
	this->width = static_cast<int>(refWidth);
	this->height = static_cast<int>(refHeight);

	// Mapp
	this->cordinate_mapper = nullptr;
	result = sensor->NuiGetCoordinateMapper(&this->cordinate_mapper);
	if (FAILED(result)) {
		cout << "Error : Cordinate Mapper " << endl;
		return;
	}

	// OpenCV
	this->frame_img.allocate(this->width, this->height, OF_IMAGE_COLOR);
	this->frame = cv::Mat(this->frame_img.getHeight(), this->frame_img.getWidth(), CV_MAKETYPE(CV_8UC3, this->frame_img.getPixels().getNumChannels()), this->frame_img.getPixels().getData(), 0);
	this->depth_img.allocate(this->width, this->height, OF_IMAGE_COLOR);
	this->depth = cv::Mat(this->depth_img.getHeight(), this->depth_img.getWidth(), CV_MAKETYPE(CV_8UC3, this->depth_img.getPixels().getNumChannels()), this->depth_img.getPixels().getData(), 0);

	this->events[0] = this->color_event;
	this->events[1] = this->depth_event;

	string moji = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	this->font.loadFont("fonts/Kazesawa-Bold.ttf", 13);

	this->fbo.allocate(ofGetWidth(), ofGetHeight());
	this->fbo.begin();
	ofClear(0);

	ofBackground(255);
	ofSetColor(0);

	for (int x = 0; x < ofGetWidth(); x += 10) {

		for (int y = 0; y < ofGetHeight(); y += 10) {

			this->font.drawString(moji.substr((int)ofRandom(26), 1), x, y);
		}
	}

	this->fbo.end();
	this->shader.load("", "shader/shader.frag");
}

//--------------------------------------------------------------
void ofApp::update() {

	HRESULT result = S_OK;

	std::vector<NUI_COLOR_IMAGE_POINT> color_point(width * height);

	// Wait frame update.
	ResetEvent(this->color_event);
	ResetEvent(this->depth_event);
	WaitForMultipleObjects(ARRAYSIZE(this->events), this->events, true, INFINITE);

	// Get frame from color camera.
	NUI_IMAGE_FRAME colorImageFrame = { 0 };
	result = sensor->NuiImageStreamGetNextFrame(this->color_handle, 0, &colorImageFrame);
	if (FAILED(result)) {
		std::cerr << "Error : NuiImageStreamGetNextFrame( COLOR )" << std::endl;
		return;
	}

	// Get image data.
	INuiFrameTexture* pColorFrameTexture = colorImageFrame.pFrameTexture;
	NUI_LOCKED_RECT colorLockedRect;
	pColorFrameTexture->LockRect(0, &colorLockedRect, nullptr, 0);

	//// Get frame from depth camera.
	NUI_IMAGE_FRAME depthImageFrame = { 0 };
	result = this->sensor->NuiImageStreamGetNextFrame(this->depth_handle, 0, &depthImageFrame);
	if (FAILED(result)) {
		std::cerr << "Error : NuiImageStreamGetNextFrame( DEPTH )" << std::endl;
		return;
	}

	// Get depth data.
	BOOL nearMode = false;
	INuiFrameTexture* pDepthFrameTexture = nullptr;
	this->sensor->NuiImageFrameGetDepthImagePixelFrameTexture(this->depth_handle, &depthImageFrame, &nearMode, &pDepthFrameTexture);
	NUI_LOCKED_RECT depthLockedRect;
	pDepthFrameTexture->LockRect(0, &depthLockedRect, nullptr, 0);

	// OpenCV(image)
	cv::Mat src = cv::Mat(this->height, this->width, CV_8UC4, reinterpret_cast<unsigned char*>(colorLockedRect.pBits));
	cv::cvtColor(src, this->frame, CV_BGRA2RGB);

	// OpenCV(depth)
	cv::Mat bufferMat = cv::Mat::zeros(height, width, CV_8UC3);
	NUI_DEPTH_IMAGE_PIXEL* depth_pixel = reinterpret_cast<NUI_DEPTH_IMAGE_PIXEL*>(depthLockedRect.pBits);
	this->cordinate_mapper->MapDepthFrameToColorFrame(NUI_IMAGE_RESOLUTION_640x480, width * height, depth_pixel, NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, width * height, &color_point[0]);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			unsigned int index = y * width + x;
			bufferMat.at<cv::Vec3b>(color_point[index].y, color_point[index].x)[0] = depth_pixel[index].depth / 255;
			bufferMat.at<cv::Vec3b>(color_point[index].y, color_point[index].x)[1] = depth_pixel[index].depth - (int)(depth_pixel[index].depth / 255) * 255;
		}
	}
	bufferMat.copyTo(this->depth);
	
	// Release
	pColorFrameTexture->UnlockRect(0);
	pDepthFrameTexture->UnlockRect(0);
	this->sensor->NuiImageStreamReleaseFrame(this->color_handle, &colorImageFrame);
	this->sensor->NuiImageStreamReleaseFrame(this->depth_handle, &depthImageFrame);

	// ofImage Update
	this->frame_img.update();
	this->depth_img.update();
}

//--------------------------------------------------------------
void ofApp::draw() {
	
	ofSetColor(255);

	this->shader.begin();
	this->shader.setUniform1f("time", ofGetElapsedTimef());
	this->shader.setUniform2f("resolution", ofGetWidth(), ofGetHeight());
	this->shader.setUniformTexture("tex1", this->frame_img, 1);
	this->shader.setUniformTexture("tex2", this->fbo.getTexture(), 2);
	this->shader.setUniformTexture("depth", this->depth_img, 3);

	ofRect(0, 0, ofGetWidth(), ofGetHeight());

	this->shader.end();
}

//========================================================================
int main() {
	ofSetupOpenGL(640, 480, OF_WINDOW);
	ofRunApp(new ofApp());
}