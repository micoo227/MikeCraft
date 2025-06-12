#pragma once

#include "block.h"

#include <vector>
#include <cstdint>

class Chunk
{
   public:
    static const int WIDTH  = 16;   // X-axis
    static const int HEIGHT = 256;  // Y-axis
    static const int DEPTH  = 16;   // Z-axis

    std::vector<GLfloat> meshVertices;
    std::vector<GLuint>  meshIndices;
    GLuint               VAO = 0, VBO = 0, EBO = 0;
    bool                 meshGenerated = false;

    Chunk(int x, int z);

    Block getBlock(int x, int y, int z) const;
    void  setBlock(int x, int y, int z, const Block& block);

    std::vector<uint8_t> serialize() const;

    int getX() const
    {
        return chunkX;
    }
    int getZ() const
    {
        return chunkZ;
    }

    void generateMesh();
    void uploadMeshToGPU();
    void deleteMesh();

   private:
    int                chunkX, chunkZ;
    std::vector<Block> blocks;  // 1 byte per block

    void addFace(int x, int y, int z, Direction face, GLuint& indexOffset);
};
