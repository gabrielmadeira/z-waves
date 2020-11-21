#version 330 core

in vec4 position_world;
in vec4 normal;

in vec4 position_model;

in vec2 texcoords;
in vec3 color_v;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define ZOMBIE  3
#define PCUBE  4
#define GUN  5
#define ZCUBE  6
#define MAP  7
#define TREE  8
#define COW  9
#define GRASS  10
#define STONE  11
#define STONE2  12
#define TREE2  13
uniform int object_id;

uniform vec4 bbox_min;
uniform vec4 bbox_max;

uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform sampler2D TextureImage6;
uniform sampler2D TextureImage7;
uniform sampler2D TextureImage8;
uniform sampler2D TextureImage9;
uniform sampler2D TextureImage10;
uniform sampler2D TextureImage11;
uniform sampler2D TextureImage12;


out vec3 color;

#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{

    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    vec4 p = position_world;

    vec4 n = normalize(normal);

    vec4 l = normalize(vec4(1.0,1.0,1.0,0.0));

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
    else if ( object_id == BUNNY )
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x-minx)/(maxx-minx);
        V = (position_model.y-miny)/(maxy-miny);
    }
    else if (( object_id == PLANE ) || ( object_id == ZOMBIE ) || ( object_id == GUN ) || ( object_id == ZCUBE ) || ( object_id == MAP ) || ( object_id == TREE ) 
    || ( object_id == COW ) || ( object_id == GRASS ) || ( object_id == STONE ) || ( object_id == STONE2 ) || ( object_id == TREE2 ))
    {
        U = texcoords.x;
        V = texcoords.y;
    }


    float lambert = max(0,dot(n,l));

    vec3 Ka = vec3(0.0015,0.00009,0.00009);

    vec3 Ks = vec3(0.8,0.8,0.8);

    vec3 I = vec3(0.2,0.2,0.2);

    vec3 Ia = vec3(1.3,1.0,1.0);

    float q = 30;
    float ql = 5;


    vec3 Kd0;
    
    if ( object_id == SPHERE ){
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia ;
    }else if ( object_id == GUN ){
        Kd0 = texture(TextureImage4, vec2(U,V)).rgb;
        Ks = vec3(4.8,4.8,4.8);
        color =  Kd0 * Ia * (lambert + 2.01)  + Ka*Ia  + Ks*I*pow(dot(n,h),ql);
    }else if ( object_id == ZCUBE ){
        Kd0 = texture(TextureImage5, vec2(U,V)).rgb;
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia ;
    }else if ( object_id == MAP ){
        Kd0 = texture(TextureImage6, vec2(U,V)).rgb;
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia ;
    }else if ( object_id == TREE ){
        Kd0 = texture(TextureImage7, vec2(U,V)).rgb;
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia ;
    }else if ( object_id == COW ){
        Kd0 = texture(TextureImage8, vec2(U,V)).rgb;
        Ks = vec3(4.8,4.8,4.8);
        color =  Kd0 * Ia * (lambert + 5.01)  + Ka*Ia  + Ks*I*pow(dot(n,h),ql);
    }else if ( object_id == GRASS ){
        Kd0 = texture(TextureImage9, vec2(U,V)).rgb;
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia ;
    }else if ( object_id == STONE ){
        Kd0 = texture(TextureImage10, vec2(U,V)).rgb;
        Ks = vec3(4.8,4.8,4.8);
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia  + Ks*I*pow(dot(n,h),ql);
    }else if ( object_id == STONE2 ){
        Kd0 = texture(TextureImage11, vec2(U,V)).rgb;
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia ;
    }else if ( object_id == TREE2 ){
        Kd0 = texture(TextureImage12, vec2(U,V)).rgb;
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia ;
    }else{
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
        color =  Kd0 * Ia * (lambert + 0.01)  + Ka*Ia ;
    }

    if ( object_id == SPHERE ){
        color = pow(color_v, vec3(1.0,1.0,1.0)/2.0);
    }else{
        color = pow(color, vec3(1.0,1.0,1.0)/2.0);
    }

    
}

