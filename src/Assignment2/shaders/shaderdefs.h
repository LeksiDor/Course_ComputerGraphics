#ifndef SHADER_SHADERDEFS_H
#define SHADER_SHADERDEFS_H

#ifdef __cplusplus
#include <glm/glm.hpp>
namespace shader {
using namespace glm;
#endif // __cplusplus



struct UniformsStruct
{
    vec2 resolution; // Resolution of the screen.
    vec2 mouse; // Mouse coordinates.
    float time; // Time since startup, in seconds.
};


#ifdef __cplusplus
} // namespace shader
#endif // __cplusplus



#endif // SHADER_SHADERDEFS_H