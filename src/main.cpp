#include <cmath>
#include <cstdio>
#include <cstdlib>


#include <map>
#include <stack>
#include <string>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <random>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "utils.h"
#include "matrices.h"


struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};

struct SceneObject
{
    std::string  name;  
    size_t       first_index;
    size_t       num_indices; 
    GLenum       rendering_mode; 
    GLuint       vertex_array_object_id; 
    glm::vec3    bbox_min; 
    glm::vec3    bbox_max;
};

void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

void BuildTrianglesAndAddToVirtualScene(ObjModel*);
void ComputeNormals(ObjModel* model);

void LoadShadersFromFiles(); 
void LoadTextureImage(const char* filename);
void DrawVirtualObject(const char* object_name); 

GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

GLuint g_NumLoadedTextures = 0;

std::map<std::string, SceneObject> g_VirtualScene;
std::stack<glm::mat4>  g_MatrixStack;

std::default_random_engine generator;

bool g_thirdPerson = false;
bool g_paused = false;
bool g_paused_aux = false;

float g_ScreenRatio = 1.0f;

float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

int wave = 1;
int score = 0;

float charSpeed = 4.5;

glm::vec4 w;
glm::vec4 u;

int playerLife = 15;

float g_DeslocX = 1.0f;
float g_DeslocY = 0.0f;
float g_DeslocZ = 1.0f;

float deltaTime;
float currentTime;
float lastTime;

float walking1 = 3.14;
float walking2 = 3.14;


class Zombie{
    public:
        float speed;
        float DeslocX, DeslocY, DeslocZ;
        Zombie(float x, float z, float s, int l);
        void calcLocation();
        bool dead;
        int life;
        void hit();
        int orientation;
        float hitCooldown = 0.5;
        float hitCooldownAux = 0;

};

Zombie::Zombie(float x,float z, float s, int l){
    speed = s;
    DeslocX = x;
    DeslocY = -1.1f;
    DeslocZ = z;
    dead = false;
    life = l;
    orientation = 0;
}

void Zombie::calcLocation(){
    
    int Xori = 0;
    int Zori = 0;

    if (DeslocX > g_DeslocX){
        DeslocX -= speed*deltaTime;
        Xori = 0;
    }else{
        DeslocX += speed*deltaTime;
        Xori = 2*2;
    }

    if (DeslocZ > g_DeslocZ){
        DeslocZ -= speed*deltaTime;
        Zori = 3*2;
    }else{
        DeslocZ += speed*deltaTime;
        Zori = 1*2;
    }
    if (abs(DeslocX - g_DeslocX) > abs(DeslocZ - g_DeslocZ)){
        orientation = Xori;
    }else{
        orientation = Zori;
    }
}

void Zombie::hit(){
    life--;
    if (life <= 0){
        score++;
        std::cout << "SCORE: " << score << "\n";
        dead = true;
    }
}

std::vector<Zombie> zombies;

class Projectile{
    public:
        float speed;
        float DeslocX, DeslocY, DeslocZ;
        float creationTime;
        glm::vec4 direction;
        Projectile(float x, float y, float z, glm::vec4 d, float s);
        void calcLocation();
        float lifeTime = 5;
        void testLifeTime();
        bool projectileDisabled = false;
        glm::vec4 p1;
        glm::vec4 p2;
        glm::vec4 p3;
        glm::vec4 p4;
        float t = 0;
};

Projectile::Projectile(float x,float y,float z, glm::vec4 d, float s){
    speed = s;
    DeslocX = x;
    DeslocY = y;
    DeslocZ = z;
    direction = normalize(d);
    creationTime = (float)glfwGetTime();

    int distance = 10*wave;

    p1 = glm::vec4(x,y,z,1.0f);
    p2 = glm::vec4(x-direction[0]*distance,y-direction[1]*distance+2,z-direction[2]*distance,1.0f);
    p3 = glm::vec4(x-direction[0]*distance*2,y-direction[1]*distance*2+1.8,z-direction[2]*distance*2,1.0f);
    p4 = glm::vec4(x-direction[0]*distance*3,y-direction[1]*distance*3-2,z-direction[2]*distance*3,1.0f);
}

void Projectile::calcLocation(){
    float currentPTime = (float)glfwGetTime();
    t = (currentPTime - creationTime)/lifeTime;

    glm::vec4 c12 = p1 + t*(p2-p1);
    glm::vec4 c23 = p2 + t*(p3-p2);
    glm::vec4 c34 = p3 + t*(p4-p3);
    glm::vec4 c123 = c12 + t*(c23-c12);
    glm::vec4 c234 = c23 + t*(c34-c23);
    glm::vec4 c = c123 + t*(c234-c123);

    DeslocX = c[0];
    DeslocY = c[1];
    DeslocZ = c[2];

}

void Projectile::testLifeTime(){
    float currentPTime = (float)glfwGetTime();
    projectileDisabled = ((currentPTime - creationTime) > lifeTime);
}
std::vector<Projectile> projectiles;

bool intersect(glm::vec3 bbox_min_a, glm::vec3 bbox_max_a, glm::vec3 bbox_min_b, glm::vec3 bbox_max_b) {
  return ((bbox_min_a[0] <= bbox_max_b[0] && bbox_max_a[0] >= bbox_min_b[0]) && 
            (bbox_min_a[1] <= bbox_max_b[1] && bbox_max_a[1] >= bbox_min_b[1]) && 
            (bbox_min_a[2] <= bbox_max_b[2] && bbox_max_a[2] >= bbox_min_b[2]));
}
bool intersectCylinderY(float rad1, glm::vec3 center1, float rad2, glm::vec3 center2) {
    center1[1] = center2[1];
    return (length(center1 - center2) < (rad1+rad2));
}

std::vector<glm::vec4> trees;
std::vector<glm::vec3> grass;
std::vector<glm::vec3> stones1;
std::vector<glm::vec3> stones2;

bool intersectTrees(glm::vec3 centerP){
    bool intersect = false;
    for (int i=0; i<trees.size(); i++){
        if (intersectCylinderY(1,glm::vec3(trees[i][0],0,trees[i][2]),1,glm::vec3(centerP[0],0,centerP[2]))){
            if (!((trees[i][0]<2) && (trees[i][0]>-2) && (trees[i][2]<2) && (trees[i][2]>-2))){
                intersect = true;
            }
        }
    }
    return intersect;
}

bool cowAvaiable;
glm::vec3 cowPosition;
float cowTime = 0;

float g_CameraTheta = 0.0f;
float g_CameraPhi = 0.0f;

int main(int argc, char* argv[])
{

    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }


    glfwSetErrorCallback(ErrorCallback);


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    window = glfwCreateWindow(800, 800, "Z-Waves", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowSize(window, 800, 800);

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    LoadShadersFromFiles();

    LoadTextureImage("../../data/tc-earth_daymap_surface.jpg");      // TextureImage0
    LoadTextureImage("../../data/tc-earth_nightmap_citylights.gif"); // TextureImage1
    LoadTextureImage("../../data/grass.jpg"); // TextureImage2
    LoadTextureImage("../../data/zombie.png"); // TextureImage3
    LoadTextureImage("../../data/gun.png"); // TextureImage4
    LoadTextureImage("../../data/zcube.png"); // TextureImage5
    LoadTextureImage("../../data/map.png"); // TextureImage6
    LoadTextureImage("../../data/tree.png"); // TextureImage7
    LoadTextureImage("../../data/cow.png"); // TextureImage8
    LoadTextureImage("../../data/grass.png"); // TextureImage9
    LoadTextureImage("../../data/stone.png"); // TextureImage10
    LoadTextureImage("../../data/stone2.png"); // TextureImage11
    LoadTextureImage("../../data/tree2.png"); // TextureImage12


    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel zombiemodel("../../data/zombie.obj");
    ComputeNormals(&zombiemodel);
    BuildTrianglesAndAddToVirtualScene(&zombiemodel);

    ObjModel bunnymodel("../../data/bunny.obj");
    ComputeNormals(&bunnymodel);
    BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel pcubemodel("../../data/pcube.obj");
    ComputeNormals(&pcubemodel);
    BuildTrianglesAndAddToVirtualScene(&pcubemodel);

    ObjModel gunmodel("../../data/gun.obj");
    ComputeNormals(&gunmodel);
    BuildTrianglesAndAddToVirtualScene(&gunmodel);

    ObjModel zcubemodel("../../data/zcube.obj");
    ComputeNormals(&zcubemodel);
    BuildTrianglesAndAddToVirtualScene(&zcubemodel);

    ObjModel mapmodel("../../data/map.obj");
    ComputeNormals(&mapmodel);
    BuildTrianglesAndAddToVirtualScene(&mapmodel);
    
    ObjModel treemodel("../../data/tree.obj");
    ComputeNormals(&treemodel);
    BuildTrianglesAndAddToVirtualScene(&treemodel);

    ObjModel cowmodel("../../data/cow.obj");
    ComputeNormals(&cowmodel);
    BuildTrianglesAndAddToVirtualScene(&cowmodel);

    ObjModel grassmodel("../../data/grass.obj");
    ComputeNormals(&grassmodel);
    BuildTrianglesAndAddToVirtualScene(&grassmodel);

    ObjModel stonemodel("../../data/stone.obj");
    ComputeNormals(&stonemodel);
    BuildTrianglesAndAddToVirtualScene(&stonemodel);

    ObjModel stone2model("../../data/stone2.obj");
    ComputeNormals(&stone2model);
    BuildTrianglesAndAddToVirtualScene(&stone2model);

    ObjModel tree2model("../../data/tree2.obj");
    ComputeNormals(&tree2model);
    BuildTrianglesAndAddToVirtualScene(&tree2model);


    glm::vec3 bbox_min_projectile_obj = g_VirtualScene["pcube"].bbox_min* 0.05f;
    glm::vec3 bbox_max_projectile_obj = g_VirtualScene["pcube"].bbox_max* 0.05f;

    glm::vec3 bbox_min_zombie_obj = g_VirtualScene["zombie"].bbox_min;
    glm::vec3 bbox_max_zombie_obj = g_VirtualScene["zombie"].bbox_max;

    glm::vec3 bbox_min_player_obj = g_VirtualScene["bunny"].bbox_min;
    glm::vec3 bbox_max_player_obj = g_VirtualScene["bunny"].bbox_max;

    glm::vec3 bbox_min_player = bbox_min_player_obj;
    glm::vec3 bbox_max_player = bbox_max_player_obj;


    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

   
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    lastTime = (float)glfwGetTime();
    float gunCooldownAux = 0;

    std::uniform_real_distribution<float> distribution_tree(-40*1.5,40*1.5);
    std::uniform_int_distribution<int> distribution_tree2(0,1);
    for (int i=0; i<100;i++){
        trees.push_back(glm::vec4(distribution_tree(generator),-1.0f,distribution_tree(generator),distribution_tree2(generator)));
    }

    for (int i=0; i<700;i++){
        grass.push_back(glm::vec3(distribution_tree(generator),-1.0f,distribution_tree(generator)));
    }

    for (int i=0; i<600;i++){
        stones1.push_back(glm::vec3(distribution_tree(generator),-1.0f,distribution_tree(generator)));
    }

    std::uniform_real_distribution<float> distribution_stone1(-35*1.5-30,-35*1.5-5);
    std::uniform_real_distribution<float> distribution_stone2(35*1.5+5,35*1.5+30);

    for (int i=0; i<20;i++){
        
        stones2.push_back(glm::vec3(distribution_tree(generator),-1.0f,distribution_stone1(generator)));
        stones2.push_back(glm::vec3(distribution_tree(generator),-1.0f,distribution_stone2(generator)));
        stones2.push_back(glm::vec3(distribution_stone1(generator),-1.0f,distribution_tree(generator)));
        stones2.push_back(glm::vec3(distribution_stone2(generator),-1.0f,distribution_tree(generator)));
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    while (!glfwWindowShouldClose(window))
    {

        if (g_paused_aux){
            if (g_paused){
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }else{
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            g_paused_aux = false;
        }

        glClearColor(0.1f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);

        float r = 8;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        glm::vec4 camera_position_c;
        glm::vec4 camera_lookat_l;
        glm::vec4 camera_view_vector;
        glm::vec4 camera_up_vector;

        if (g_thirdPerson){
            camera_position_c  = glm::vec4(x+g_DeslocX,y,z+g_DeslocZ,1.0f);
            camera_lookat_l    = glm::vec4(g_DeslocX,g_DeslocY,g_DeslocZ,1.0f);
            camera_view_vector = camera_lookat_l - camera_position_c;
            camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);
        }else{
            camera_position_c  = glm::vec4(g_DeslocX,0.0f,g_DeslocZ,1.0f);
            camera_lookat_l    = glm::vec4(0.0f+g_DeslocX,0.0f+g_DeslocY,0.0f+g_DeslocZ,1.0f);
            camera_view_vector = camera_position_c - (camera_position_c+glm::vec4(x,y,z,0.0f));;
            camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);
        }

        

        w = -camera_view_vector / norm(camera_view_vector);
        u = crossproduct(camera_up_vector,w) / norm(crossproduct(camera_up_vector,w));

        currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        float charDeltaSpeed = deltaTime*charSpeed;

        bool changePosition = false;
        float aux_g_DeslocX;
        float aux_g_DeslocZ;

        if (glfwGetKey(window, GLFW_KEY_W ) == GLFW_PRESS){
            changePosition = true;

            aux_g_DeslocX = g_DeslocX - w[0]*charDeltaSpeed;
            aux_g_DeslocZ = g_DeslocZ - w[2]*charDeltaSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_S ) == GLFW_PRESS){
            changePosition = true;

            aux_g_DeslocX = g_DeslocX + w[0]*charDeltaSpeed;
            aux_g_DeslocZ = g_DeslocZ + w[2]*charDeltaSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_A ) == GLFW_PRESS){
            changePosition = true;

            aux_g_DeslocX = g_DeslocX - u[0]*charDeltaSpeed;
            aux_g_DeslocZ = g_DeslocZ - u[2]*charDeltaSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_D ) == GLFW_PRESS){
            changePosition = true;

            aux_g_DeslocX = g_DeslocX + u[0]*charDeltaSpeed;
            aux_g_DeslocZ = g_DeslocZ + u[2]*charDeltaSpeed;
        }
        if (changePosition){

            if (((aux_g_DeslocZ > -35*1.5) && (aux_g_DeslocZ < 42*1.5)) && (!intersectTrees(glm::vec3(g_DeslocX,0,aux_g_DeslocZ)))){
                g_DeslocZ = aux_g_DeslocZ;
            }

            if (((aux_g_DeslocX > -35*1.5) && (aux_g_DeslocX < 35*1.5)) && (!intersectTrees(glm::vec3(aux_g_DeslocX,0,g_DeslocZ)))){
                g_DeslocX = aux_g_DeslocX;
            }

        }

        if (cowTime > 0){
            charSpeed = 30;
        }else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT ) == GLFW_PRESS){
            charSpeed = 10;
        }else{
            charSpeed = 4.5;
        }

        if ((glfwGetKey(window, GLFW_KEY_W ) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_S ) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_A ) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_D ) == GLFW_PRESS) ){
            walking1 = cos(currentTime*6)+3.14;;
            walking2 = -cos(currentTime*6)+3.14;
        }else{
            walking1 = 3.14;
            walking2 = 3.14;
        }
        
        float gunCooldown = 0.4-(wave*0.01);
        
        if (cowTime > 0){
            gunCooldown = 0;
        } 

        if ((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) && ((currentTime - gunCooldownAux) > gunCooldown)){
            gunCooldownAux = currentTime;
            if (g_thirdPerson){
                projectiles.push_back(Projectile(g_DeslocX+(-w[0]*1.5),g_DeslocY+(-w[1]*1.5)+1.2f,g_DeslocZ+(-w[2]*1.5),w,27.0f));
            }else{
                projectiles.push_back(Projectile(g_DeslocX+(-w[0]*1.5),g_DeslocY+(-w[1]*1.5)-0.2f,g_DeslocZ+(-w[2]*1.5),w,27.0f));
            }
            
        }

        bbox_min_player = glm::vec3(g_DeslocX+bbox_min_player_obj[0],g_DeslocY+bbox_min_player_obj[1],g_DeslocZ+bbox_min_player_obj[2]);
        bbox_max_player = glm::vec3(g_DeslocX+bbox_max_player_obj[0],g_DeslocY+bbox_max_player_obj[1],g_DeslocZ+bbox_max_player_obj[2]);

        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        glm::mat4 projection;

        float nearplane = -0.1f;  
        float farplane  = -150.0f; 

        float field_of_view = 3.141592 / 3.0f;
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

        glm::mat4 model = Matrix_Identity(); 

        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));


        #define SPHERE 0
        #define BUNNY 1
        #define PLANE 2
        #define ZOMBIE 3
        #define PCUBE 4
        #define GUN 5
        #define ZCUBE 6
        #define MAP 7
        #define TREE 8
        #define COW 9
        #define GRASS 10
        #define STONE 11
        #define STONE2 12
        #define TREE2 13


        if (zombies.size() <= 0){

            for (int i=0; i<(wave*2); i++){
                std::uniform_real_distribution<float> distribution1(-35,35);
                std::uniform_real_distribution<float> distribution2(0.5,wave*0.5);
                std::uniform_int_distribution<int> distribution3(1,6);

                zombies.push_back(Zombie(distribution1(generator), distribution1(generator), distribution2(generator), distribution3(generator)));
            }
            if (!cowAvaiable){
                cowAvaiable = true;
                std::uniform_real_distribution<float> distribution1(-35,35);
                cowPosition = glm::vec3(distribution1(generator),-1.0f,distribution1(generator));
            }
            std::cout << "------------ WAVE " << wave << " ------------\n";
            wave++;
        }

        if (cowAvaiable){
            model = Matrix_Translate(cowPosition[0],cowPosition[1],cowPosition[2]) *  Matrix_Scale(1.0f,1.0f,1.0f) * Matrix_Rotate_Y(currentTime);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, COW);
            DrawVirtualObject("cow");

            if (intersectCylinderY(1,cowPosition,1,glm::vec3(g_DeslocX,0.0f,g_DeslocZ))){
                cowAvaiable = false;
                cowTime = 5;
                playerLife += 2;
            }
        }

        if (cowTime > 0){
            cowTime -= deltaTime;
        }


        bool removeDeadZombiesRoutine = false;
        bool destroyProjectilesRoutine = false;

        for(int i = 0; i < zombies.size(); ++i){

            glm::mat4 model;

            glm::vec3 bbox_min = glm::vec3(zombies[i].DeslocX+bbox_min_zombie_obj[0],zombies[i].DeslocY+bbox_min_zombie_obj[1],zombies[i].DeslocZ+bbox_min_zombie_obj[2]);
            glm::vec3 bbox_max = glm::vec3(zombies[i].DeslocX+bbox_max_zombie_obj[0],zombies[i].DeslocY+bbox_max_zombie_obj[1],zombies[i].DeslocZ+bbox_max_zombie_obj[2]);

            if (intersect(bbox_min_player, bbox_max_player, bbox_min, bbox_max)){
                if((currentTime - zombies[i].hitCooldownAux) > zombies[i].hitCooldown){
                    zombies[i].hitCooldownAux = currentTime;
                    std::cout << "ZOMBIE HIT!\n";
                    playerLife -= 1;
                    std::cout << "Life: " << playerLife << "\n";
                    if(playerLife <= 0){
                        glfwSetWindowShouldClose(window, GL_TRUE);
                    }
                }
            }else{
                zombies[i].calcLocation();
            }

            model = Matrix_Translate(0.0f,1.0f,0.0f) * Matrix_Translate(zombies[i].DeslocX,zombies[i].DeslocY,zombies[i].DeslocZ) * Matrix_Rotate_Y(0.7853*zombies[i].orientation);
            
            PushMatrix(model);
                model = model * Matrix_Scale(0.1f,0.4f,0.2f);
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                glUniform1i(object_id_uniform, ZCUBE);
                DrawVirtualObject("zcube");
            PopMatrix(model);

            PushMatrix(model);
                model = model * Matrix_Translate(0.0f,0.55f,0.0f) * Matrix_Scale(0.12f,0.12f,0.12f);
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                glUniform1i(object_id_uniform, ZCUBE);
                DrawVirtualObject("zcube");
            PopMatrix(model);

            PushMatrix(model);
                model = model * Matrix_Translate(0.0f, 0.35f, -0.35f) ;
                PushMatrix(model);
                    model = model ;
                    PushMatrix(model);
                        model = model * Matrix_Rotate_Z(1.5) * Matrix_Translate(0.0f, 0.4f, 0.0f) *  Matrix_Scale(0.1f,0.4f,0.2f) * Matrix_Scale(0.9f,0.9f,0.5f); 
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, ZCUBE);
                        DrawVirtualObject("zcube");
                    PopMatrix(model);
                PopMatrix(model);
            PopMatrix(model);

            PushMatrix(model);
                model = model * Matrix_Translate(0.0f, 0.35f, 0.35f) ;
                PushMatrix(model);
                    model = model ;
                    PushMatrix(model);
                        model = model * Matrix_Rotate_Z(1.5) * Matrix_Translate(0.0f, 0.4f, 0.0f) *  Matrix_Scale(0.1f,0.4f,0.2f) * Matrix_Scale(0.9f,0.9f,0.5f); 
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, ZCUBE);
                        DrawVirtualObject("zcube");
                    PopMatrix(model);
                PopMatrix(model);
            PopMatrix(model);

            PushMatrix(model);
                model = model * Matrix_Translate(0.0f, -0.4f, 0.12f) ;
                PushMatrix(model);
                    model = model ;
                    PushMatrix(model);
                        model = model * Matrix_Rotate_Z(cos(currentTime*4)+3.14) * Matrix_Translate(0.0f, 0.4f, 0.0f) *  Matrix_Scale(0.1f,0.4f,0.2f) * Matrix_Scale(0.9f,0.9f,0.5f); 
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, ZCUBE);
                        DrawVirtualObject("zcube");
                    PopMatrix(model);
                PopMatrix(model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f, -0.4f, -0.12f) ;
                PushMatrix(model);
                    model = model ;
                    PushMatrix(model);
                        model = model * Matrix_Rotate_Z(-cos(currentTime*4)+3.14) * Matrix_Translate(0.0f, 0.4f, 0.0f) *  Matrix_Scale(0.1f,0.4f,0.2f) * Matrix_Scale(0.9f,0.9f,0.5f); 
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, ZCUBE);
                        DrawVirtualObject("zcube");
                    PopMatrix(model);
                PopMatrix(model);
            PopMatrix(model);

            if (zombies[i].dead){
                removeDeadZombiesRoutine = true;
            }
        }

        while (removeDeadZombiesRoutine){
            for(int i = 0; i < zombies.size(); ++i){
                if (zombies[i].dead){
                    zombies.erase(zombies.begin() + i);
                    break;
                }
                if ((i+1) >= (zombies.size())){
                    removeDeadZombiesRoutine = false;
                }
            }
            if (zombies.size() <= 0){
                removeDeadZombiesRoutine = false;
            }
        }

        
        for(int i = 0; i < projectiles.size(); ++i){
            projectiles[i].calcLocation();
            projectiles[i].testLifeTime();

            glm::vec3 bbox_min = glm::vec3(projectiles[i].DeslocX+bbox_min_projectile_obj[0],projectiles[i].DeslocY+bbox_min_projectile_obj[1],projectiles[i].DeslocZ+bbox_min_projectile_obj[2]);
            glm::vec3 bbox_max = glm::vec3(projectiles[i].DeslocX+bbox_max_projectile_obj[0],projectiles[i].DeslocY+bbox_max_projectile_obj[1],projectiles[i].DeslocZ+bbox_max_projectile_obj[2]);

            for(int j = 0; j < zombies.size(); ++j){

                glm::vec3 bbox_min_z = glm::vec3(zombies[j].DeslocX+bbox_min_zombie_obj[0],zombies[j].DeslocY+bbox_min_zombie_obj[1],zombies[j].DeslocZ+bbox_min_zombie_obj[2]);
                glm::vec3 bbox_max_z = glm::vec3(zombies[j].DeslocX+bbox_max_zombie_obj[0],zombies[j].DeslocY+bbox_max_zombie_obj[1],zombies[j].DeslocZ+bbox_max_zombie_obj[2]);

                if (intersect(bbox_min, bbox_max, bbox_min_z, bbox_max_z)){
                    std::cout << "PLAYER HIT!\n";
                    projectiles[i].projectileDisabled = true;
                    zombies[j].hit();
                }
            }

            model = Matrix_Translate(projectiles[i].DeslocX,projectiles[i].DeslocY,projectiles[i].DeslocZ) * Matrix_Scale(0.05f,0.05f,0.05f);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, PCUBE);
            DrawVirtualObject("pcube");

            


            if (projectiles[i].projectileDisabled){
                destroyProjectilesRoutine = true;
            }
        }
        while (destroyProjectilesRoutine){
            for(int i = 0; i < projectiles.size(); ++i){
                if (projectiles[i].projectileDisabled){
                    projectiles.erase(projectiles.begin() + i);
                    break;
                }
                if ((i+1) >= projectiles.size()){
                    destroyProjectilesRoutine = false;
                }
            }
            if (projectiles.size() <= 0){
                destroyProjectilesRoutine = false;
            }
        }
        

        model = Matrix_Translate(-10.0f,35.0f,0.0f)
              * Matrix_Rotate_Z(0.6f)
              * Matrix_Rotate_X(0.2f)
              * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f)
              * Matrix_Scale(10.0f,10.0f,10.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, SPHERE);
        DrawVirtualObject("sphere");


        if(g_thirdPerson){
            model = Matrix_Translate(0.0f,0.2f,0.0f) * Matrix_Translate(g_DeslocX,g_DeslocY,g_DeslocZ) * Matrix_Rotate_Y(g_CameraTheta-1.57);
            
            PushMatrix(model);
                model = model *  Matrix_Scale(0.1f,0.4f,0.2f);
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                glUniform1i(object_id_uniform, PCUBE);
                DrawVirtualObject("pcube");
            PopMatrix(model);

            PushMatrix(model);
                model = model * Matrix_Translate(0.0f,0.55f,0.0f) * Matrix_Scale(0.12f,0.12f,0.12f);
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                glUniform1i(object_id_uniform, PCUBE);
                DrawVirtualObject("pcube");
            PopMatrix(model);

            PushMatrix(model);
                model = model * Matrix_Translate(0.0f, 0.35f, -0.35f) ;
                PushMatrix(model);
                    model = model * Matrix_Rotate_Z(g_CameraPhi+1.57-0.4) * Matrix_Rotate_X(0.4) * Matrix_Translate(0.0f, 0.4f, 0.0f) ;
                    PushMatrix(model);
                        model = model * Matrix_Scale(0.1f,0.4f,0.2f) * Matrix_Scale(0.9f,0.9f,0.5f); 
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, PCUBE);
                        DrawVirtualObject("pcube");
                    PopMatrix(model);

                    PushMatrix(model);
                        model = model * Matrix_Translate(0.0f, 0.4f, 0.04f) * Matrix_Rotate_X(-0.4) * Matrix_Rotate_Z(-1.5) * Matrix_Scale(0.08f,0.08f,0.08f) * Matrix_Rotate_Y(1.57);
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, GUN);
                        DrawVirtualObject("gun");
                    PopMatrix(model);
                    
                PopMatrix(model);
            PopMatrix(model);

            PushMatrix(model);
                model = model * Matrix_Translate(0.0f, 0.35f, 0.35f) ;
                PushMatrix(model);
                    model = model ;
                    PushMatrix(model); 
                        model = model  * Matrix_Rotate_Z(g_CameraPhi+1.57-0.4) * Matrix_Rotate_X(-0.4) * Matrix_Translate(0.0f, 0.4f, 0.0f) *  Matrix_Scale(0.1f,0.4f,0.2f) * Matrix_Scale(0.9f,0.9f,0.5f); 
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, PCUBE);
                        DrawVirtualObject("pcube");
                    PopMatrix(model);
                PopMatrix(model);
            PopMatrix(model);

            PushMatrix(model);
                model = model * Matrix_Translate(0.0f, -0.4f, 0.12f) ;
                PushMatrix(model);
                    model = model ;
                    PushMatrix(model);
                        model = model * Matrix_Rotate_Z(walking1) * Matrix_Translate(0.0f, 0.4f, 0.0f) *  Matrix_Scale(0.1f,0.4f,0.2f) * Matrix_Scale(0.9f,0.9f,0.5f); 
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, PCUBE);
                        DrawVirtualObject("pcube");
                    PopMatrix(model);
                PopMatrix(model);
            PopMatrix(model);
            PushMatrix(model);
     
                model = model * Matrix_Translate(0.0f, -0.4f, -0.12f) ;
                PushMatrix(model);
                    model = model ;
                    PushMatrix(model);
                        model = model * Matrix_Rotate_Z(walking2) * Matrix_Translate(0.0f, 0.4f, 0.0f) *  Matrix_Scale(0.1f,0.4f,0.2f) * Matrix_Scale(0.9f,0.9f,0.5f); 
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        glUniform1i(object_id_uniform, PCUBE);
                        DrawVirtualObject("pcube");
                    PopMatrix(model);
                PopMatrix(model);
            PopMatrix(model);
            
        }else{
            model =  Matrix_Translate(g_DeslocX,g_DeslocY,g_DeslocZ) * Matrix_Rotate_Y(g_CameraTheta+2.35)  * Matrix_Translate(0.5f,-0.2f-(g_CameraPhi/2),0.5f)  * Matrix_Scale(0.08f,0.08f,0.08f)  * Matrix_Rotate_Y(4) *  Matrix_Rotate_X(-g_CameraPhi);
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(object_id_uniform, GUN);
            DrawVirtualObject("gun");
        }

        model = Matrix_Translate(1.0f,-1.1f,1.0f) *  Matrix_Scale(1.5f,2.0f,1.5f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, MAP);
        DrawVirtualObject("map");

        for (int i=0; i<trees.size();i++){
            if ((trees[i][3]) == 0){
                model = Matrix_Translate(trees[i][0],-1.1f,trees[i][2]) *  Matrix_Scale(1.5f,2.0f,1.5f) * Matrix_Rotate_Y(trees[i][0]);
                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(object_id_uniform, TREE);
                DrawVirtualObject("tree");
            }else{
                model = Matrix_Translate(trees[i][0],-1.1f,trees[i][2]) *  Matrix_Scale(0.4f,0.4f,0.4f) * Matrix_Rotate_Y(trees[i][0]);
                glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(object_id_uniform, TREE2);
                DrawVirtualObject("tree2");
            }

        }

        for (int i=0; i<grass.size();i++){
            model = Matrix_Translate(grass[i][0],-1.1f,grass[i][2]) *  Matrix_Scale(0.4f,0.4f,0.4f) * Matrix_Rotate_Y(grass[i][0]);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, GRASS);
            DrawVirtualObject("grass");
        }

        for (int i=0; i<stones1.size();i++){
            model = Matrix_Translate(stones1[i][0],-1.1f,stones1[i][2]) *  Matrix_Scale(0.6f,0.6f,0.6f) * Matrix_Rotate_Y(stones1[i][0])* Matrix_Rotate_X(stones1[i][2]);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, STONE);
            DrawVirtualObject("stone");
        }

        for (int i=0; i<stones2.size();i++){
            model = Matrix_Translate(stones2[i][0],((abs(stones2[i][0])+abs(stones2[i][2]))/90),stones2[i][2]) *  Matrix_Scale(3.0f,3.0f,3.0f) * Matrix_Rotate_Y(stones2[i][0]) * Matrix_Rotate_X(stones2[i][2]);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, STONE2);
            DrawVirtualObject("stone2");
        }


        glfwSwapBuffers(window);
        

        glfwPollEvents();
    }


    glfwTerminate();

    return 0;
}
    

void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

void DrawVirtualObject(const char* object_name)
{
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    glBindVertexArray(0);
}

void LoadShadersFromFiles()
{

    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    if ( program_id != 0 )
        glDeleteProgram(program_id);

    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    model_uniform           = glGetUniformLocation(program_id, "model"); 
    view_uniform            = glGetUniformLocation(program_id, "view"); 
    projection_uniform      = glGetUniformLocation(program_id, "projection"); 
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); 
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");


    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage10"), 10);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage11"), 11);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage12"), 12);   
    glUseProgram(0);
}


void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}


GLuint LoadShader_Vertex(const char* filename)
{
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    LoadShader(filename, vertex_shader_id);

    return vertex_shader_id;
}

GLuint LoadShader_Fragment(const char* filename)
{
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    LoadShader(filename, fragment_shader_id);

    return fragment_shader_id;
}

void LoadShader(const char* filename, GLuint shader_id)
{
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    glCompileShader(shader_id);

    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    delete [] log;
}

void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];

                model_coefficients.push_back( vx ); 
                model_coefficients.push_back( vy ); 
                model_coefficients.push_back( vz ); 
                model_coefficients.push_back( 1.0f ); 

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);


                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); 
                    normal_coefficients.push_back( ny ); 
                    normal_coefficients.push_back( nz );
                    normal_coefficients.push_back( 0.0f ); 
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; 
        theobject.num_indices    = last_index - first_index + 1; 
        theobject.rendering_mode = GL_TRIANGLES;    
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; 
    GLint  number_of_dimensions = 4; 
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; 
        number_of_dimensions = 4; 
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; 
        number_of_dimensions = 2; 
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    glBindVertexArray(0);
}

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    GLuint program_id = glCreateProgram();

    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    glLinkProgram(program_id);

    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    return program_id;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    g_ScreenRatio = (float)width / height;
}

double g_LastCursorPosX, g_LastCursorPosY;


void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{

    if (g_paused)
        return;

    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    g_CameraTheta -= 0.01f*dx;
    g_CameraPhi   += 0.01f*dy;

    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_thirdPerson = !g_thirdPerson;
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_paused_aux = true;
        g_paused = !g_paused;
    }

}

void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}
