#include "camera.h"
#include "chunk_manager.h"
#include "file_utils.h"
#include "renderer.h"
#include "shader.h"
#include "texture_atlas.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

bool   firstMouse = true;
float  lastX      = 800.0f / 2.0;
float  lastY      = 600.0f / 2.0;
Camera camera;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(MovementDirections::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(MovementDirections::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(MovementDirections::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(MovementDirections::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.processKeyboard(MovementDirections::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.processKeyboard(MovementDirections::DOWN, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX      = xpos;
        lastY      = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to top
    lastX         = xpos;
    lastY         = ypos;

    camera.processMouseMovement(xOffset, yOffset);
}

int main()
{
    ensureWorldFolderExists();

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "MikeCraft", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    Shader       shader("../../res/shaders/default.vert", "../../res/shaders/default.frag");
    TextureAtlas textureAtlas("../../res/images/atlas.png", true);
    textureAtlas.bind(0);
    shader.use();
    shader.setInt("atlas", 0);

    Renderer     renderer(shader, camera);
    ChunkManager chunkManager;

    Chunk&              chunk  = chunkManager.getChunk(1, 1);
    Chunk&              chunk2 = chunkManager.getChunk(-1, -1);
    Chunk&              chunk3 = chunkManager.getChunk(1, -1);
    Chunk&              chunk4 = chunkManager.getChunk(-1, 1);
    std::vector<Chunk*> chunks = chunkManager.getLoadedChunks();

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime          = currentFrame - lastFrame;
        lastFrame          = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.renderChunks(chunks);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    chunkManager.unloadAllChunks();

    glfwTerminate();
    return 0;
}