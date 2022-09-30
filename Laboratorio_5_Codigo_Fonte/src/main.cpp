//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 5
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
// Headers das bibliotecas OpenGL
#include <glad/glad.h>  // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h> // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "bezier.h"
#include "opponent.h"
#define PI 3.14159265358979323846
// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char *filename, const char *basepath = NULL, bool triangulate = true)
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

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4 &M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel *);                         // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel *model);                                        // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles();                                                 // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char *filename);                                 // Função que carrega imagens de textura

void DrawVirtualObject(const char *object_name);                             // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char *filename);                              // Carrega um vertex shader
GLuint LoadShader_Fragment(const char *filename);                            // Carrega um fragment shader
void LoadShader(const char *filename, GLuint shader_id);                     // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel *);                                          // Função para debugging

void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string name;              // Nome do objeto
    size_t first_index;            // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t num_indices;            // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum rendering_mode;         // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3 bbox_min;            // Axis-Aligned Bounding Box do objeto
    glm::vec3 bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4> g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;  // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse
bool wPressed = false;
bool aPressed = false;
bool sPressed = false;
bool dPressed = false;
bool ctrlPressed=false;
bool spacePressed=false;
bool startPressed=false;
// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = -PI/2;    // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;      // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem
float camSpeed=10.0f;
glm::vec4 c=glm::vec4(0.0f,0.0f,0.0f,1.0f);

// Variável que controla o tipo de camera.
int camType = 0;

// Variável que controla se o texto informativo será mostrado na tela.

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;



int main(int argc, char *argv[])
{

    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow *window;
    window = glfwCreateWindow(800, 600, "INF01047 - 00326872 e 00323915 - Izaias Saturnino de Lima Neto e João Pedro Lopes Bazotti", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *glversion = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos as imagens para serem utilizadas como textura
    LoadTextureImage("../../data/op.png");
    LoadTextureImage("../../data/BF.png");
    LoadTextureImage("../../data/retro.png");
    LoadTextureImage("../../data/track.png");
    LoadTextureImage("../../data/start.png");


    // Construímos a representação de objetos geométricos através de malhas de triângulos

    ObjModel carmodel("../../data/opponent.obj");
    ComputeNormals(&carmodel);
    BuildTrianglesAndAddToVirtualScene(&carmodel);

    ObjModel playermodel("../../data/blue_falcon.obj");
    ComputeNormals(&playermodel);
    BuildTrianglesAndAddToVirtualScene(&playermodel);

    ObjModel trackmodel("../../data/track.obj");
    ComputeNormals(&trackmodel);
    BuildTrianglesAndAddToVirtualScene(&trackmodel);

    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel decormodel("../../data/decor.obj");
    ComputeNormals(&decormodel);
    BuildTrianglesAndAddToVirtualScene(&decormodel);

    ObjModel startmodel("../../data/start.obj");
    ComputeNormals(&startmodel);
    BuildTrianglesAndAddToVirtualScene(&startmodel);

    if (argc > 1)
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    TextRendering_Init();

    // Habilitamos o Z-buffer, ele depois é manipulado para desenhar o cenario.
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // time vars
    float prev_time = (float)glfwGetTime();
    float delta_t = 0.0f;
    // player vars
    glm::vec4 carPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 carForward = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 modelPlayer;
    float max_velocity = 20.0;
    float friction = 0.7;
    glm::vec4 current_velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 acceleration = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 frame_movement = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

    // initial model manipulation
    modelPlayer = Matrix_Identity();
    modelPlayer = Matrix_Translate(carPos.x, carPos.y, carPos.z) * modelPlayer;
    modelPlayer = Matrix_Rotate_Y(3.141592 / 2) * modelPlayer;

    // oponnent vars and model manipulation
    glm::vec4 oldPos1 = glm::vec4(0.0f, 0.16f, 2.0f, 1.0f);
    glm::vec4 oldPos2 = glm::vec4(0.0f, 0.16f, -2.0f, 1.0f);

    glm::mat4 modelOponnent1;
    modelOponnent1 = Matrix_Identity();
    modelOponnent1 = Matrix_Scale(0.0012, 0.0012, 0.0012) * modelOponnent1;
    modelOponnent1 = Matrix_Rotate_Y(3.141592 / 2) * modelOponnent1;
    modelOponnent1 = Matrix_Translate(oldPos1.x, oldPos1.y, oldPos1.z) * modelOponnent1;
    glm::vec4 opponnent1forward = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 opponnent1pos = glm::vec4(oldPos1.x, oldPos1.y, oldPos1.z, 1.0f);

    glm::mat4 modelOponnent2;
    modelOponnent2 = Matrix_Identity();
    modelOponnent2 = Matrix_Scale(0.0012, 0.0012, 0.0012) * modelOponnent2;
    modelOponnent2 = Matrix_Rotate_Y(3.141592 / 2) * modelOponnent2;
    modelOponnent2 = Matrix_Translate(oldPos2.x, oldPos2.y, oldPos2.z) * modelOponnent2;
    glm::vec4 opponnent2forward = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 opponnent2pos = glm::vec4(oldPos2.x, oldPos2.y, oldPos2.z, 1.0f);

    // bezier control points1
    std::vector<glm::vec4> controlPoints1_1;
    controlPoints1_1.push_back(oldPos1);
    controlPoints1_1.push_back(glm::vec4(156.483f, 0.16f, -3.52993f, 1.0f));
    controlPoints1_1.push_back(glm::vec4(134.85f, 0.16f, -6.45085f, 1.0f));
    controlPoints1_1.push_back(glm::vec4(129.0f, 0.16f, 60.9768f, 1.0f));

    std::vector<glm::vec4> controlPoints1_2;
    controlPoints1_2.push_back(glm::vec4(129.0f, 0.16f, 60.9768f, 1.0f));
    controlPoints1_2.push_back(glm::vec4(99.3377f, 0.16f, 71.0411f, 1.0f));
    controlPoints1_2.push_back(glm::vec4(121.338f, 0.16f, 30.559f, 1.0f));
    controlPoints1_2.push_back(glm::vec4(69.6432f, 0.16f, 41.1186f, 1.0f));

    std::vector<glm::vec4> controlPoints1_3;
    controlPoints1_3.push_back(glm::vec4(69.6432f, 0.16f, 41.1186f, 1.0f));
    controlPoints1_3.push_back(glm::vec4(23.2552f, 0.16f, 32.2232f, 1.0f));
    controlPoints1_3.push_back(glm::vec4(24.7635f, 0.16f, 47.7012f, 1.0f));
    controlPoints1_3.push_back(glm::vec4(26.857f, 0.16f, 105.02f, 1.0f));

    std::vector<glm::vec4> controlPoints1_4;
    controlPoints1_4.push_back(glm::vec4(26.857f, 0.16f, 105.02f, 1.0f));
    controlPoints1_4.push_back(glm::vec4(10.3733f, 0.16f, 123.0f, 1.0f));
    controlPoints1_4.push_back(glm::vec4(5.2825f, 0.16f, 124.649f, 1.0f));
    controlPoints1_4.push_back(glm::vec4(2.76479f, 0.16f, 65.1178f, 1.0f));
    std::vector<glm::vec4> controlPoints1_5;
    controlPoints1_5.push_back(glm::vec4(2.76479f, 0.16f, 65.1178f, 1.0f));
    controlPoints1_5.push_back(glm::vec4(5.32203f, 0.16f, 51.0035f, 1.0f));
    controlPoints1_5.push_back(glm::vec4(-53.3087f, 0.16f, 59.3961f, 1.0f));
    controlPoints1_5.push_back(glm::vec4(-49.7859f, 0.16f, 50.4402f, 1.0f));
    std::vector<glm::vec4> controlPoints1_6;
    controlPoints1_6.push_back(glm::vec4(-49.7859f, 0.16f, 50.4402f, 1.0f));
    controlPoints1_6.push_back(glm::vec4(-54.4495f, 0.16f, 20.2291f, 1.0f));
    controlPoints1_6.push_back(glm::vec4(-66.0504, 0.16f, -13.1143f, 1.0f));
    controlPoints1_6.push_back(glm::vec4(3.0f,0.16f,2.0f,1.0f));
    // bezier control points2

    std::vector<glm::vec4> controlPoints2_1;
    controlPoints2_1.push_back(oldPos2);
    controlPoints2_1.push_back(glm::vec4(156.483f, 0.16f, -3.52993f, 1.0f));
    controlPoints2_1.push_back(glm::vec4(134.85f, 0.16f, -6.45085f, 1.0f));
    controlPoints2_1.push_back(glm::vec4(132.0f, 0.16f, 60.9768f, 1.0f));

    std::vector<glm::vec4> controlPoints2_2;
    controlPoints2_2.push_back(glm::vec4(132.0f, 0.16f, 60.9768f, 1.0f));
    controlPoints2_2.push_back(glm::vec4(99.3377f, 0.16f, 71.0411f, 1.0f));
    controlPoints2_2.push_back(glm::vec4(121.338f, 0.16f, 30.559f, 1.0f));
    controlPoints2_2.push_back(glm::vec4(69.6432f, 0.16f, 43.1186f, 1.0f));

    std::vector<glm::vec4> controlPoints2_3;
    controlPoints2_3.push_back(glm::vec4(69.6432f, 0.16f, 43.1186f, 1.0f));
    controlPoints2_3.push_back(glm::vec4(23.2552f, 0.16f, 32.2232f, 1.0f));
    controlPoints2_3.push_back(glm::vec4(24.7635f, 0.16f, 47.7012f, 1.0f));
    controlPoints2_3.push_back(glm::vec4(28.857f, 0.16f, 105.02f, 1.0f));

    std::vector<glm::vec4> controlPoints2_4;
    controlPoints2_4.push_back(glm::vec4(28.857f, 0.16f, 105.02f, 1.0f));
    controlPoints2_4.push_back(glm::vec4(10.3733f, 0.16f, 123.0f, 1.0f));
    controlPoints2_4.push_back(glm::vec4(5.2825f, 0.16f, 124.649f, 1.0f));
    controlPoints2_4.push_back(glm::vec4(2.76479f, 0.16f, 69.1178f, 1.0f));

    std::vector<glm::vec4> controlPoints2_5;
    controlPoints2_5.push_back(glm::vec4(2.76479f, 0.16f, 69.1178f, 1.0f));
    controlPoints2_5.push_back(glm::vec4(5.32203f, 0.16f, 51.0035f, 1.0f));
    controlPoints2_5.push_back(glm::vec4(-53.3087f, 0.16f, 59.3961f, 1.0f));
    controlPoints2_5.push_back(glm::vec4(-52.7859f, 0.16f, 50.4402f, 1.0f));

    std::vector<glm::vec4> controlPoints2_6;
    controlPoints2_6.push_back(glm::vec4(-52.7859f, 0.16f, 50.4402f, 1.0f));
    controlPoints2_6.push_back(glm::vec4(-54.4495f, 0.16f, 20.2291f, 1.0f));
    controlPoints2_6.push_back(glm::vec4(-66.0504, 0.16f, -6.89044f, 1.0f));
    controlPoints2_6.push_back(glm::vec4(3.0f,0.16f,-2.0f,1.0f));

    //decor model


    bool raceStart = false;
    bool lost = false;
    bool checkpoint = false;
    bool finished=false;
    bool updateCamPos;

    //car lateral movement

    glm::vec4 lateral_velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    glm::vec4 carLeft = crossproduct(up_vector, carForward);
    glm::vec4 carRight = -carLeft;

    while (!glfwWindowShouldClose(window))
    {
        glDisable(GL_CULL_FACE);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glUseProgram(program_id);


        glm::mat4 view;
        float current_time = (float)glfwGetTime();
        delta_t = current_time - prev_time;
        prev_time = current_time;
        glm::vec4 camera_position_c;
        if (camType < 2)
        {
            updateCamPos=true;
            float r = g_CameraDistance;
            float y = r * sin(g_CameraPhi);
            float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
            float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);
            c = glm::vec4(x, y, z, 1.0f);
            //cameras dinamica e lookat
            if (camType == 0)
            {
                camera_position_c = Matrix_Translate(-carForward.x*6 /(1+norm(current_velocity)*0.04), 2/(1+norm(current_velocity)*0.05f), -carForward.z*6/(1+norm(current_velocity)*0.04)) * carPos;
            }
            else if (camType == 1)
            {
                camera_position_c = Matrix_Translate(carPos.x, carPos.y, carPos.z) * c;
            }
            glm::vec4 camera_lookat_l = carPos;
            glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c;
            glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);


            view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        }else{
            //camera livre
            if(updateCamPos){
                updateCamPos=false;
                c=Matrix_Translate(-carForward.x,-carForward.y,carForward.z)*carPos;
            }
            float vy = sin(g_CameraPhi);
            float vz = cos(g_CameraPhi)*cos(g_CameraTheta);
            float vx = cos(g_CameraPhi)*sin(g_CameraTheta);
            glm::vec4 camera_view_vector = glm::vec4(vx,vy,vz,0.0f);
            glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);
            glm::vec4 w =-camera_view_vector/norm(camera_view_vector);
            glm::vec4 intermediate=crossproduct(camera_up_vector,camera_view_vector);
            glm::vec4 u = intermediate/norm(intermediate);
            if(wPressed){
                c+= -w*camSpeed*delta_t;
            }
            if(aPressed){
                c+= u*camSpeed*delta_t;
            }

            if(sPressed){
                c+= w*camSpeed*delta_t;
            }
            if(dPressed){
                c+= -u*camSpeed*delta_t;
            }
            if(spacePressed){
                c+= -crossproduct(w,u)/norm(crossproduct(w,u))*camSpeed*delta_t;
            }
            if(ctrlPressed){
                c+= crossproduct(w,u)/norm(crossproduct(w,u))*camSpeed*delta_t;
            }
            camera_position_c=c;
            view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
            }
        glm::mat4 projection;


        float nearplane = -0.1f;
        float farplane = -200.0f;
        float field_of_view = (PI / 3.0f)-(norm(current_velocity)*0.002f);
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

#define BLUE_FALCON 0
#define PLANE 1
#define OPPONENT 2
#define SPHERE 3
#define DECOR 4
#define START 5
        //skysphere + decoracoes implementadas desenhando primeiro e limpando o zbuffer
        glm::mat4 modelSkybox=Matrix_Translate(camera_position_c.x,camera_position_c.y,camera_position_c.z)*Matrix_Scale(3.0f,3.0f,3.0f)*Matrix_Identity();
        glm::mat4 modelDecor = Matrix_Translate(camera_position_c.x+0.6f,camera_position_c.y+0.05f,camera_position_c.z-0.01f)*Matrix_Rotate_Z(PI/8)*Matrix_Rotate_Y(PI/2)*Matrix_Identity();
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(modelSkybox));
        glUniform1i(object_id_uniform, SPHERE);
        DrawVirtualObject("sphere");

        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(modelDecor));
        glUniform1i(object_id_uniform, DECOR);
        DrawVirtualObject("decor");
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
          auto reset = [&]() 
        { 
            carForward = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            current_velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            carPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            oldPos1 = glm::vec4(0.0f, 0.16f, 2.0f, 1.0f);
            oldPos2 = glm::vec4(0.0f, 0.16f, -2.0f, 1.0f);
            modelPlayer = Matrix_Identity();
            modelPlayer = Matrix_Translate(carPos.x, carPos.y, carPos.z) * modelPlayer;
            modelPlayer = Matrix_Rotate_Y(3.141592 / 2) * modelPlayer;
            modelOponnent1 = Matrix_Identity();
            modelOponnent1 = Matrix_Scale(0.0012, 0.0012, 0.0012) * modelOponnent1;
            modelOponnent1 = Matrix_Rotate_Y(3.141592 / 2) * modelOponnent1;
            modelOponnent1 = Matrix_Translate(oldPos1.x, oldPos1.y, oldPos1.z) * modelOponnent1;
            opponnent1forward = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            opponnent1pos = glm::vec4(oldPos1.x, oldPos1.y, oldPos1.z, 1.0f);

            modelOponnent2 = Matrix_Identity();
            modelOponnent2 = Matrix_Scale(0.0012, 0.0012, 0.0012) * modelOponnent2;
            modelOponnent2 = Matrix_Rotate_Y(3.141592 / 2) * modelOponnent2;
            modelOponnent2 = Matrix_Translate(oldPos2.x, oldPos2.y, oldPos2.z) * modelOponnent2;
            opponnent2forward = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            opponnent2pos = glm::vec4(oldPos2.x, oldPos2.y, oldPos2.z, 1.0f); 
            lost=false;
            checkpoint=false;
            finished=false;
        };
        if (!raceStart&&startPressed)
        {//restart race
            glfwSetTime(0);
            raceStart = true;
            prev_time=0;
            reset();
        }
        if(raceStart){
        // definição dos controles do player e modelo de fisica
        current_velocity -= friction * delta_t * current_velocity;
        if(camType<2){
        if (wPressed)
        {
            acceleration += max_velocity * carForward * delta_t;
        }
        if (aPressed)
        {
            float velocity_norm = norm(current_velocity);
            float rotation = 2.0f;
            if (velocity_norm > 0)
            {
                rotation = std::min(std::max(max_velocity / norm(current_velocity), 0.5f), 2.0f);
            }
            carForward = Matrix_Rotate_Y(rotation * delta_t) * carForward;
            modelPlayer = Matrix_Translate(carPos.x, carPos.y, carPos.z) * Matrix_Rotate_Y(rotation * delta_t) * Matrix_Translate(-carPos.x, -carPos.y, -carPos.z) * modelPlayer;
        }

        if (sPressed)
        {
            acceleration -= max_velocity * carForward * delta_t;
        }
        if (dPressed)
        {
            float velocity_norm = norm(current_velocity);
            float rotation = 2.0f;
            if (velocity_norm > 0)
            {
                rotation = std::min(std::max(max_velocity / norm(current_velocity), 0.5f), 2.0f);
            }
            carForward = Matrix_Rotate_Y(-rotation * delta_t) * carForward;
            modelPlayer = Matrix_Translate(carPos.x, carPos.y, carPos.z) * Matrix_Rotate_Y(-rotation * delta_t) * Matrix_Translate(-carPos.x, -carPos.y, -carPos.z) * modelPlayer;
        }

        lateral_velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        carLeft = crossproduct(up_vector, carForward);
        carRight = -carLeft;
        if(g_LeftMouseButtonPressed){
            lateral_velocity += carLeft * max_velocity * 30.0f * delta_t;
        }
        if(g_RightMouseButtonPressed){
            lateral_velocity += carRight * max_velocity * 30.0f * delta_t;
        }

        }

        if (norm(acceleration) == 0 && norm(current_velocity) < 0.1)
        {
            current_velocity *= 0;
        }

        if (dotproduct(current_velocity, carForward) < 0)
        {
            current_velocity = norm(current_velocity) * -carForward + acceleration;
        }
        else
        {
            current_velocity = norm(current_velocity) * carForward + acceleration;
        }

        frame_movement = (current_velocity + lateral_velocity) * delta_t;
        modelPlayer = Matrix_Translate(frame_movement.x, frame_movement.y, frame_movement.z) * modelPlayer;
        carPos += frame_movement;
        acceleration *= 0;


        // comportamento dos oponentes
        float bezierTime1 = current_time / 5;
        float bezierTime2 = current_time / 8;
        // oponnent 1

        modelOponnent1 = opponentMovement(modelOponnent1, bezierTime1, controlPoints1_1, controlPoints1_2,controlPoints1_3,controlPoints1_4,controlPoints1_5,controlPoints1_6, 3, opponnent1forward, opponnent1pos, oldPos1);


        // oponnent 2

        modelOponnent2 = opponentMovement(modelOponnent2, bezierTime2, controlPoints2_1, controlPoints2_2,controlPoints2_3,controlPoints2_4,controlPoints2_5,controlPoints2_6, 3, opponnent2forward, opponnent2pos, oldPos2);

        }
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(modelPlayer));
        glUniform1i(object_id_uniform, BLUE_FALCON);
        DrawVirtualObject("blue_falcon");
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(modelOponnent1));
        glUniform1i(object_id_uniform, OPPONENT);
        DrawVirtualObject("opponent");
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(modelOponnent2));
        glUniform1i(object_id_uniform, OPPONENT);
        DrawVirtualObject("opponent");
        // Pista
        glm::mat4 model = Matrix_Identity();
        model = Matrix_Rotate_Y(-PI / 2) * model;
        model = Matrix_Scale(8.0f, 8.0f, 8.0f) * model;
        model = Matrix_Translate(0.0f, -0.8f, 0.0f) * model;
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, PLANE);
        DrawVirtualObject("Track");
        model = Matrix_Identity();
        model = Matrix_Rotate_Y(-PI / 2) * model;
        model = Matrix_Scale(1.0f, 1.0f, 1.0f) * model;
        model = Matrix_Translate(2.0f, 1.0f, 0.0f) * model;
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, START);
        DrawVirtualObject("Starting_Line");


        //win/lose logic
        if(current_time>30 && raceStart && !finished){
            lost=true;
        }
        if(false/*colisao com checkpoint*/){
            checkpoint=true;
        }
        if(false/*colisao com final*/){
            if(checkpoint){
                finished=true;
            }
        }
        float pad = TextRendering_LineHeight(window);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if(finished && !lost){
            TextRendering_PrintString(window, "You Win, Press Enter to Restart", -1.0f+pad/10, -1.0f+2*pad/10,1.0f);
        }else if(finished && lost){
            TextRendering_PrintString(window, "You Lost, Press Enter to Restart",-1.0f+pad/10, -1.0f+2*pad/10,1.0f);
        }
        
        
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}


// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char *filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if (data == NULL)
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    // glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
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

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char *object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void *)(g_VirtualScene[object_name].first_index * sizeof(GLuint)));

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if (program_id != 0)
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform = glGetUniformLocation(program_id, "model");           // Variável da matriz "model"
    view_uniform = glGetUniformLocation(program_id, "view");             // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform = glGetUniformLocation(program_id, "object_id");   // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform = glGetUniformLocation(program_id, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4 &M)
{
    if (g_MatrixStack.empty())
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel *model)
{
    if (!model->attrib.normals.empty())
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4 vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx, vy, vz, 1.0);
            }

            const glm::vec4 a = vertices[0];
            const glm::vec4 b = vertices[1];
            const glm::vec4 c = vertices[2];

            const glm::vec4 n = crossproduct(b - a, c - a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3 * triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize(3 * num_vertices);

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3 * i + 0] = n.x;
        model->attrib.normals[3 * i + 1] = n.y;
        model->attrib.normals[3 * i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel *model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float> model_coefficients;
    std::vector<float> normal_coefficients;
    std::vector<float> texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval, maxval, maxval);
        glm::vec3 bbox_max = glm::vec3(minval, minval, minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];

                indices.push_back(first_index + 3 * triangle + vertex);

                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                // printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back(vx);   // X
                model_coefficients.push_back(vy);   // Y
                model_coefficients.push_back(vz);   // Z
                model_coefficients.push_back(1.0f); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if (idx.normal_index != -1)
                {
                    const float nx = model->attrib.normals[3 * idx.normal_index + 0];
                    const float ny = model->attrib.normals[3 * idx.normal_index + 1];
                    const float nz = model->attrib.normals[3 * idx.normal_index + 2];
                    normal_coefficients.push_back(nx);   // X
                    normal_coefficients.push_back(ny);   // Y
                    normal_coefficients.push_back(nz);   // Z
                    normal_coefficients.push_back(0.0f); // W
                }

                if (idx.texcoord_index != -1)
                {
                    const float u = model->attrib.texcoords[2 * idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2 * idx.texcoord_index + 1];
                    texture_coefficients.push_back(u);
                    texture_coefficients.push_back(v);
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name = model->shapes[shape].name;
        theobject.first_index = first_index;                  // Primeiro índice
        theobject.num_indices = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;              // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
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
    GLuint location = 0;            // "(location = 0)" em "shader_vertex.glsl"
    GLint number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!normal_coefficients.empty())
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (!texture_coefficients.empty())
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char *filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try
    {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar *shader_string = str.c_str();
    const GLint shader_string_length = static_cast<GLint>(str.length());

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar *log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if (log_length != 0)
    {
        std::string output;

        if (!compiled_ok)
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

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete[] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if (linked_ok == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar *log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete[] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f * dx;
        g_CameraPhi -= 0.01f * dy;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f / 2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f * yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod)
{
    // ================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Se o usuário apertar a tecla C,trocamos a camera.
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        camType++;
        if (camType > 2)
        {
            camType = 0;
        }
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R&& action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout, "Shaders recarregados!\n");
        fflush(stdout);
    }

    if (key == GLFW_KEY_W)
    {
        if (action == GLFW_PRESS)
        {
            wPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            wPressed = false;
        }
        else if (action == GLFW_REPEAT)
        {
        }
    }
    if (key == GLFW_KEY_A)
    {
        if (action == GLFW_PRESS)
        {
            aPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            aPressed = false;
        }
        else if (action == GLFW_REPEAT)
        {
        }
    }
    if (key == GLFW_KEY_S)
    {
        if (action == GLFW_PRESS)
        {
            sPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            sPressed = false;
        }
        else if (action == GLFW_REPEAT)
        {
        }
    }
    if (key == GLFW_KEY_D)
    {
        if (action == GLFW_PRESS)
        {
            dPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            dPressed = false;
        }
        else if (action == GLFW_REPEAT)
        {
        }
    }
    if (key == GLFW_KEY_SPACE)
    {
        if (action == GLFW_PRESS)
        {
            spacePressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            spacePressed = false;
        }
        else if (action == GLFW_REPEAT)
        {
        }
    }
    if (key == GLFW_KEY_LEFT_CONTROL)
    {
        if (action == GLFW_PRESS)
        {
            ctrlPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            ctrlPressed = false;
        }
        else if (action == GLFW_REPEAT)
        {
        }
    }
    if (key == GLFW_KEY_ENTER)
    {
        if (action == GLFW_PRESS)
        {
            startPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            startPressed = false;
        }
        else if (action == GLFW_REPEAT)
        {
        }
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char *description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

