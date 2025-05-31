#include "chunk.h"
#include "zlib/zlib.h"

#include <stdexcept>

// clang-format off
namespace {
    const GLfloat faceVertexOffsets[6][12] = {
        {0,0,1, 0,0,0, 0,1,0, 0,1,1}, // left
        {1,0,0, 1,0,1, 1,1,1, 1,1,0}, // right
        {0,0,0, 1,0,0, 1,0,1, 0,0,1}, // bottom
        {0,1,1, 1,1,1, 1,1,0, 0,1,0}, // top
        {1,0,0, 0,0,0, 0,1,0, 1,1,0}, // back
        {0,0,1, 1,0,1, 1,1,1, 0,1,1}  // front
    };
    constexpr GLuint faceIndices[6] = {0, 1, 2, 0, 2, 3};
}
// clang-format on

Chunk::Chunk(int x, int z)
    : chunkX(x), chunkZ(z), blocks(WIDTH * HEIGHT * DEPTH, Block(Block::Id::AIR))
{
}

Block Chunk::getBlock(int x, int y, int z) const
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH)
    {
        throw std::out_of_range("Block coordinates out of range");
    }
    return blocks[x + WIDTH * (z + DEPTH * y)];
}

void Chunk::setBlock(int x, int y, int z, const Block& block)
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH)
    {
        throw std::out_of_range("Block coordinates out of range");
    }
    blocks[x + WIDTH * (z + DEPTH * y)] = block;
}

std::vector<uint8_t> Chunk::serialize() const
{
    std::vector<uint8_t> rawData;
    rawData.reserve(blocks.size());
    for (const Block& block : blocks)
    {
        rawData.push_back(static_cast<uint8_t>(block.getId()));
    }

    uLongf               compressedSize = compressBound(rawData.size());
    std::vector<uint8_t> compressedData(compressedSize);

    int result = compress(compressedData.data(), &compressedSize, rawData.data(), rawData.size());
    if (result != Z_OK)
    {
        throw std::runtime_error("Failed to compress chunk data");
    }

    compressedData.resize(compressedSize);

    std::vector<uint8_t> serializedData;
    uint32_t             length = static_cast<uint32_t>(compressedData.size());
    serializedData.push_back((length >> 24) & 0xFF);  // Most significant byte
    serializedData.push_back((length >> 16) & 0xFF);
    serializedData.push_back((length >> 8) & 0xFF);
    serializedData.push_back(length & 0xFF);  // Least significant byte

    serializedData.insert(serializedData.end(), compressedData.begin(), compressedData.end());

    return serializedData;
}

void Chunk::generateMesh()
{
    meshVertices.clear();
    meshIndices.clear();
    GLuint indexOffset = 0;

    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int z = 0; z < DEPTH; ++z)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                Block block = getBlock(x, y, z);
                if (!block.isSolid())
                    continue;

                for (int face = 0; face < 6; ++face)
                {
                    int nx = x, ny = y, nz = z;
                    switch (face)
                    {
                        case 0:
                            nx = x - 1;
                            break;  // left
                        case 1:
                            nx = x + 1;
                            break;  // right
                        case 2:
                            ny = y - 1;
                            break;  // bottom
                        case 3:
                            ny = y + 1;
                            break;  // top
                        case 4:
                            nz = z - 1;
                            break;  // back
                        case 5:
                            nz = z + 1;
                            break;  // front
                    }
                    bool neighborSolid = false;
                    if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && nz >= 0 && nz < DEPTH)
                    {
                        neighborSolid = getBlock(nx, ny, nz).isSolid();
                    }
                    if (neighborSolid)
                        continue;

                    addFace(x, y, z, face, indexOffset);
                }
            }
        }
    }
    meshGenerated = true;
}

void Chunk::uploadMeshToGPU()
{
    if (VAO == 0)
        glGenVertexArrays(1, &VAO);
    if (VBO == 0)
        glGenBuffers(1, &VBO);
    if (EBO == 0)
        glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(GLfloat), meshVertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(GLuint), meshIndices.data(),
                 GL_STATIC_DRAW);

    // Position attribute (assuming 3 floats per vertex)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*) 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Chunk::deleteMesh()
{
    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0)
    {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    meshGenerated = false;
    meshVertices.clear();
    meshIndices.clear();
}

void Chunk::addFace(int x, int y, int z, int face, GLuint& indexOffset)
{
    const GLfloat* offsets = faceVertexOffsets[face];
    for (int i = 0; i < 4; ++i)
    {
        meshVertices.push_back(x + offsets[i * 3 + 0]);
        meshVertices.push_back(y + offsets[i * 3 + 1]);
        meshVertices.push_back(z + offsets[i * 3 + 2]);
    }
    // Add 6 indices for the face in order to make 2 tris
    for (int i = 0; i < 6; ++i)
    {
        meshIndices.push_back(indexOffset + faceIndices[i]);
    }
    indexOffset += 4;  // 4 vertices per face
}