#include "load_texture.h"
#include "profile.h"

#include <GL/glew.h>
#include <SDL2/SDL_image.h>

namespace Texture
{
  uint32_t
  LoadTexture(const char* FileName)
  {
    BEGIN_TIMED_BLOCK(LoadTexture);
    SDL_Surface* ImageSurface = IMG_Load(FileName);
    if(ImageSurface)
    {
      SDL_Surface* DestSurface =
        SDL_ConvertSurfaceFormat(ImageSurface, SDL_PIXELFORMAT_ABGR8888, 0);
      SDL_FreeSurface(ImageSurface);

      uint32_t Texture;
      glGenTextures(1, &Texture);
      glBindTexture(GL_TEXTURE_2D, Texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DestSurface->w, DestSurface->h, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, DestSurface->pixels);
      float MaxSupportedAnisotropy;
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &MaxSupportedAnisotropy);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MaxSupportedAnisotropy);
      glGenerateMipmap(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);

      SDL_FreeSurface(DestSurface);

      END_TIMED_BLOCK(LoadTexture);
      return Texture;
    }
    else
    {
      printf("Platform: texture load from image error: %s\n", SDL_GetError());
      assert(0);
    }
    END_TIMED_BLOCK(LoadTexture);
    return 0;
  }
}
