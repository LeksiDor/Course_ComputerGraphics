#version 450
#extension GL_GOOGLE_include_directive : enable

#include "shaderdefs.h"

layout(binding = 0) uniform UniformBufferObject { UniformsStruct uniforms; };

const vec2 u_resolution = uniforms.resolution;
const vec2 u_mouse = uniforms.mouse;
const float u_time = uniforms.time;


layout(location = 0) out vec4 outColor;



void main() {
    const vec2 uv = vec2(gl_FragCoord.xy) / u_resolution;
    outColor = vec4( uv, 0.0, 1.0 );
}