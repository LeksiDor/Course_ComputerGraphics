#version 450

layout(binding = 0) uniform UniformBufferObject {
    vec2 resolution; // Resolution of the screen.
    vec2 mouse; // Mouse coordinates.
    float time; // Time since startup, in seconds.
} uniforms;

vec2 u_resolution = uniforms.resolution;
vec2 u_mouse = uniforms.mouse;
float u_time = uniforms.time;


layout(location = 0) out vec4 outColor;



void main() {
    const vec2 uv = vec2(gl_FragCoord.xy) / u_resolution;
    outColor = vec4( uv, 0.0, 1.0 );
}