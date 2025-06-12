#include "chunk.h"
#include "zlib/zlib.h"

#include <glm/vec2.hpp>
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
                    Direction dir = static_cast<Direction>(face);
                    int       nx = x, ny = y, nz = z;
                    switch (dir)
                    {
                        case Direction::LEFT:
                            nx = x - 1;
                            break;  // left
                        case Direction::RIGHT:
                            nx = x + 1;
                            break;  // right
                        case Direction::BOTTOM:
                            ny = y - 1;
                            break;  // bottom
                        case Direction::TOP:
                            ny = y + 1;
                            break;  // top
                        case Direction::BACK:
                            nz = z - 1;
                            break;  // back
                        case Direction::FRONT:
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

                    addFace(x, y, z, dir, indexOffset);
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

    // Position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*) 0);
    glEnableVertexAttribArray(0);

    // Texture coordinate attribute (2 floats)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          (void*) (3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

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

void Chunk::addFace(int x, int y, int z, Direction face, GLuint& indexOffset)
{
    const GLfloat* offsets = faceVertexOffsets[static_cast<int>(face)];

    constexpr float cellSize = 1.0f / 16.0f;
    AtlasCoords     coords   = Block::getAtlasCoords(getBlock(x, y, z).getId(), face);
    float           u_min    = coords.x * cellSize;
    float           v_min    = coords.y * cellSize;
    float           u_max    = u_min + cellSize;
    float           v_max    = v_min + cellSize;

    // UVs for the 4 face vertices (adjust order if needed)
    glm::vec2 faceUVs[4] = {{u_min, v_min}, {u_max, v_min}, {u_max, v_max}, {u_min, v_max}};

    for (int i = 0; i < 4; ++i)
    {
        meshVertices.push_back(x + offsets[i * 3 + 0]);
        meshVertices.push_back(y + offsets[i * 3 + 1]);
        meshVertices.push_back(z + offsets[i * 3 + 2]);
        meshVertices.push_back(faceUVs[i].x);
        meshVertices.push_back(faceUVs[i].y);
    }
    // Add 6 indices for the face in order to make 2 tris
    for (int i = 0; i < 6; ++i)
    {
        meshIndices.push_back(indexOffset + faceIndices[i]);
    }
    indexOffset += 4;  // 4 vertices per face
}