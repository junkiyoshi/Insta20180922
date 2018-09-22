#version 150

uniform float time;
uniform vec2 resolution;
uniform sampler2DRect tex1;
uniform sampler2DRect tex2;
uniform sampler2DRect depth;

out vec4 outputColor;

void main() {

  vec2 p = (gl_FragCoord.xy * 2.0 - resolution) / min(resolution.x, resolution.y);

  vec4 t1 = texture(tex1, vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y));
  vec4 t2 = texture(tex2, vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y));

  float r = texture(depth, vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y)).r;
  float g = texture(depth, vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y)).g;
  float d = r * 255.0 + g;
  d /= 65,536;

  vec4 color = d < 0.1 ? t2 + t1 : vec4(1.0);
  gl_FragColor = color;
}
