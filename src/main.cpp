
#include <cmath>
#include <cstdio>
#include <cstdlib>


#include <map>
#include <stack>
#include <string>
#include <limits>
#include <fstream>
#include <sstream>
#include <bits/stdc++.h>


#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "utils.h"
#include "matrices.h"

void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

void DrawCube(GLint render_as_black_uniform); // Desenha um cubo
GLuint BuildTriangles();
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);


void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);


struct SceneObject
{
    const char*  name;
    void*        first_index;
    int          num_indices;
    GLenum       rendering_mode;
};


std::map<const char*, SceneObject> g_VirtualScene;
std::stack<glm::mat4>  g_MatrixStack;

float g_ScreenRatio = 1.0f;

float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;


float speed = 0.1;

glm::vec4 w;
glm::vec4 u;

float g_DeslocX = 1.0f;
float g_DeslocY = 0.0f;
float g_DeslocZ = 1.0f;

class Zombie{
    public:
        float speed;
        float DeslocX, DeslocY, DeslocZ;
        Zombie(float x, float z, float s);
        void calcLocation();

};

Zombie::Zombie(float x,float z, float s){
    speed = s;
    DeslocX = x;
    DeslocY = 0.0f;
    DeslocZ = z;
}

void Zombie::calcLocation(){
    if (DeslocX > g_DeslocX){
        DeslocX -= speed;
    }else{
        DeslocX += speed;
    }

    if (DeslocZ > g_DeslocZ){
        DeslocZ -= speed;
    }else{
        DeslocZ += speed;
    }
}

std::vector<Zombie> zombies;


bool g_LeftMouseButtonPressed = false;


float g_CameraTheta = 0.0f;
float g_CameraPhi = 0.0f;
float g_CameraDistance = 2.5f;

float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

bool g_UsePerspectiveProjection = true;

bool g_ShowInfoText = true;

int main()
{
    zombies.push_back(Zombie(4.0,5.0,0.008));
    zombies.push_back(Zombie(-4.0,-5.0,0.001));
    zombies.push_back(Zombie(2.0,3.0,0.005));
    zombies.push_back(Zombie(1.0,4.0,0.003));



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
    window = glfwCreateWindow(1000, 1000, "Trabalho Final", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }


    //glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    //glfwSetScrollCallback(window, ScrollCallback);


    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowSize(window, 1000, 1000);

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    GLuint program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    GLuint vertex_array_object_id = BuildTriangles();


    GLint model_uniform           = glGetUniformLocation(program_id, "model");
    GLint view_uniform            = glGetUniformLocation(program_id, "view");
    GLint projection_uniform      = glGetUniformLocation(program_id, "projection");
    GLint render_as_black_uniform = glGetUniformLocation(program_id, "render_as_black");

    glEnable(GL_DEPTH_TEST);

    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;


    while (!glfwWindowShouldClose(window))
    {

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);

        glBindVertexArray(vertex_array_object_id);

        float r = g_CameraDistance;
        float y = sin(g_CameraPhi);
        float z = cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = cos(g_CameraPhi)*sin(g_CameraTheta);

        glm::vec4 camera_position_c  = glm::vec4(g_DeslocX,0.0f,g_DeslocZ,1.0f);
        glm::vec4 camera_lookat_l    = glm::vec4(0.0f+g_DeslocX,0.0f+g_DeslocY,0.0f+g_DeslocZ,1.0f);
        glm::vec4 camera_view_vector = camera_position_c - (camera_position_c+glm::vec4(x,y,z,0.0f));;
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);

        w = -camera_view_vector / norm(camera_view_vector);
        u = crossproduct(camera_up_vector,w) / norm(crossproduct(camera_up_vector,w));

        if (glfwGetKey(window, GLFW_KEY_W ) == GLFW_PRESS){
            g_DeslocX -= w[0]*speed;
            g_DeslocY -= w[1]*speed;
            g_DeslocZ -= w[2]*speed;
        }

        if (glfwGetKey(window, GLFW_KEY_S ) == GLFW_PRESS){
            g_DeslocX += w[0]*speed;
            g_DeslocY += w[1]*speed;
            g_DeslocZ += w[2]*speed;
        }

        if (glfwGetKey(window, GLFW_KEY_A ) == GLFW_PRESS){
            g_DeslocX -= u[0]*speed;
            g_DeslocY -= u[1]*speed;
            g_DeslocZ -= u[2]*speed;
        }

        if (glfwGetKey(window, GLFW_KEY_D ) == GLFW_PRESS){
            g_DeslocX += u[0]*speed;
            g_DeslocY += u[1]*speed;
            g_DeslocZ += u[2]*speed;
        }

        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        glm::mat4 projection;

        float nearplane = -0.1f;  
        float farplane  = -40.0f; 

        float field_of_view = 3.141592 / 3.0f;
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));


        for(int i = 0; i < zombies.size(); ++i){

            glm::mat4 model;

            zombies[i].calcLocation();

            model = Matrix_Identity() * Matrix_Translate(zombies[i].DeslocX, zombies[i].DeslocY, zombies[i].DeslocZ);

            // Translação inicial do torso
        model = model * Matrix_Translate(g_TorsoPositionX - 1.0f, g_TorsoPositionY + 1.0f, 0.0f);
        // Guardamos matriz model atual na pilha
        PushMatrix(model);
            // Atualizamos a matriz model (multiplicação à direita) para fazer um escalamento do torso
            model = model * Matrix_Scale(0.8f, 1.0f, 0.2f);
            // Enviamos a matriz "model" para a placa de vídeo (GPU). Veja o
            // arquivo "shader_vertex.glsl", onde esta é efetivamente
            // aplicada em todos os pontos.
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            // Desenhamos um cubo. Esta renderização irá executar o Vertex
            // Shader definido no arquivo "shader_vertex.glsl", e o mesmo irá
            // utilizar as matrizes "model", "view" e "projection" definidas
            // acima e já enviadas para a placa de vídeo (GPU).
            DrawCube(render_as_black_uniform); // #### TORSO
        // Tiramos da pilha a matriz model guardada anteriormente
        PopMatrix(model);
        // Neste ponto a matriz model recuperada é a matriz inicial (translação do torso)

/* ========= Cabeça ========= */

        PushMatrix(model); // Guardamos matriz model atual na pilha
            model = model * Matrix_Rotate_Z(3.14159); // Rotacionamos o cubo em 180 graus ao redor de Z para que o eixo seja correspondente à um pescoço
            model = model * Matrix_Translate(0.0f, -0.75f, 0.0f); // Atualizamos matriz model com uma translação
            PushMatrix(model); // Guardamos matriz model atual na pilha
                model = model // Atualizamos matriz model com a rotação
                      * Matrix_Rotate_Z(g_AngleZ)  // TERCEIRO rotação Z de Euler
                      * Matrix_Rotate_Y(g_AngleY)  // SEGUNDO rotação Y de Euler
                      * Matrix_Rotate_X(g_AngleX); // PRIMEIRO rotação X de Euler
                PushMatrix(model); // Guardamos matriz model atual na pilha
                    model = model * Matrix_Scale(0.3f, 0.3f, 0.3f); // Atualizamos matriz model com um escalamento
                    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                    DrawCube(render_as_black_uniform); // #### CABEÇA - Desenhamos a cabeça
                PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
        PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

/* ========= Braço Direito ========= */

        PushMatrix(model); // Guardamos matriz model atual na pilha
            model = model * Matrix_Translate(-0.55f, 0.0f, 0.0f); // Atualizamos matriz model (multiplicação à direita) com uma translação para o braço direito
            PushMatrix(model); // Guardamos matriz model atual na pilha
                model = model // Atualizamos matriz model (multiplicação à direita) com a rotação do braço direito
                      * Matrix_Rotate_Z(g_AngleZ)  // TERCEIRO rotação Z de Euler
                      * Matrix_Rotate_Y(g_AngleY)  // SEGUNDO rotação Y de Euler
                      * Matrix_Rotate_X(g_AngleX); // PRIMEIRO rotação X de Euler

                PushMatrix(model); // Guardamos matriz model atual na pilha
                    model = model * Matrix_Scale(0.2f, 0.6f, 0.2f); // Atualizamos matriz model (multiplicação à direita) com um escalamento do braço direito
                    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                    DrawCube(render_as_black_uniform); // #### BRAÇO DIREITO - Desenhamos o braço direito
                PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

                /* ========= Antebraço Direito ========= */

                PushMatrix(model); // Guardamos matriz model atual na pilha
                    model = model * Matrix_Translate(0.0f, -0.65f, 0.0f); // Atualizamos matriz model (multiplicação à direita) com a translação do antebraço direito
                    model = model // Atualizamos matriz model (multiplicação à direita) com a rotação do antebraço direito
                          * Matrix_Rotate_Z(g_ForearmAngleZ)  // SEGUNDO rotação Z de Euler
                          * Matrix_Rotate_X(g_ForearmAngleX); // PRIMEIRO rotação X de Euler

                    PushMatrix(model); // Guardamos matriz model atual na pilha
                        model = model * Matrix_Scale(0.2f, 0.6f, 0.2f); // Atualizamos matriz model (multiplicação à direita) com um escalamento do antebraço direito
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                        DrawCube(render_as_black_uniform); // #### ANTEBRAÇO DIREITO - Desenhamos o antebraço direito

                        /* ========= Mão Direita ========= */

                        PushMatrix(model); // Guardamos matriz model atual na pilha
                            model = model * Matrix_Translate(0.0f, -0.655f, 0.0f); // Atualizamos matriz model com translação
                            PushMatrix(model); // Guardamos matriz model atual na pilha
                                model = model * Matrix_Scale(1.0f, 0.15f, 1.0f); // Atualizamos matriz model com escalamento para ficar achatado e parecer uma mão
                                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                                DrawCube(render_as_black_uniform); // #### MÃO DIREITA - Desenhamos a mão direita
                            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
                        PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

                    PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
                PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
        PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

/* ========= Braço Esquerdo ========= */

        PushMatrix(model); // Guardamos matriz model atual na pilha
            model = model * Matrix_Translate(+0.55f, 0.0f, 0.0f); // Atualizamos matriz model com uma translação para o braço esquerdo
            PushMatrix(model); // Guardamos matriz model atual na pilha
                model = model // Atualizamos matriz model com a rotação do braço esquerdo
                      * Matrix_Rotate_Z(-g_AngleZ)  // TERCEIRO rotação Z de Euler
                      * Matrix_Rotate_Y(g_AngleY)  // SEGUNDO rotação Y de Euler
                      * Matrix_Rotate_X(g_AngleX); // PRIMEIRO rotação X de Euler

                PushMatrix(model); // Guardamos matriz model atual na pilha
                    model = model * Matrix_Scale(0.2f, 0.6f, 0.2f); // Atualizamos matriz model com um escalamento do braço esquerdo
                    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                    DrawCube(render_as_black_uniform); // #### BRAÇO ESQUERDO - Desenhamos o braço esquerdo
                PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

                /* ========= Antebraço Esquerdo ========= */

                PushMatrix(model); // Guardamos matriz model atual na pilha
                    model = model * Matrix_Translate(0.0f, -0.65f, 0.0f); // Atualizamos matriz model com a translação do antebraço esquerdo
                    model = model // Atualizamos matriz model com a rotação do antebraço esquerdo
                          * Matrix_Rotate_Z(-g_ForearmAngleZ)  // SEGUNDO rotação Z de Euler
                          * Matrix_Rotate_X(g_ForearmAngleX); // PRIMEIRO rotação X de Euler

                    PushMatrix(model); // Guardamos matriz model atual na pilha
                        model = model * Matrix_Scale(0.2f, 0.6f, 0.2f); // Atualizamos matriz model com um escalamento do antebraço esquerdo
                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                        DrawCube(render_as_black_uniform); // #### ANTEBRAÇO ESQUERDO - Desenhamos o antebraço esquerdo

                        /* ========= Mão Esquerda ========= */

                        PushMatrix(model); // Guardamos matriz model atual na pilha
                            model = model * Matrix_Translate(0.0f, -0.655f, 0.0f); // Atualizamos matriz model com translação
                            PushMatrix(model); // Guardamos matriz model atual na pilha
                                model = model * Matrix_Scale(1.0f, 0.15f, 1.0f); // Atualizamos matriz model com escalamento para ficar achatado e parecer uma mão
                                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                                DrawCube(render_as_black_uniform); // #### MÃO ESQUERDA - Desenhamos a mão esquerda
                            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
                        PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
                    PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

                PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
        PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

/* ========= Perna Direita ========= */

        PushMatrix(model); // Guardamos matriz model atual na pilha
            model = model * Matrix_Translate(-0.20f, -0.90f, 0.0f); // Atualizamos matriz model com uma translação
            PushMatrix(model); // Guardamos matriz model atual na pilha
                model = model * Matrix_Scale(0.3f, 0.7f, 0.3f); // Atualizamos matriz model com um escalamento
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                DrawCube(render_as_black_uniform); // #### PERNA DIREITA - Desenhamos a perna direita
            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

            /* ========= Canela Direita ========= */

            PushMatrix(model); // Guardamos matriz model atual na pilha
                model = model * Matrix_Translate(0.0f, -0.75f, 0.0f); // Atualizamos matriz model com uma translação
                PushMatrix(model); // Guardamos matriz model atual na pilha
                    model = model * Matrix_Scale(0.25f, 0.7f, 0.25f); // Atualizamos matriz model com um escalamento
                    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                    DrawCube(render_as_black_uniform); // #### CANELA DIREITA - Desenhamos a canela direita

                    /* ========= Pé Direito ========= */

                    PushMatrix(model); // Guardamos matriz model atual na pilha
                        model = model * Matrix_Translate(0.0f, -0.67f, 0.45f); // Atualizamos matriz model com uma translação
                        PushMatrix(model); // Guardamos matriz model atual na pilha
                            model = model * Matrix_Scale(0.8f, 0.15f, 2.1f); // Atualizamos matriz model com um escalamento para parecer um pé
                            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                            DrawCube(render_as_black_uniform); // #### PÉ DIREITO - Desenhamos o pé direito
                        PopMatrix(model);// Tiramos da pilha a matriz model guardada anteriormente
                    PopMatrix(model);// Tiramos da pilha a matriz model guardada anteriormente

                PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
        PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

/* ========= Perna Esquerda ========= */

         PushMatrix(model); // Guardamos matriz model atual na pilha
            model = model * Matrix_Translate(+0.20f, -0.90f, 0.0f); // Atualizamos matriz model com uma translação
            PushMatrix(model); // Guardamos matriz model atual na pilha
                model = model * Matrix_Scale(0.3f, 0.7f, 0.3f); // Atualizamos matriz model com um escalamento
                glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                DrawCube(render_as_black_uniform); // #### PERNA ESQUERDA - Desenhamos a perna esquerda
            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

            /* ========= Canela Esquerda ========= */

            PushMatrix(model); // Guardamos matriz model atual na pilha
                model = model * Matrix_Translate(0.0f, -0.75f, 0.0f); // Atualizamos matriz model com uma translação
                PushMatrix(model); // Guardamos matriz model atual na pilha
                    model = model * Matrix_Scale(0.25f, 0.7f, 0.25f); // Atualizamos matriz model com um escalamento
                    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                    DrawCube(render_as_black_uniform); // #### CANELA ESQUERDA - Desenhamos a canela esquerda

                    /* ========= Pé Esquerdo ========= */

                    PushMatrix(model); // Guardamos matriz model atual na pilha
                        model = model * Matrix_Translate(0.0f, -0.67f, 0.45f); // Atualizamos matriz model com uma translação
                        PushMatrix(model); // Guardamos matriz model atual na pilha
                            model = model * Matrix_Scale(0.8f, 0.15f, 2.1f); // Atualizamos matriz model com um escalamento para parecer um pé
                            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model)); // Enviamos matriz model atual para a GPU
                            DrawCube(render_as_black_uniform); // #### PÉ ESQUERDO - Desenhamos o pé esquerdo
                        PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
                    PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

                PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
            PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente
        PopMatrix(model); // Tiramos da pilha a matriz model guardada anteriormente

        // Agora queremos desenhar os eixos XYZ de coordenadas GLOBAIS.
        // Para tanto, colocamos a matriz de modelagem igual à identidade.
        // Veja slides 2-14 e 184-190 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        model = Matrix_Identity();

            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(render_as_black_uniform, false);
            glDrawElements(
                g_VirtualScene["cube_faces"].rendering_mode,
                g_VirtualScene["cube_faces"].num_indices,
                GL_UNSIGNED_INT,
                (void*)g_VirtualScene["cube_faces"].first_index
            );
            glLineWidth(4.0f);

            glDrawElements(
                g_VirtualScene["axes"].rendering_mode,
                g_VirtualScene["axes"].num_indices,
                GL_UNSIGNED_INT,
                (void*)g_VirtualScene["axes"].first_index
            );

            glUniform1i(render_as_black_uniform, true);

            glDrawElements(
                g_VirtualScene["cube_edges"].rendering_mode,
                g_VirtualScene["cube_edges"].num_indices,
                GL_UNSIGNED_INT,
                (void*)g_VirtualScene["cube_edges"].first_index
            );
        }


        glm::mat4 model = Matrix_Identity();
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glLineWidth(10.0f);
        glUniform1i(render_as_black_uniform, false);
        glDrawElements(
            g_VirtualScene["axes"].rendering_mode,
            g_VirtualScene["axes"].num_indices,
            GL_UNSIGNED_INT,
            (void*)g_VirtualScene["axes"].first_index
        );

        glBindVertexArray(0);

        glm::vec4 p_model(0.5f, 0.5f, 0.5f, 1.0f);

         glfwSwapBuffers(window);


        glfwPollEvents();
    }


    glfwTerminate();

    return 0;
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


void DrawCube(GLint render_as_black_uniform)
{

    glUniform1i(render_as_black_uniform, false);

    glDrawElements(
        g_VirtualScene["cube_faces"].rendering_mode, // Veja slides 124-130 do documento Aula_04_Modelagem_Geometrica_3D.pdf
        g_VirtualScene["cube_faces"].num_indices,    //
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene["cube_faces"].first_index
    );

    // Pedimos para OpenGL desenhar linhas com largura de 4 pixels.
    glLineWidth(4.0f);

     glDrawElements(
        g_VirtualScene["axes"].rendering_mode,
        g_VirtualScene["axes"].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene["axes"].first_index
    );

    glUniform1i(render_as_black_uniform, true);

    glDrawElements(
        g_VirtualScene["cube_edges"].rendering_mode,
        g_VirtualScene["cube_edges"].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene["cube_edges"].first_index
    );
}

GLuint BuildTriangles()
{
    GLfloat model_coefficients[] = {
        -0.5f,  0.5f,  0.5f, 1.0f, // posição do vértice 0
        -0.5f, -0.5f,  0.5f, 1.0f, // posição do vértice 1
         0.5f, -0.5f,  0.5f, 1.0f, // posição do vértice 2
         0.5f,  0.5f,  0.5f, 1.0f, // posição do vértice 3
        -0.5f,  0.5f, -0.5f, 1.0f, // posição do vértice 4
        -0.5f, -0.5f, -0.5f, 1.0f, // posição do vértice 5
         0.5f, -0.5f, -0.5f, 1.0f, // posição do vértice 6
         0.5f,  0.5f, -0.5f, 1.0f, // posição do vértice 7
    // Vértices para desenhar o eixo X
    //    X      Y     Z     W
         0.0f,  0.0f,  0.0f, 1.0f, // posição do vértice 8
         1.0f,  0.0f,  0.0f, 1.0f, // posição do vértice 9
    // Vértices para desenhar o eixo Y
    //    X      Y     Z     W
         0.0f,  0.0f,  0.0f, 1.0f, // posição do vértice 10
         0.0f,  1.0f,  0.0f, 1.0f, // posição do vértice 11
    // Vértices para desenhar o eixo Z
    //    X      Y     Z     W
         0.0f,  0.0f,  0.0f, 1.0f, // posição do vértice 12
         0.0f,  0.0f,  1.0f, 1.0f, // posição do vértice 13
    };

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);

    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);

    glBindVertexArray(vertex_array_object_id);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);

    glBufferData(GL_ARRAY_BUFFER, sizeof(model_coefficients), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(model_coefficients), model_coefficients);

    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(location);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLfloat color_coefficients[] = {
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 1
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 2
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 3
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 4
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 5
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 6
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 7
    // Cores para desenhar o eixo X
        1.0f, 0.0f, 0.0f, 1.0f, // cor do vértice 8
        1.0f, 0.0f, 0.0f, 1.0f, // cor do vértice 9
    // Cores para desenhar o eixo Y
        0.0f, 1.0f, 0.0f, 1.0f, // cor do vértice 10
        0.0f, 1.0f, 0.0f, 1.0f, // cor do vértice 11
    // Cores para desenhar o eixo Z
        0.0f, 0.0f, 1.0f, 1.0f, // cor do vértice 12
        0.0f, 0.0f, 1.0f, 1.0f, // cor do vértice 13
    };
    GLuint VBO_color_coefficients_id;
    glGenBuffers(1, &VBO_color_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_color_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color_coefficients), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color_coefficients), color_coefficients);
    location = 1; // "(location = 1)" em "shader_vertex.glsl"
    number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint indices[] = {
        0, 1, 2, // triângulo 1
        7, 6, 5, // triângulo 2
        3, 2, 6, // triângulo 3
        4, 0, 3, // triângulo 4
        4, 5, 1, // triângulo 5
        1, 5, 6, // triângulo 6
        0, 2, 3, // triângulo 7
        7, 5, 4, // triângulo 8
        3, 6, 7, // triângulo 9
        4, 3, 7, // triângulo 10
        4, 1, 0, // triângulo 11
        1, 6, 2, // triângulo 12

        0, 1, // linha 1
        1, 2, // linha 2
        2, 3, // linha 3
        3, 0, // linha 4
        0, 4, // linha 5
        4, 7, // linha 6
        7, 6, // linha 7
        6, 2, // linha 8
        6, 5, // linha 9
        5, 4, // linha 10
        5, 1, // linha 11
        7, 3, // linha 12

        8 , 9 , // linha 1
        10, 11, // linha 2
        12, 13  // linha 3
    };

    SceneObject cube_faces;
    cube_faces.name           = "Cubo (faces coloridas)";
    cube_faces.first_index    = (void*)0; // Primeiro índice está em indices[0]
    cube_faces.num_indices    = 36;       // Último índice está em indices[35]; total de 36 índices.
    cube_faces.rendering_mode = GL_TRIANGLES; // Índices correspondem ao tipo de rasterização GL_TRIANGLES.

    g_VirtualScene["cube_faces"] = cube_faces;

    SceneObject cube_edges;
    cube_edges.name           = "Cubo (arestas pretas)";
    cube_edges.first_index    = (void*)(36*sizeof(GLuint)); // Primeiro índice está em indices[36]
    cube_edges.num_indices    = 24; // Último índice está em indices[59]; total de 24 índices.
    cube_edges.rendering_mode = GL_LINES; // Índices correspondem ao tipo de rasterização GL_LINES.

    g_VirtualScene["cube_edges"] = cube_edges;

    SceneObject axes;
    axes.name           = "Eixos XYZ";
    axes.first_index    = (void*)(60*sizeof(GLuint)); // Primeiro índice está em indices[60]
    axes.num_indices    = 6; // Último índice está em indices[65]; total de 6 índices.
    axes.rendering_mode = GL_LINES; // Índices correspondem ao tipo de rasterização GL_LINES.
    g_VirtualScene["axes"] = axes;

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    glBindVertexArray(0);

    return vertex_array_object_id;
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

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {

        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {

        g_LeftMouseButtonPressed = false;
    }
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{

    if (!g_LeftMouseButtonPressed)
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


void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}
