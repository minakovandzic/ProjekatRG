#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <cstdlib>
#include <cmath>

#include <random>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

unsigned int loadTexture(const char *path);
unsigned int loadCubemap(vector<std::string> faces);

void foxMovement(Camera_Movement direction);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

bool ssaoButton=false;
bool lightButton = false;
bool blinn = false;
bool blinnKeyPressed = false;
bool flashLight = false;

Camera camera(glm::vec3(0.0f, 3.0f, 15.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
//positions
glm::vec3 barnPosition(0.0f, 0.0f, 0.0f);
glm::vec3 foxLocation(0.0f,0.0f,7.0f);


unsigned int b1 = 2;
unsigned int b2 = 4;
unsigned int b3 = 3;
unsigned int b4 = 5;
unsigned int b5 = 2;


bool on_left=false,on_right=false,front=false,back=true;//fox's rotation
bool showAnimals=false;

void renderCube();
void renderQuad();
float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Project", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    //SSAO shader
    Shader shaderGeometryPass("resources/shaders/ssao_geometry.vs", "resources/shaders/ssao_geometry.fs");
    Shader shaderLightingPass("resources/shaders/ssao.vs", "resources/shaders/ssao_lighting.fs");
    Shader shaderSSAO("resources/shaders/ssao.vs", "resources/shaders/ssao.fs");
    Shader shaderSSAOBlur("resources/shaders/ssao.vs", "resources/shaders/ssao_blur.fs");
    //shaders
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader lightShader("resources/shaders/light.vs", "resources/shaders/light.fs");
    Shader blending("resources/shaders/blending.vs","resources/shaders/blending.fs");
    // set up vertex data (and buffer(s)) and configure vertex attrib
    // floor plain coordinates
    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    float floorVertices[] = {
            // positions          // normals          // texture coords
            0.5f,  0.5f,  0.0f,  0.0f, 0.0f, -1.0f,  1.0f,  1.0f,  // top right
            0.5f, -0.5f,  0.0f,  0.0f, 0.0f, -1.0f,  1.0f,  0.0f,  // bottom right
            -0.5f, -0.5f,  0.0f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f,  // bottom left
            -0.5f,  0.5f,  0.0f,  0.0f, 0.0f, -1.0f,  0.0f,  1.0f   // top left
    };

    // floor vertices for use in EBO
    unsigned int floorIndices[] = {
            0, 1, 3,  // first Triangle
            1, 2, 3   // second Triangle
    };
    // Floor setup
    unsigned int floorVAO, floorVBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    // skybox coordinates
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // Skybox setup
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);


    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/grass.png").c_str());
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/ground1/coast_sand_rocks_02_diff_4k.jpg").c_str());
    unsigned int specularMap = loadTexture(FileSystem::getPath("resources/textures/ground1/coast_sand_rocks_02_arm_4k.jpg").c_str());
    lightShader.use();
    lightShader.setInt("material.diffuse", 0);
    lightShader.setInt("material.specular", 1);
    lightShader.setInt("material.specular2", 2);

    // floor textures
    unsigned int floorDiffuseMap = loadTexture(FileSystem::getPath("resources/textures/ground1/coast_sand_rocks_02_diff_4k.jpg").c_str());
    unsigned int floorSpecularMap = specularMap;
    vector<glm::vec3> grass
            {
                    glm::vec3(-12.1f, 2.0f, -6.5f),
                    glm::vec3(-3.0f, 2.0f, 5.5f),
                    glm::vec3(3.5f, 2.0f, 5.5f),
                    glm::vec3(13.0f, 2.0f, 5.5f),
                    glm::vec3(13.0f, 2.0f, 6.5f),
                    glm::vec3(13.5f, 2.0f, 6.5f),
                    glm::vec3(-13.5f, 2.0f, 3.5f),
            };
    blending.use();
    blending.setInt("texture1",0);

    vector<std::string> skyboxSides = {
            FileSystem::getPath("resources/textures/skybox3/px(1).jpg"),
            FileSystem::getPath("resources/textures/skybox3/nx(1).jpg"),
            FileSystem::getPath("resources/textures/skybox3/py(1).jpg"),
            FileSystem::getPath("resources/textures/skybox3/ny(1).jpg"),
            FileSystem::getPath("resources/textures/skybox3/pz(1).jpg"),
            FileSystem::getPath("resources/textures/skybox3/nz(1).jpg")
    };
    unsigned int cubemapTexture = loadCubemap(skyboxSides);
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);


    Model barn(FileSystem::getPath("resources/objects/barn/Rbarn15.obj"));
    barn.SetShaderTextureNamePrefix("material.");

    Model bird1(FileSystem::getPath("resources/objects/bird1/12250_Bird_v1_L3.obj"));
    bird1.SetShaderTextureNamePrefix("material.");

    Model bird2(FileSystem::getPath("resources/objects/bird2/12248_Bird_v1_L2.obj"));
    bird2.SetShaderTextureNamePrefix("material.");

    Model bird3(FileSystem::getPath("resources/objects/bird3/12256_canadiangoos_v1_l3.obj"));
    bird3.SetShaderTextureNamePrefix("material.");

    Model bird4(FileSystem::getPath("resources/objects/bird4/Chick.obj"));
    bird4.SetShaderTextureNamePrefix("material.");

    Model bird5(FileSystem::getPath("resources/objects/bird5/12249_Bird_v1_L2.obj"));
    bird5.SetShaderTextureNamePrefix("material.");

    Model fox(FileSystem::getPath("resources/objects/fox/13577_Tibetan_Hill_Fox_v1_L3.obj"));
    fox.SetShaderTextureNamePrefix("material.");

    Model house(FileSystem::getPath("resources/objects/cottage/cottage.obj"));
    house.SetShaderTextureNamePrefix("material.");

    Model cat(FileSystem::getPath("resources/objects/cat/12221_Cat_v1_l3.obj"));
    cat.SetShaderTextureNamePrefix("material.");

    Model silos(FileSystem::getPath("resources/objects/silos/20954_Farm_Silo_v1_NEW.obj"));
    silos.SetShaderTextureNamePrefix("material.");


    float time=glfwGetTime();

    //bird1
    glm::vec3 bird1Positions[] = {
            glm::vec3(-6.0f, 0.0f, -2.5f),
            glm::vec3(7.5f, 0.0f, 3.0f)
    };
    glm::mat4* modelMatricesBird1;
    modelMatricesBird1 = new glm::mat4[b1];

    for (unsigned int i = 0; i < b1; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bird1Positions[i]);
        model = glm::scale(model, glm::vec3(0.09,0.09,0.09));
        model = glm::rotate(model, glm::radians(-90.0f),glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatricesBird1[i] = model;
    }

    //bird2
    glm::vec3 bird2Positions[] = {
            glm::vec3(-6.0f, 0.0f, 4.3f),
            glm::vec3(-8.0f, 0.0f, 5.7f),
            glm::vec3(3.8f, 0.0f, -3.8f),
            glm::vec3(5.0f, 0.0f, -6.9f)
    };
    glm::mat4* modelMatricesBird2;
    modelMatricesBird2 = new glm::mat4[b2];

    for (unsigned int i = 0; i < b2; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bird2Positions[i]);
        model = glm::scale(model, glm::vec3(0.025,0.025,0.025));
        model = glm::rotate(model, glm::radians(-90.0f),glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatricesBird2[i] = model;
    }

    //bird3
    glm::vec3 bird3Positions[] = {
            glm::vec3(-4.4f, 0.0f, 2.0f),
            glm::vec3(5.0f, 0.0f, 8.0f),
            glm::vec3(7.0f, 0.0f, -3.0f),
    };
    glm::mat4* modelMatricesBird3;
    modelMatricesBird3 = new glm::mat4[b3];

    for (unsigned int i = 0; i < b3; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bird3Positions[i]);
        model = glm::scale(model, glm::vec3(0.03,0.03,0.03));
        model = glm::rotate(model, glm::radians(-90.0f),glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-60.0f),glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatricesBird3[i] = model;
    }

    //bird4
    glm::vec3 bird4Positions[] = {
            glm::vec3(-8.0f, 0.0f, 7.0f),
            glm::vec3(3.5f, 0.0f, 10.0f),
            glm::vec3(10.0f, 0.0f, 8.0f),
            glm::vec3(-9.0f, 0.0f, 4.0f),
            glm::vec3(1.0f, 0.0f, 6.0f)
    };
    glm::mat4* modelMatricesBird4;
    modelMatricesBird4 = new glm::mat4[b4];

    for (unsigned int i = 0; i < b4; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bird4Positions[i]);
        model = glm::scale(model, glm::vec3(0.005,0.005,0.005));
        model = glm::rotate(model, glm::radians(-90.0f),glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatricesBird4[i] = model;
    }

    //bird5
    glm::vec3 bird5Positions[] = {
            glm::vec3(5.8f, 0.0f, 4.0f),
            glm::vec3(-6.7f, 0.0f, 8.0f),
    };
    glm::mat4* modelMatricesBird5;
    modelMatricesBird5 = new glm::mat4[b5];

    for (unsigned int i = 0; i < b5; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, bird5Positions[i]);
        model = glm::scale(model, glm::vec3(0.03,0.03,0.03));
        model = glm::rotate(model, glm::radians(-90.0f),glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatricesBird5[i] = model;
    }

    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedo;
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedo);
    glBindTexture(GL_TEXTURE_2D, gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // also create framebuffer to hold SSAO processing stage
    // -----------------------------------------------------
    unsigned int ssaoFBO, ssaoBlurFBO;
    glGenFramebuffers(1, &ssaoFBO);  glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    unsigned int ssaoColorBuffer, ssaoColorBufferBlur;
    // SSAO color buffer
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    // and blur stage
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0;

        // scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }
    // generate noise texture
    // ----------------------
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
        ssaoNoise.push_back(noise);
    }
    unsigned int noiseTexture; glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // lighting info
    // -------------
    glm::vec3 lightPos = glm::vec3(2.0, 4.0, -2.0);
    glm::vec3 lightColor = glm::vec3(0.86, 0.3f, 0.2f);

    // shader configuration
    // --------------------
    shaderLightingPass.use();
    shaderLightingPass.setInt("gPosition", 0);
    shaderLightingPass.setInt("gNormal", 1);
    shaderLightingPass.setInt("gAlbedo", 2);
    shaderLightingPass.setInt("ssao", 3);
    shaderSSAO.use();
    shaderSSAO.setInt("gPosition", 0);
    shaderSSAO.setInt("gNormal", 1);
    shaderSSAO.setInt("texNoise", 2);
    shaderSSAOBlur.use();
    shaderSSAOBlur.setInt("ssaoInput", 0);


    while (!glfwWindowShouldClose(window))
    {
        auto currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float time = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(ssaoButton==true){
            // 1. geometry pass: render scene's geometry/color data into gbuffer
            // -----------------------------------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT,
                                                    0.1f, 100.0f);
            glm::mat4 view = camera.GetViewMatrix();
            glm::mat4 model = glm::mat4(1.0f);
            shaderGeometryPass.use();
            shaderGeometryPass.setMat4("projection", projection);
            shaderGeometryPass.setMat4("view", view);

            // room cube
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0, 30.0f, 0.0f));
            model = glm::scale(model, glm::vec3(30.0f));
            shaderGeometryPass.setMat4("model", model);
            shaderGeometryPass.setInt("invertedNormals", 1); // invert normals as we're inside the cube
            renderCube();
            shaderGeometryPass.setInt("invertedNormals", 0);

            // barn
            model = glm::mat4(1.0f);
            model = glm::translate(model, barnPosition);
            model = glm::scale(model, glm::vec3(0.006f));
            shaderGeometryPass.setMat4("model", model);
            barn.Draw(shaderGeometryPass);

            //house
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-20., 0, -5));
            model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(1.5f));
            shaderGeometryPass.setMat4("model", model);
            house.Draw(shaderGeometryPass);

            //cat
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-12, 0, -2));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(0.04f));
            shaderGeometryPass.setMat4("model", model);
            cat.Draw(shaderGeometryPass);

            //silos
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(14, 0, -5));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(-120.0f), glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(2.0f));
            shaderGeometryPass.setMat4("model", model);
            silos.Draw(shaderGeometryPass);

            //fox
            model = glm::mat4(1.0f);
            model = glm::translate(model, foxLocation);
            if (back)
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
            if (front) {
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
                model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));
            }
            if (on_left) {
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 0.0, 1.0));
            }
            if (on_right) {
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
            }
            //model = glm::rotate(model, glm::radians(-90.0f)?, glm::vec3(1.0, 0.0, 0.0));
            model = glm::scale(model, glm::vec3(0.025f));
            shaderGeometryPass.setMat4("model", model);
            fox.Draw(shaderGeometryPass);

            if (showAnimals) {
                for (unsigned int i = 0; i < b1; i++) {
                    shaderGeometryPass.setMat4("model", modelMatricesBird1[i]);
                    bird1.Draw(shaderGeometryPass);
                }
                for (unsigned int i = 0; i < b2; i++) {
                    shaderGeometryPass.setMat4("model", modelMatricesBird2[i]);
                    bird2.Draw(shaderGeometryPass);
                }
                for (unsigned int i = 0; i < b3; i++) {
                    shaderGeometryPass.setMat4("model", modelMatricesBird3[i]);
                    bird3.Draw(shaderGeometryPass);
                }
                for (unsigned int i = 0; i < b4; i++) {
                    shaderGeometryPass.setMat4("model", modelMatricesBird4[i]);
                    bird4.Draw(shaderGeometryPass);
                }
                for (unsigned int i = 0; i < b5; i++) {
                    shaderGeometryPass.setMat4("model", modelMatricesBird5[i]);
                    bird5.Draw(shaderGeometryPass);
                }
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 2. generate SSAO texture
            // ------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderSSAO.use();
            // Send kernel + rotation
            for (unsigned int i = 0; i < 64; ++i)
                shaderSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
            shaderSSAO.setMat4("projection", projection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, noiseTexture);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);


            // 3. blur SSAO texture to remove noise
            // ------------------------------------
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
            glClear(GL_COLOR_BUFFER_BIT);
            shaderSSAOBlur.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
            renderQuad();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);


            // 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
            // -----------------------------------------------------------------------------------------------------
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            shaderLightingPass.use();
            // send light relevant uniforms
            glm::vec3 lightPosView = glm::vec3(camera.GetViewMatrix() * glm::vec4(lightPos, 1.0));
            shaderLightingPass.setVec3("light.Position", lightPosView);
            shaderLightingPass.setVec3("light.Color", lightColor);
            // Update attenuation parameters
            const float linear = 0.09;
            const float quadratic = 0.032;
            shaderLightingPass.setFloat("light.Linear", linear);
            shaderLightingPass.setFloat("light.Quadratic", quadratic);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gAlbedo);
            glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
            glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
            renderQuad();
        }
        else {
            // lightshowShader setup
            lightShader.use();
            lightShader.setVec3("viewPos", camera.Position);
            lightShader.setFloat("material.shininess", 16.0f);
            lightShader.setInt("blinn", blinn);
            lightShader.setInt("flashLight", flashLight);
            lightShader.setVec3("pointLightColor", 1, 1,0);
            // directional light setup
            lightShader.setVec3("dirLight.direction", 1.0f, -0.5f, 0.0f);
            lightShader.setVec3("dirLight.ambient", 0.5f, 0.5f, 0.5f);
            lightShader.setVec3("dirLight.diffuse", 0.1f, 0.5f, 0.f);
            lightShader.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

            // point light setup
            lightShader.setVec3("pointLight.position", barnPosition);
            lightShader.setVec3("pointLight.ambient", 0.01f, 0.01f, 0.01f);
            lightShader.setVec3("pointLight.diffuse", 0.8f, 0.8f, 0.8f);
            lightShader.setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
            lightShader.setFloat("pointLight.constant", 1.0f);
            lightShader.setFloat("pointLight.linear", 0.05f);
            lightShader.setFloat("pointLight.quadratic", 0.012f);


            // spotlight setup
            lightShader.setVec3("spotLight.position", glm::vec3(0.0f, 20.0f, 0.0f));
            lightShader.setVec3("spotLight.direction", glm::vec3(0, -1, 0));
            lightShader.setVec3("spotLight.ambient", 0.5f, 0.5f, 0.0f);
            lightShader.setVec3("spotLight.diffuse", 0.5f, 0.5f, 0.0f);
            lightShader.setVec3("spotLight.specular", 0.5f, 0.5f, 0.0f);
            lightShader.setFloat("spotLight.constant", 1.0f);
            lightShader.setFloat("spotLight.linear", 0.005);
            lightShader.setFloat("spotLight.quadratic", 0.0002);
            lightShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(16.5f)));
            lightShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(22.0f)));

            // view/projection transformations
            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT,
                                                    0.1f, 100.0f);
            glm::mat4 view = camera.GetViewMatrix();
            lightShader.setMat4("projection", projection);
            lightShader.setMat4("view", view);

            glm::mat4 model(1.0f);
            if (showAnimals) {
                for (unsigned int i = 0; i < b1; i++) {
                    lightShader.setMat4("model", modelMatricesBird1[i]);
                    bird1.Draw(lightShader);
                }
                for (unsigned int i = 0; i < b2; i++) {
                    lightShader.setMat4("model", modelMatricesBird2[i]);
                    bird2.Draw(lightShader);
                }
                for (unsigned int i = 0; i < b3; i++) {
                    lightShader.setMat4("model", modelMatricesBird3[i]);
                    bird3.Draw(lightShader);
                }
                for (unsigned int i = 0; i < b4; i++) {
                    lightShader.setMat4("model", modelMatricesBird4[i]);
                    bird4.Draw(lightShader);
                }
                for (unsigned int i = 0; i < b5; i++) {
                    lightShader.setMat4("model", modelMatricesBird5[i]);
                    bird5.Draw(lightShader);
                }
            }
            // barn setup
            // material properties
            lightShader.setFloat("material.shininess", 1.0f);
            // world transformations
            model = glm::mat4(1.0f);
            model = glm::translate(model, barnPosition);
            //model = glm::rotate(model, time / 4, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.006f));
            lightShader.setMat4("model", model);
            // render barn
            barn.Draw(lightShader);

            // house
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-20.0f, 0.0f, -5.0f));
            model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(0, 1, 0));
            //model = glm::rotate(model, glm::radians(-65.0f), glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(1.5f));
            lightShader.setMat4("model", model);
            house.Draw(lightShader);

            //silos
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(14, 0, -5));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(-120.0f), glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(2.0f));
            lightShader.setMat4("model", model);
            silos.Draw(lightShader);

            //cat
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-12, 0, -2));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(0.04f));
            lightShader.setMat4("model", model);
            cat.Draw(lightShader);


            //fox
            model = glm::mat4(1.0f);

            model = glm::translate(model, foxLocation);
            if (back)
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
            if (front) {
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
                model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));
            }
            if (on_left) {
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0, 0.0, 1.0));
            }
            if (on_right) {
                model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
            }

            model = glm::scale(model, glm::vec3(0.025));
            lightShader.setMat4("model", model);

            fox.Draw(lightShader);
            // floor setup
            // light properties
            lightShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

            // material properties
            lightShader.setFloat("material.shininess", 10.0f);

            // world transformation
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -0.51f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(500.0f));
            lightShader.setMat4("model", model);

            // bind diffuse map
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, floorDiffuseMap);

            // bind specular map
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, floorSpecularMap);

            // render floor
            glBindVertexArray(floorVAO);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            glDisable(GL_CULL_FACE);


            // skybox shader setup
            // -----------
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
            skyboxShader.use();
            view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
            skyboxShader.setMat4("view", view);
            skyboxShader.setMat4("projection", projection);

            // render skybox cube
            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

            blending.use();
            projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f,
                                          100.0f);
            view = camera.GetViewMatrix();
            model = glm::mat4(1.0f);
            blending.setMat4("projection", projection);
            blending.setMat4("view", view);
            glBindVertexArray(transparentVAO);
            glBindTexture(GL_TEXTURE_2D, transparentTexture);
            for (unsigned int i = 0; i < grass.size(); i++) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, grass[i]);
                model = glm::scale(model, glm::vec3(3.0f, 5.0f, 3.0f));
                blending.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteBuffers(1, &floorEBO);
    glDeleteBuffers(1, &skyboxVBO);

    // destroy all remaining windows/cursors, free any allocated resources
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        front = true;
        back = on_right = on_left = false;
        foxMovement(FORWARD);
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        back = true;
        front = on_left = on_right = false;
        foxMovement(BACKWARD);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        on_left = true;
        on_right = front = back = false;
        foxMovement(LEFT);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        on_right=true;
        on_left=front=back=false;
        foxMovement(RIGHT);
    }
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        showAnimals=true;
        flashLight=false;
    }

    // switch Blinn-Phong lighting model on/off
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !blinnKeyPressed)
    {
        blinn = !blinn;
        blinnKeyPressed = true;
        if (blinn)
            cout << "Blinn-Phong" << endl;
        else
            cout << "Phong" << endl;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        blinnKeyPressed = false;
    }

    // flash on/off
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !lightButton)
    {
        flashLight = !flashLight;
        lightButton=true;
        if(flashLight)
            cout << "Alarm is on!" << endl;
        else
            cout << "Alarm is off!" << endl;
    }
    if(glfwGetKey(window, GLFW_KEY_O) == GLFW_RELEASE){
        lightButton=false;
    }

    if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS){
        ssaoButton=true;
    }
    if(glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE){
        ssaoButton=false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; // reversed since y-coordinates go from bottom to top

    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RED;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void foxMovement(Camera_Movement direction)
{
    float v = 2.5f * deltaTime;
    glm::vec3 s(1.0f, 0.0f, 1.0f);

    if (direction == FORWARD)
        foxLocation += camera.Front * v * s;
    if (direction == BACKWARD)
        foxLocation-= camera.Front * v * s;
    if (direction == LEFT)
        foxLocation -= camera.Right * v * s;
    if (direction == RIGHT)
        foxLocation += camera.Right * v * s;
}
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
