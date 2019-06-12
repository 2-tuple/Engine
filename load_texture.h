#pragma once

#include "stack_alloc.h"

#include <GL/glew.h>
#include "file_queries.h"

namespace Texture
{
  void SetTextureVerticalFlipOnLoad();
  uint32_t LoadTexture(uint8_t* Data, int32_t Width, int32_t Height);
  uint32_t LoadTexture(const char* FileName);
  uint32_t LoadCubemapTexture(path* FilePaths);
}
