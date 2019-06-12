#include "load_texture.h"
#include "profile.h"

#include <GL/glew.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Texture
{
  void SetTextureVerticalFlipOnLoad()
  {
    // TODO(rytis): Decide whether we want to flip the textures on load.
    // stbi_set_flip_vertically_on_load(true);
  }

  uint32_t
  LoadTexture(const char* FileName)
  {
    BEGIN_TIMED_BLOCK(LoadTexture);

    int32_t Width, Height, Components;
    uint8_t* Data = stbi_load(FileName, &Width, &Height, &Components, 4);

    uint32_t Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(Data);

    END_TIMED_BLOCK(LoadTexture);
    return Texture;
  }

  uint32_t
  LoadTexture(uint8_t* Data, int32_t Width, int32_t Height)
  {
    BEGIN_TIMED_BLOCK(LoadTexture);

    uint32_t Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
    // glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    END_TIMED_BLOCK(LoadTexture);
    return Texture;
  }

  uint32_t
  LoadCubemapTexture(path* FilePaths)
  {
    uint32_t Texture;
    glGenTextures(1, &Texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Texture);

    for(int i = 0; i < 6; i++)
    {
      int32_t Width, Height, Components;
      uint8_t* Data = stbi_load(FilePaths[i].Name, &Width, &Height, &Components, 4);
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, Width, Height,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, Data);
      stbi_image_free(Data);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return Texture;
  }
}
