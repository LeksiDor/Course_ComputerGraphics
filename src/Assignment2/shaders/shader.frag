#version 450

layout ( binding = 0 ) uniform UniformBufferObject {
    mat4 lookAt; // LookAt matrix.
    vec2 resolution; // Resolution of the screen.
    vec2 mouse; // Mouse coordinates.
    float time; // Time since startup, in seconds.
    float gamma; // Gamma correction parameter.
    int shadow; // 0 = none, 1 = sharp, 2 = soft.
} uniforms;

layout( binding = 1 ) uniform sampler2D texSampler;


layout(location = 0) out vec4 outColor;



// Tampere University
// COMP.CE.430 Computer Graphics Coding Assignment 2 (2021)
//
//   Oleksii Doronin, H272353
//
// Name of the functionality      |Done| Notes
//-------------------------------------------------------------------------------
// example functionality          | X | Example note: control this with var YYYY
// Mandatory functionalities ----------------------------------------------------
//   Perspective projection       | X |
//   Phong shading                | X | See function PhongColor().
//   Camera movement and rotation | X | See how uniforms.lookAt is set in main.cpp.
//   Sharp shadows                | X | See uniforms.shadow.
// Extra functionalities --------------------------------------------------------
//   Tone mapping                 | X | See uniforms.gamma.
//   PBR shading                  | X | See function BlinnPhongColor().
//   Soft shadows                 | X | See uniforms.shadow.
//   Sharp reflections            | X | See material parameter reflectivity.
//   Glossy reflections           | X | See material parameter reflectionGlossiness.
//   Refractions                  | X | See material parameter refractivity.
//   Caustics                     |   |
//   SDF Ambient Occlusions       |   |
//   Texturing                    | X | See function check_flag.
//   Simple game                  | X | Camera positioning using keys. Check console output for more info.
//   Progressive path tracing     |   |
//   Basic post-processing        |   |
//   Advanced post-processing     |   |
//   Screen space reflections     |   |
//   Screen space AO              |   |
//   Simple own SDF               | X | See function check_refractor.
//   Advanced own SDF             | X | See function check_fractal.
//   Animated SDF                 | X | See function check_blob, check_flag.
//   Other?                       | X | Custom C++ wrapping.



#define PI 3.14159265359
#define EPSILON 0.00001

// These definitions are tweakable.

/* Minimum distance a ray must travel. Raising this value yields some performance
 * benefits for secondary rays at the cost of weird artefacts around object
 * edges.
 */
#define MIN_DIST 0.08
/* Maximum distance a ray can travel. Changing it has little to no performance
 * benefit for indoor scenes, but useful when there is nothing for the ray
 * to intersect with (such as the sky in outdoors scenes).
 */
#define MAX_DIST 20.0
/* Maximum number of steps the ray can march. High values make the image more
 * correct around object edges at the cost of performance, lower values cause
 * weird black hole-ish bending artefacts but is faster.
 */
#define MARCH_MAX_STEPS 128
/* Typically, this doesn't have to be changed. Lower values cause worse
 * performance, but make the tracing stabler around slightly incorrect distance
 * functions.
 * The current value merely helps with rounding errors.
 */
#define STEP_RATIO 0.999
/* Determines what distance is considered close enough to count as an
 * intersection. Lower values are more correct but require more steps to reach
 * the surface
 */
#define HIT_RATIO 0.001


struct material
{
    vec3 diffuse;
    vec3 specular;
    float specularPower;
    float reflectivity; // 0 for no reflection, 1 for full reflection.
    float reflectionGlossiness; // Exponential distribution parameter. Values from [0,Inf).
    int textureIdx; // Index of texture. Right now, negative for no texture, and 0 for THE texture.
    vec2 texCoord; // Textural coordinates from [0,1]. Should not be in material for a real program.
    float refractivity; // 0 for no refraction, 1 for full refraction.
    float refractionIndex; // Ratio of indices in Shell's law.
};

const material material_default = material( vec3(0.8), vec3(0.8), 20.0, 0.0, 0.0, -1, vec2(0), 0.0, 1.0 );

// This lamp is positioned at the hole in the roof.
// Consider it as a point light.
const vec3 lamp_pos = vec3( 0.0, 3.1, 3.0 );
const float lamp_delta_dist = 0.1; // distance to discard when tracing shadow rays.
const vec3 light_area_radius = vec3( 0.5, 0.5, 0.5 );
const int num_light_samples = 25;

const int num_glossy_samples = 25;


// Good resource for finding more building blocks for distance functions:
// https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm

/* Basic box distance field.
 *
 * Parameters:
 *  p   Point for which to evaluate the distance field
 *  b   "Radius" of the box
 *
 * Returns:
 *  Distance to the box from point p.
 */
float box(vec3 p, vec3 b)
{
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

/* Rotates point around origin along the X axis.
 *
 * Parameters:
 *  p   The point to rotate
 *  a   The angle in radians
 *
 * Returns:
 *  The rotated point.
 */
vec3 rot_x(vec3 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return vec3(
        p.x,
        c*p.y-s*p.z,
        s*p.y+c*p.z
    );
}

/* Rotates point around origin along the Y axis.
 *
 * Parameters:
 *  p   The point to rotate
 *  a   The angle in radians
 *
 * Returns:
 *  The rotated point.
 */
vec3 rot_y(vec3 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return vec3(
        c*p.x+s*p.z,
        p.y,
        -s*p.x+c*p.z
    );
}

/* Rotates point around origin along the Z axis.
 *
 * Parameters:
 *  p   The point to rotate
 *  a   The angle in radians
 *
 * Returns:
 *  The rotated point.
 */
vec3 rot_z(vec3 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return vec3(
        c*p.x-s*p.y,
        s*p.x+c*p.y,
        p.z
    );
}


// Helper function to make angle from [0,2*PI).
float regular_angle( const in float angle )
{
    return mod( angle, 2*PI );
}


/* Each object has a distance function and a material function. The distance
 * function evaluates the distance field of the object at a given point, and
 * the material function determines the surface material at a point.
 */


void check_blob( const vec3 p, inout float min_dist, inout material mat )
{
    const vec3 center = vec3( -0.5, -2.2 + abs( sin(regular_angle( uniforms.time*3.0 )) ), 2.0 );
    const vec3 p_loc = p - center;
    // Get spherical coordinates.
    const float rho = length( p_loc );
    const float h = p_loc.y / rho;
    const float theta = asin(h);
    const float phi = atan(p_loc.z/p_loc.x) + ( (p_loc.x < 0) ? PI : 0.0);

    const float t = 2.0 * uniforms.time;
    float variation = sin(regular_angle( 10.0*theta + 5.0*t )) * (1.0-pow(h,4)) * sin(10.0*phi);
    const float radius = 0.8 + variation * sin(t) * 0.07;

    const float dist = length(p_loc) - radius;
    if ( dist < min_dist )
        min_dist = dist;
    else
        return;

    mat = material_default;
    mat.diffuse = mat.specular = vec3( 1.0, 0.5, 0.3 );
}


void check_sphere( const vec3 p, inout float min_dist, inout material mat )
{
    const vec3 center = vec3( 1.5, -1.8, 4.0 );
    const float radius = 1.2;

    vec3 p_loc = p - center;

    const float dist = length(p_loc) - radius;
    if ( dist < min_dist )
        min_dist = dist;
    else
        return;

    mat = material_default;
    mat.diffuse = mat.specular = vec3( 0.1, 0.2, 0.0 );
}


void check_room( const vec3 p, inout float min_dist, inout material mat )
{
    const vec3 dimensions = vec3( 3.0, 3.0, 6.0 );
    const vec3 center = vec3( 0.0, 0.0, 0.0 );

    const float dist = max(
        -box( p - (lamp_pos-vec3(0,lamp_delta_dist,0)), light_area_radius ),
        -box( p - center, dimensions )
    );

    if ( dist < min_dist )
        min_dist = dist;
    else
        return;

    mat = material_default;
    if ( p.x <= -2.98 )
    {
        mat.diffuse = vec3( 1.0, 0.0, 0.0 );
    }
    else if ( p.x >= 2.98 )
    {
        mat.diffuse = vec3( 0.0, 1.0, 0.0 );
    }
    else
    {
        mat.diffuse = vec3( 1.0, 1.0, 1.0 );
    }
    mat.specular = mat.diffuse;
}


void check_crate( const vec3 p, inout float min_dist, inout material mat )
{
    const vec3 center = vec3( -1, -1, 5 );
    const vec3 size = vec3( 1, 2, 1 );

    const vec3 p_loc = rot_y( p - center, regular_angle(uniforms.time) );

    const float dist = box( p_loc, size );
    if ( dist < min_dist )
        min_dist = dist;
    else
        return;

    mat = material_default;
    if ( fract( p_loc.x + floor( p_loc.y * 2.0 ) * 0.5 + floor( p_loc.z * 2.0 ) * 0.5 ) < 0.5 )
        mat.diffuse = vec3( 0.0, 1.0, 1.0 );
    else
        mat.diffuse = vec3( 1.0, 1.0, 1.0 );
    mat.specular = vec3( 0.2, 0.2, 0.7 );
}


void check_flag( const vec3 p, inout float min_dist, inout material mat )
{
    const vec3 center = vec3( -2, 2, 3 );
    const vec3 size = vec3( 1.5, 1.0, 0.1 );
    const float wave_size = 0.1;

    vec3 p_loc = p - center;
    p_loc = rot_y( p_loc, 0.25*PI );

    p_loc.z += sin(regular_angle( -10.0*uniforms.time + 5.0*p_loc.x )) * wave_size;

    const float dist = box( p_loc, size );
    if ( dist < min_dist )
        min_dist = dist;
    else
        return;

    mat = material_default;
    mat.textureIdx = 0;
    mat.texCoord = 0.5 * ( vec2(1) + vec2(1,-1)*(p_loc/size).xy );
}


void check_reflector( const vec3 p, inout float min_dist, inout material mat )
{
    const vec3 center = vec3( -2, -2, 2.5 );
    const vec3 size = vec3( 0.5, 0.8, 0.5 );

    vec3 p_loc = p - center;
    p_loc = rot_x( p_loc, -0.1 * PI );
    p_loc = rot_y( p_loc, -0.1 * PI );
    p_loc = rot_z( p_loc, -0.1 * PI );
    p_loc = rot_y( p_loc, regular_angle(-uniforms.time) );

    const float dist = box( p_loc, size );
    if ( dist < min_dist )
        min_dist = dist;
    else
        return;

    mat = material_default;
    mat.diffuse = mat.specular = vec3( 0.2, 0.2, 0.7 );
    mat.reflectivity = 0.7;
    if ( abs(p_loc.x/size.x) > abs(p_loc.z/size.z) )
        mat.reflectionGlossiness = 20.0;
}


void check_refractor( const vec3 p, inout float min_dist, inout material mat )
{
    const vec3 center = vec3( 0, 0, 1 );
    const vec3 size = vec3( 1.5, 1.0, 0.1 );
    const float wave_size = 0.1;

    vec3 p_loc = p - center;
    //p_loc = rot_y( p_loc, 0.25 * PI );

    p_loc.z += sin( regular_angle( -10.0 * uniforms.time + 5.0 * p_loc.x ) ) * wave_size;

    const float dist = box( p_loc, size );
    if ( dist < min_dist )
        min_dist = dist;
    else
        return;

    mat = material_default;
    mat.refractivity = 0.9;
    mat.refractionIndex = 1.2;
    //mat.textureIdx = 0;
    //mat.texCoord = 0.5 * (vec2( 1 ) + vec2( 1, -1 ) * (p_loc / size).xy);
}


void check_fractal( const vec3 p, inout float min_dist, inout material mat )
{
    const vec3 center = vec3( 2, -2, 2 );
    const float sphere_radius = 0.2;
    const vec3 size = vec3( 0.5 );

    vec3 p_loc = (p - center) / size;

    const float Scale = 2.0;
    const int MaxIterations = 8;
    const int Iterations = 1 + int(2.0*uniforms.time) % MaxIterations;

    vec3 a1 = vec3( 1, 1, 1 );
    vec3 a2 = vec3( -1, -1, 1 );
    vec3 a3 = vec3( 1, -1, -1 );
    vec3 a4 = vec3( -1, 1, -1 );
    vec3 c;
    int n = 0;
    float dist, d;
    while ( n < Iterations )
    {
        c = a1; dist = length( p_loc - a1 );
        d = length( p_loc - a2 ); if ( d < dist ) { c = a2; dist = d; }
        d = length( p_loc - a3 ); if ( d < dist ) { c = a3; dist = d; }
        d = length( p_loc - a4 ); if ( d < dist ) { c = a4; dist = d; }
        p_loc = Scale*p_loc - c*(Scale - 1.0);
        n++;
    }

    p_loc *= pow( Scale, float(-n) );
    dist = length( p_loc ) - sphere_radius;

    if ( dist < min_dist )
        min_dist = dist;
    else
        return;

    mat = material_default;
    mat.diffuse = mat.specular = vec3( 0.2, 0.2, 0.7 );
    mat.reflectivity = 0.3;
}

/* The distance function collecting all others.
 *
 * Parameters:
 *  p   The point for which to find the nearest surface
 *  mat The material of the nearest surface
 *
 * Returns:
 *  The distance to the nearest surface.
 */
float map(
    in vec3 p,
    out material mat
){
    float min_dist = MAX_DIST*2.0;

    check_blob( p, min_dist, mat );
    check_room( p, min_dist, mat );
    check_crate( p, min_dist, mat );
    check_sphere( p, min_dist, mat );

    // Add your own objects here!
    check_reflector( p, min_dist, mat );
    check_refractor( p, min_dist, mat );
    check_fractal( p, min_dist, mat );
    check_flag( p, min_dist, mat );

    return min_dist;
}

/* Calculates the normal of the surface closest to point p.
 *
 * Parameters:
 *  p   The point where the normal should be calculated
 *  mat The material information, produced as a byproduct
 *
 * Returns:
 *  The normal of the surface.
 *
 * See https://www.iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
 * if you're interested in how this works.
 */
vec3 normal(vec3 p, out material mat)
{
    const vec2 k = vec2(1.0, -1.0);
    return normalize(
        k.xyy * map(p + k.xyy * EPSILON, mat) +
        k.yyx * map(p + k.yyx * EPSILON, mat) +
        k.yxy * map(p + k.yxy * EPSILON, mat) +
        k.xxx * map(p + k.xxx * EPSILON, mat)
    );
}

/* Finds the closest intersection of the ray with the scene.
 *
 * Parameters:
 *  o           Origin of the ray
 *  v           Direction of the ray
 *  max_dist    Maximum distance the ray can travel. Usually MAX_DIST.
 *  p           Location of the intersection
 *  n           Normal of the surface at the intersection point
 *  mat         Material of the intersected surface
 *  inside      Whether we are marching inside an object or not. Useful for
 *              refractions.
 *
 * Returns:
 *  true if a surface was hit, false otherwise.
 */
bool intersect(
    in vec3 o,
    in vec3 v,
    in float max_dist,
    out vec3 p,
    out vec3 n,
    out material mat,
    bool inside
) {
    float t = MIN_DIST;
    float dir = inside ? -1.0 : 1.0;
    bool hit = false;

    for(int i = 0; i < MARCH_MAX_STEPS; ++i)
    {
        p = o + t * v;
        float dist = dir * map(p, mat);

        hit = abs(dist) < HIT_RATIO * t;

        if(hit || t > max_dist) break;

        t += dist * STEP_RATIO;
    }

    n = normal(p, mat);

    return hit;
}


float GetShadowing( const in vec3 position )
{
    // Dummy parameters.
    vec3 p_dummy, n_dummy;
    material mat;

    if ( uniforms.shadow == 0 )
    {
        // No shadow.
        return 1.0;
    }
    else if ( uniforms.shadow == 1 )
    {
        // Sharp shadow.
        const float distToLight = length( lamp_pos - position );
        const vec3 dirToLight = (lamp_pos - position) / distToLight;
        if ( intersect( position + EPSILON*dirToLight, dirToLight - 2.0*EPSILON - lamp_delta_dist, distToLight, p_dummy, n_dummy, mat, false ) )
            return 0.2;
        else
            return 1.0;
    }
    else if ( uniforms.shadow == 2 )
    {
        // Soft shadow.
        const int sqrt_num_light_samples = int( sqrt( num_light_samples ) );
        float shadowing = 0.0;
        for ( int ix = 0; ix < sqrt_num_light_samples; ++ix )
        {
            for ( int iy = 0; iy < sqrt_num_light_samples; ++iy )
            {
                vec3 lambda = ( vec3(ix,0,iy) + vec3(0.5) ) / vec3( sqrt_num_light_samples );
                lambda = 2.0*lambda - vec3(1);
                const vec3 lightPos = lamp_pos + lambda*light_area_radius;
                const float distToLight = length( lightPos - position );
                const vec3 dirToLight = (lightPos - position) / distToLight;
                if ( intersect( position + EPSILON*dirToLight, dirToLight - 2.0*EPSILON - lamp_delta_dist, distToLight, p_dummy, n_dummy, mat, false ) )
                    shadowing += 0.2;
                else
                    shadowing += 1.0;
            }
        }
        shadowing /= float( sqrt_num_light_samples * sqrt_num_light_samples );
        return shadowing;
    }
    else
    {
        // Wrong value.
        return 0.5;
    }
}


vec3 PhongColor( const in vec3 position, const in vec3 norm, const in material mat, const in vec3 lightPos, const in vec3 rayDir )
{
    const vec3 lightDir = normalize( lightPos - position );
    const float dotLight = dot( lightDir, norm );
    const vec3 viewDir = normalize( -rayDir );
    const vec3 reflDir = 2.0 * dotLight * norm - lightDir;
    const float specularArg = clamp( dot( viewDir, reflDir ), 0, 1 );
    vec3 color = vec3(0);
    if ( mat.textureIdx == 0 )
        color += dotLight * texture( texSampler, mat.texCoord ).xyz;
    else
        color += dotLight * mat.diffuse;
    color += mat.specular * pow( specularArg, mat.specularPower );
    return color;
}

vec3 BlinnPhongColor( const in vec3 position, const in vec3 norm, const in material mat, const in vec3 lightPos, const in vec3 rayDir )
{
    const vec3 lightDir = normalize( lightPos - position );
    const float dotLight = dot( lightDir, norm );
    const vec3 viewDir = normalize( -rayDir );
    const vec3 halfViewLightDir = normalize( lightDir + viewDir );
    const vec3 reflDir = 2.0 * dotLight * norm - lightDir;
    const float specularArg = clamp( dot( norm, halfViewLightDir ), 0, 1 );
    vec3 color = vec3(0);
    if ( mat.textureIdx == 0 )
        color += dotLight * texture( texSampler, mat.texCoord ).xyz;
    else
        color += dotLight * mat.diffuse;
    color += mat.specular * pow( specularArg, mat.specularPower );
    return color;
}


vec3 GetSurfaceColor( const in vec3 position, const in vec3 norm, const in material mat, const in vec3 rayDir )
{
    //const vec3 color_base = PhongColor( position, norm, mat, lamp_pos, rayDir );
    const vec3 color_base = BlinnPhongColor( position, norm, mat, lamp_pos, rayDir );
    const float shadowing = GetShadowing( position );
    return shadowing * color_base;
}


/* Calculates the color of the pixel, based on view ray origin and direction.
 *
 * Parameters:
 *  o   Origin of the view ray
 *  v   Direction of the view ray
 *
 * Returns:
 *  Color of the pixel.
 */
vec3 render( vec3 rayOri, vec3 rayDir )
{
    material mat;

    vec3 color = vec3(0);

    // Compute intersection point along the view ray.
    vec3 p, n;
    if ( intersect( rayOri, rayDir, MAX_DIST, p, n, mat, false ) )
    {
        color = GetSurfaceColor( p, n, mat, rayDir );

        // Reflection.
        vec3 color_refl = vec3(0);
        if ( mat.reflectivity > 0 )
        {
            vec3 p_refl, n_refl;
            material mat_refl;
            const vec3 rayDir_refl = normalize( rayDir + 2.0*n );
            if ( mat.reflectionGlossiness <= 0.0 )
            {
                // Sharp reflection: one ray.
                if ( intersect( p + EPSILON*n, rayDir_refl, MAX_DIST, p_refl, n_refl, mat_refl, false ) )
                    color_refl += GetSurfaceColor( p_refl, n_refl, mat_refl, rayDir_refl );
            }
            else
            {
                // Get local surface tangent directions.
                const vec3 dir1 = normalize( cross( rayDir_refl, n ) );
                const vec3 dir2 = normalize( cross( rayDir_refl, dir1 ) );
                // Glossy reflection: many rays, exponential distribution.
                const int sqrt_num_samples = int( sqrt( num_glossy_samples ) );
                float exp_lambda = mat.reflectionGlossiness;
                vec3 p_refl, n_refl;
                for ( int ix = 0; ix < sqrt_num_samples; ++ix )
                {
                    for ( int iy = 0; iy < sqrt_num_samples; ++iy )
                    {
                        vec2 t = ( vec2(ix,iy) + vec2(0.5) ) / float(sqrt_num_samples);
                        t = 2.0*t - vec2(1.0);
                        //const vec2 rot_angles = 0.5*PI*t;
                        const vec2 rot_angles = 0.5*PI * sign(t)*log(abs(t))/exp_lambda;
                        vec3 d = vec3(0);
                        d += cos(t.x)*rayDir_refl + sin(t.x)*dir1;
                        d += cos(t.y)*rayDir_refl + sin(t.y)*dir2;
                        d = normalize(d);
                        if ( intersect( p + EPSILON*n, d, MAX_DIST, p_refl, n_refl, mat_refl, false ) )
                            color_refl += GetSurfaceColor( p_refl, n_refl, mat_refl, rayDir_refl );
                    }
                }
                color_refl /= float( sqrt_num_samples * sqrt_num_samples );
            }
        }
        color = (1.0-mat.reflectivity)*color + mat.reflectivity*color_refl;

        // Refraction.
        vec3 color_refr = vec3(0);
        if ( mat.refractivity > 0.0 )
        {
            vec3 p_refr, n_refr;
            material mat_refr;
            const float cosIn = dot( rayDir, n );
            const float sinIn = sqrt( 1.0 - cosIn*cosIn );
            const float sinOut = sinIn * mat.refractionIndex;
            const float cosOut = sqrt( 1.0 - sinOut*sinOut );
            const vec3 tanDir = normalize( rayDir - n*cosIn );
            const vec3 rayDir_refr = -n*cosOut + tanDir*sinOut;
            // Dirty hack: skip one internal intersection.
            intersect( p + EPSILON*rayDir_refr, rayDir_refr, MAX_DIST, p_refr, n_refr, mat_refr, true );
            if ( intersect( p_refr + EPSILON*rayDir_refr, rayDir_refr, MAX_DIST, p_refr, n_refr, mat_refr, false ) )
                color_refr += GetSurfaceColor( p_refr, n_refr, mat_refr, rayDir_refr );
        }
        color = (1.0-mat.refractivity)*color + mat.refractivity*color_refr;
    }

    // Gamma correction (a.k.a. tone mapping, in simplest form).
    color = pow( color, vec3(uniforms.gamma) );

    return color;
}


void main()
{
    const bool isCameraPerspective = true;

    // This is the position of the pixel in normalized device coordinates.
    vec2 uv = (gl_FragCoord.xy / uniforms.resolution) * 2.0 - 1.0;
    uv.y = -uv.y;
    // Calculate aspect ratio
    const float aspect = uniforms.resolution.x / uniforms.resolution.y;

    const vec3 rayOri    = ( uniforms.lookAt * vec4( 0, 0, 0, 1 ) ).xyz;
    const vec3 rayTarget = ( uniforms.lookAt * vec4( uv.x * aspect, uv.y, 1.0, 1.0 ) ).xyz;
    const vec3 rayDir    = normalize( rayTarget - rayOri );

    outColor = vec4( render( rayOri, rayDir ), 1.0 );
}