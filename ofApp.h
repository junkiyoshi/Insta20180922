#pragma once

#include <Windows.h>
#include <NuiApi.h>
#include <opencv2/opencv.hpp>
#include "ofMain.h"

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key) {}
	void keyReleased(int key) {}
	void mouseMoved(int x, int y) {}
	void mouseDragged(int x, int y, int button) {}
	void mousePressed(int x, int y, int button) {}
	void mouseReleased(int x, int y, int button) {}
	void mouseEntered(int x, int y) {}
	void mouseExited(int x, int y) {}
	void windowResized(int w, int h) {}
	void dragEvent(ofDragInfo dragInfo) {}
	void gotMessage(ofMessage msg) {}

	// Kinect
	INuiSensor* sensor;
	INuiCoordinateMapper* cordinate_mapper;
	HANDLE color_event;
	HANDLE color_handle;
	HANDLE depth_event;
	HANDLE depth_handle;
	HANDLE events[2];
	int width, height;

	// OpenCV
	cv::Mat frame;
	cv::Mat depth;

	// openFrameworks
	ofImage frame_img;
	ofImage depth_img;
	ofFbo fbo;
	ofShader shader;
	ofTrueTypeFont font;
};