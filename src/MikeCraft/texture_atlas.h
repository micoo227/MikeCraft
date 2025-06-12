#pragma once

#include <string>
#include <glad/glad.h>

class TextureAtlas
{
   public:
    TextureAtlas(const std::string& path, bool flipVertically = true);
    ~TextureAtlas();

    void bind(GLuint unit = 0) const;
    void unbind() const;

    GLuint getId() const
    {
        return id;
    }

   private:
    GLuint id    = 0;
    int    width = 256, height = 256, channels = 4;
};