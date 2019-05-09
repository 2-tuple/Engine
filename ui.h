#pragma once

#include "game.h"
#include "common.h"
#include "linear_math/vector.h"

namespace UI
{
  enum ui_colors
  {
    COLOR_Border,
    COLOR_ButtonNormal,
    COLOR_ButtonHovered,
    COLOR_ButtonPressed,
    COLOR_HeaderNormal,
    COLOR_HeaderHovered,
    COLOR_HeaderPressed,
    COLOR_CheckboxNormal,
    COLOR_CheckboxPressed,
    COLOR_CheckboxHovered,
    COLOR_ComboNormal,
    COLOR_WindowBackground,
    COLOR_WindowBorder,
    COLOR_ChildWindowBackground,
    COLOR_ScrollbarBox,
    COLOR_ScrollbarDragNormal,
    COLOR_ScrollbarDragPressed,
    COLOR_SliderBox,
    COLOR_SliderDragNormal,
    COLOR_SliderDragPressed,
    COLOR_Text,
    COLOR_Count
  };

  enum ui_style_vars
  {
    VAR_BorderThickness,
    VAR_FontSize,
    VAR_WindowPaddingX,
    VAR_WindowPaddingY,
    VAR_IndentSpacing,
    VAR_HeaderIndentSpacing,
    VAR_BoxPaddingX,
    VAR_BoxPaddingY,
    VAR_SpacingX,
    VAR_SpacingY,
    VAR_InternalSpacing,
    VAR_ScrollbarSize,
    VAR_DragMinSize,
    VAR_Count
  };

  enum ui_window_flags
  {
    WINDOW_UseVerticalScrollbar   = 1 << 0,
    WINDOW_UseHorizontalScrollbar = 1 << 1,
    WINDOW_Usetitlebar            = 1 << 2,
    WINDOW_ISCollapsable          = 1 << 3,
    WINDOW_IsChildWindow          = 1 << 4,
    WINDOW_IsNotMovable           = 1 << 5,
    WINDOW_IsNotResizable         = 1 << 6,
    WINDOW_Popup                  = 1 << 7,
    WINDOW_Combo                  = 1 << 8,
    WINDOW_NoWindowPadding        = 1 << 9,
  };

  enum ui_button_flags
  {
    BUTTON_PressOnRelease = 1 << 0,
    BUTTON_PressOnClick   = 1 << 1,
    BUTTON_PressOnHold    = 1 << 2,
  };

  struct gui_style
  {
    vec4  Colors[COLOR_Count];
    float Vars[VAR_Count];
  };

  typedef uint32_t window_flags_t;
  typedef uint32_t button_flags_t;

  void BeginFrame(game_state* GameState, const game_input* Input);
  void EndFrame();

  void BeginWindow(const char* Name, vec3 InitialPosition, vec3 Size, window_flags_t Flags = 0);
  void BeginChildWindow(const char* Name, vec3 Size, window_flags_t Flags = 0);
  void BeginPopupWindow(const char* Name, vec3 Size, window_flags_t Flags = 0);
  void EndWindow();
  void EndChildWindow();
  void EndPopupWindow();

  void Dummy(float Width = 0, float Height = 0);
  // void  TextBox(const char* Text, Width = 0);
  // void  Box(vec2 Size = {});
  float GetWindowWidth();
  float GetAvailableWidth();
  void  SameLine(float PosX = 0, float SpacingWidth = -1);
  void  NewLine();

  void PushVar(int32_t Index, float Value);
  void PopVar();
  void PushWidth(float Width);
  void PopWidth();
  void PushColor(int32_t Index, vec4 Color);
  void PopColor();
  void PushID(const void* ID);
  void PushID(const char* ID);
  void PushID(const int ID);
  void PopID();
  void Indent(float IndentWidth = 0);
  void Unindent(float IndentWidth = 0);

  bool CollapsingHeader(const char* Text, bool* IsExpanded, bool UseAvaiableWidth = true);
  bool TreeNode(const char* Label, bool* IsExpanded);
  void TreePop();
  bool Button(const char* Text, float Width = 0);
  bool ClickButton(const char* Text);
  void Checkbox(const char* Label, bool* Toggle);

  void DragFloat(const char* Label, float* Value, float MinValue, float MaxValue,
                 float ScreenDelta);

  void SliderFloat(const char* Label, float* Value, float MinValue, float MaxValue,
                   bool Vertical = false);
  void SliderRange(const void* ID, float* LeftRange, float* RightRange, float MinValue,
                   float MaxValue);
  void DragFloat3(const char* Label, float Value[3], float MinValue, float MaxValue,
                  float ScreenDelta);
  void DragFloat4(const char* Label, float Value[4], float MinValue, float MaxValue,
                  float ScreenDelta);

  void SliderInt(const char* Label, int32_t* Value, int32_t MinValue, int32_t MaxValue,
                 bool Vertical = false);

  void Combo(const char* Label, int* CurrentItem, const void* Data, int ItemCount,
             const char* (*DataToText)(const void*, int), int HeightInItems = 8);
  void Combo(const char* Label, int* CurrentItem, const char** Items, int ItemCount,
             int HeightInItems = 5);

  void Image(const char* Name, int32_t TextureID, vec3 Size);
  void Text(const char* Text);
  bool SelectSphere(vec3* Position, float Radius = 0.1f, vec4 Color = { 1, 0, 0, 1 },
                    bool PerspectiveInvariant = false, bool DrawSphere = false);

  void MoveGizmo(transform* Transform, bool UseLocalAxes = false);
  void MoveGizmo(vec3* Position);
  void PlaneGizmo(transform* Transform, int32_t PlaneIndex);
  void PlaneGizmo(transform* Position, vec3 PlaneNormal);
  // void PlaneGizmo(transform* Position, parametric_plane);

  bool     IsItemActive();
  uint32_t GetActiveID();
  uint32_t GetHotID();

  gui_style* GetStyle();

  inline const char*
  StringArrayToString(const void* Data, int Index)
  {
    const char** StringArray = (const char**)Data;
    return StringArray[Index];
  }
}
