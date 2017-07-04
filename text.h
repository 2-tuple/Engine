#pragma once

#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "linear_math/vector.h"

#define MAX_FONT_SIZE_COUNT 8
#define FONT_NAME_MAX_SIZE 64

#define TEXT_LINE_MAX_LENGTH 100

#define TEST_TEXT_CACHE 0

#if TEST_TEXT_CACHE
#define TEXTURE_CACHE_LINE_COUNT 6
#else
#define TEXTURE_CACHE_LINE_COUNT 256
#endif

namespace Text
{
  struct sized_font
  {
    TTF_Font* Font;
    int32_t   Size;
  };

  struct font
  {
    char       Name[FONT_NAME_MAX_SIZE];
    sized_font SizedFonts[MAX_FONT_SIZE_COUNT];
    int32_t    SizeCount;
    float      AverageSymbolWidth;
  };

  sized_font LoadSizedFont(const char* FontName, int FontSize);
  font       LoadFont(const char* FontName, int MinSize, int SizeCount, int DeltaSize);
  uint32_t   GetTextTextureID(font* Font, int32_t FontSize, const char* Text, vec4 Color, int32_t* Width = 0, int32_t* Height = 0);
  void       GetTextSize(const font* Font, const char* Text, int32_t* OutWidth, int32_t* OutHeight);
  void       ClearTextRequestCounts();
  void       ResetCache();
}
