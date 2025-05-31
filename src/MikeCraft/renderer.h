#pragma once

#include "camera.h"
#include "chunk.h"
#include "shader.h"

#include <glad/glad.h>

class Renderer
{
   public:
    Renderer(Shader& shader, Camera& camera);
    void renderChunk(const Chunk& chunk);
    void renderChunks(const std::vector<Chunk*>& chunks);

   private:
    Shader&       shader;
    Camera&       camera;
    static GLuint VAO, VBO, EBO;
    static void   initBlockMesh();
    static bool   blockMeshInitialized;
};