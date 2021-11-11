#version 450
#extension GL_GOOGLE_include_directive : enable

#include "shaderdefs.h"


layout(location = 0) out vec4 outColor;

void main() {
    //const vec2 uv = (gl_FragCoord.xy/u_resolution)*2.0-1.0;
    const vec2 uv = vec2(gl_FragCoord.xy) / Res;
    outColor = vec4( uv, 0.0, 1.0 );
}