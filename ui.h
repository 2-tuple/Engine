#pragma once

#include "game.h"
#include "common.h"
#include "linear_math/vector.h"

extern vec4 g_BorderColor;
extern vec4 g_NormalColor;
extern vec4 g_HighlightColor;
extern vec4 g_PressedColor;
extern vec4 g_BoolNormalColor;
extern vec4 g_BoolPressedColor;
extern vec4 g_BoolHighlightColor;
extern vec4 g_FontColor;

namespace UI
{
  struct im_layout
  {
    union {
      struct
      {
        float X;
        float Y;
        float Z;
      };
      vec3 CurrentP;
    };
    float Width;
    float RowHeight;
    float ColumnWidth;
    vec3  TopLeft;
    float AspectRatio;
  };

  im_layout NewLayout(vec3 TopLeft, float Width, float RowHeight, float ScrollbarWidth = 0.05f,
                      float AspectRatio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);
  void Row(im_layout* Layout, int ColumnCount = 1);
  void Row(game_state* GameState, UI::im_layout* Layout, int ColumnCount, const char* Text,
           vec4 DescriptionColor = { 0.4f, 0.3f, 0.2f, 1 });
  void DrawSquareTexture(game_state* GameState, UI::im_layout* Layout, uint32_t TextureId);
  void DrawTextBox(game_state* GameState, vec3 TopLeft, float Width, float Height, const char* Text,
                   vec4 InnerColor, vec4 BorderColor);
  void DrawTextBox(game_state* GameState, im_layout* Layout, const char* Text,
                   vec4 InnerColor  = vec4{ 0.4f, 0.4f, 0.4f, 1 },
                   vec4 BorderColor = vec4{ 0, 0, 0, 1 });
  bool ExpandableButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                        const char* Text, bool* IsExpanded);
  void BoolButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                  const char* Text, bool* Toggle);
  bool PushButton(game_state* GameState, im_layout* Layout, const game_input* Input,
                  const char* Text, const void* ID = 0);
  void SliderFloat(game_state* GameState, im_layout* Layout, const game_input* Input, char* Text,
                   float* Var, float Min, float Max, float ScreenValue);
  void ComboBox(int32_t* ActiveIndex, void* ItemList, int32_t ListLength, game_state* GameState,
                im_layout* Layout, const game_input* Input, float SectionHeight, float* ScrollK,
                size_t ElementSize, char* (*ElementToCharPtr)(void*));
}

#define _DrawTextButton(Layout, Text) DrawTextButton(GameState, (Layout), (Text))
#define _BoolButton(Layout, Input, Text, Bool)                                                     \
  BoolButton(GameState, (Layout), (Input), (Text), (Bool))
#define _ExpandableButton(Layout, Input, Text, IsExpanded)                                         \
  ExpandableButton(GameState, (Layout), (Input), (Text), (IsExpanded))
#define _BeginScrollableList(Layout, Input, TotalRowCount, ScrollRowCount, g_ScrollK)              \
  BeginScrollableList(GameState, (Layout), (Input), (TotalRowCount), (ScrollRowCount), (g_ScrollK))
