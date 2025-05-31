#include "renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

GLuint Renderer::VAO                  = 0;
GLuint Renderer::VBO                  = 0;
GLuint Renderer::EBO                  = 0;
bool   Renderer::blockMeshInitialized = false;

Renderer::Renderer(Shader& shader, Camera& camera) : shader(shader), camera(camera)
{
    initBlockMesh();
}

void Renderer::renderChunk(const Chunk& chunk)
{
    // Assume chunk.uploadMeshToGPU() has already been called and mesh is ready
    if (!chunk.meshGenerated || chunk.VAO == 0)
        return;

    shader.use();
    shader.setMat4("view", camera.getViewMatrix());
    shader.setMat4("projection", camera.getProjectionMatrix());

    glm::mat4 model = glm::translate(
        glm::mat4(1.0f), glm::vec3(chunk.getX() * Chunk::WIDTH, 0, chunk.getZ() * Chunk::DEPTH));
    shader.setMat4("model", model);

    glBindVertexArray(chunk.VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(chunk.meshIndices.size()), GL_UNSIGNED_INT,
                   0);
    glBindVertexArray(0);
}

void Renderer::renderChunks(const std::vector<Chunk*>& chunks)
{
    for (const Chunk* chunk : chunks)
    {
        if (chunk)
            renderChunk(*chunk);
    }
}

void Renderer::initBlockMesh()
{
    if (blockMeshInitialized)
        return;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Block::VERTICES), Block::VERTICES, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Block::INDICES), Block::INDICES, GL_STATIC_DRAW);

    // Position attribute (3 floats per vertex)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    blockMeshInitialized = true;
}