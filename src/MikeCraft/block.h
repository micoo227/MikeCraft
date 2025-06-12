#pragma once

#include <glad/glad.h>

struct AtlasCoords
{
    int x, y;
};

enum class Direction
{
    LEFT,
    RIGHT,
    BOTTOM,
    TOP,
    BACK,
    FRONT
};

class Block
{
   public:
    enum class Id
    {
        AIR,
        GRASS,
        DIRT,
        STONE,
        SAND,
        WOOD,
        LEAVES,
        WATER,
        LAVA,
    };

    Block(Id id);

    static AtlasCoords getAtlasCoords(Id id, Direction face);

    Id   getId() const;
    bool isSolid() const;
    bool isTransparent() const;

    static constexpr size_t VERTEX_COUNT = 3 * 8;  // 3 coordinates per vertex, 8 vertices
    static constexpr size_t INDEX_COUNT =
        6 * 2 * 3;  // 6 faces, 2 triangles per face, 3 indices per triangle

    static const GLfloat VERTICES[VERTEX_COUNT];
    static const GLuint  INDICES[INDEX_COUNT];

   private:
    Id   id;
    bool solid;
    bool transparent;
};