#include "texture_atlas.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stdexcept>

TextureAtlas::TextureAtlas(const std::string& path, bool flipVertically)
{
    stbi_set_flip_vertically_on_load(flipVertically);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data)
        throw std::runtime_error("Failed to load texture: " + path);

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

TextureAtlas::~TextureAtlas()
{
    glDeleteTextures(1, &id);
}

void TextureAtlas::bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void TextureAtlas::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}