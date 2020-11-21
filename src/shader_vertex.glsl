#version 330 core

layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;

#define SPHERE 0
uniform int object_id;
uniform vec4 bbox_min;
uniform vec4 bbox_max;
uniform sampler2D TextureImage0;
out vec3 color_v;
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923


void main()
{

 
    gl_Position = projection * view * model * model_coefficients;

    position_world = model * model_coefficients;

    position_model = model_coefficients;

    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    texcoords = texture_coefficients;


    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    vec4 p = position_world;

    vec4 n = normalize(normal);

    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    vec4 v = normalize(camera_position - p);

    vec4 r = -l + 2*n*(dot(n,l));

    vec4 h = normalize(v+l);

    float U = 0.0;
    float V = 0.0;

    if ( object_id == SPHERE )
    {
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        vec4 pl = bbox_center + normalize(position_model-bbox_center);
        vec4 pv = pl - bbox_center;

        U = ((atan(pv[0],pv[2]))+M_PI)/(2*M_PI);
        V = (asin(pv[1])+M_PI_2)/M_PI;
    }

    float lambert = max(0,dot(n,l));
    vec3 Ka = vec3(0.04,0.04,0.04);
    vec3 Ks = vec3(0.8,0.8,0.8);
    vec3 I = vec3(0.2,0.2,0.2);
    vec3 Ia = vec3(0.9,0.7,0.7);
    float q = 30;
    float ql = 5;

    vec3 Kd0;
    Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    color_v =  Kd0 * Ia * (lambert + 0.01);
}