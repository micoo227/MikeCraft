#include "block.h"

Block::Block(Id id) : id(id)
{
    switch (id)
    {
        case Id::AIR:
            solid       = false;
            transparent = true;
            break;
        case Id::WATER:
            solid       = false;
            transparent = true;
            break;
        default:
            solid       = true;
            transparent = false;
            break;
    }
}

AtlasCoords Block::getAtlasCoords(Block::Id id, Direction face)
{
    switch (id)
    {
        case Id::GRASS:
            if (face == Direction::TOP)
                return AtlasCoords{0, 15};
            else if (face == Direction::BOTTOM)
                return AtlasCoords{2, 15};
            else
                return AtlasCoords{1, 15};
        case Id::DIRT:
            return {2, 15};
        case Id::STONE:
            return {3, 15};
        default:
            return {0, 0};
    }
}

Block::Id Block::getId() const
{
    return id;
}

bool Block::isSolid() const
{
    return solid;
}

bool Block::isTransparent() const
{
    return transparent;
}

// clang-format off
const GLfloat Block::VERTICES[] = {
	0, 0, 0,
	1, 0, 0,
	1, 1, 0,
	0, 1, 0,

	0, 0, 1,
	1, 0, 1,
	1, 1, 1,
	0, 1, 1,
};

const GLuint Block::INDICES[] = {
	1, 0, 3, 1, 3, 2, // Back face
	4, 5, 6, 4, 6, 7, // Front face
	5, 1, 2, 5, 2, 6, // Right face
	0, 4, 7, 0, 7, 3, // Left face
	2, 3, 7, 2, 7, 6, // Top face
	5, 4, 0, 5, 0, 1, // Bottom face
};
// clang-format on