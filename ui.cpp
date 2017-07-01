#include "ui.h"
#include "ui_internal.h"
#include "debug_drawing.h"
#include "misc.h"
#include "text.h"

static bool ButtonBehavior(rect BoundingBox, ui_id ID, bool* OutHeld = NULL, bool* OutHovered = NULL, UI::button_flags_t Flags = 0);
static rect SliderBehavior(ui_id ID, rect BB, float* ScrollNorm, float NormDragSize, bool Vertical = false, bool* OutHeld = NULL, bool* OutHovering = NULL);
static void Scrollbar(gui_window* Window, bool Vertical);

void
UI::BeginFrame(game_state* GameState, const game_input* Input)
{
  if(g_Context.InitChecksum != CONTEXT_CHECKSUM)
  {
    Create(&g_Context, GameState);
  }
  g_Context.Input = Input;

  gui_context& g = g_Context;

  g.HotID         = 0;
  g.HoveredWindow = NULL;
  if(g.Input->ToggledEditorMode)
  {
    g.ActiveID = 0;
  }

  for(int i = g.OrderedWindows.Count - 1; 0 <= i; i--)
  {
    if(IsMouseInsideRect(g.OrderedWindows[i]->ClippedSizeRect))
    {
      // Set hovered window
      g.HoveredWindow = g.OrderedWindows[i];

      // Perform mouse scrolling
      if(g.Input->dMouseWheelScreen)
      {
        const float MagicScrollPixelAmount = 50;
        if(g.Input->LeftShift.EndedDown) // Horizontal scrolling
        {
          const float NormScrollDistance = (g.HoveredWindow->SizeNoScroll.X < g.HoveredWindow->ContentsSize.X)
                                             ? (g.Input->dMouseWheelScreen * MagicScrollPixelAmount) / (g.HoveredWindow->ContentsSize.X - g.HoveredWindow->SizeNoScroll.X)
                                             : 0;
          g.HoveredWindow->ScrollNorm.X = ClampFloat(0, g.HoveredWindow->ScrollNorm.X + NormScrollDistance, 1);
        }
        else // Vertical scrolling
        {
          const float NormScrollDistance = (g.HoveredWindow->SizeNoScroll.Y < g.HoveredWindow->ContentsSize.Y)
                                             ? (g.Input->dMouseWheelScreen * MagicScrollPixelAmount) / (g.HoveredWindow->ContentsSize.Y - g.HoveredWindow->SizeNoScroll.Y)
                                             : 0;
          g.HoveredWindow->ScrollNorm.Y = ClampFloat(0, g.HoveredWindow->ScrollNorm.Y + NormScrollDistance, 1);
        }
      }

      break;
    }
  }
  for(int i = 0; i < g.Windows.Count; i++)
  {
    if(g.ActiveID && g.Windows[i].MoveID == g.ActiveID)
    {
      MoveWindowToFront(&g.Windows[i]);
      g.Windows[i].Position += { (float)g.Input->dMouseScreenX, (float)g.Input->dMouseScreenY };
      break;
    }
  }
}

void
AddWindowToSortedBuffer(fixed_stack<gui_window*, 10>* SortedWindows, gui_window* Window)
{
  SortedWindows->Push(Window);
  for(int i = 0; i < Window->ChildWindows.Count; i++)
  {
    gui_window* ChildWindow = Window->ChildWindows[i];
    AddWindowToSortedBuffer(SortedWindows, ChildWindow);
  }
}

void
UI::EndFrame()
{
  gui_context&                 g = *GetContext();
  fixed_stack<gui_window*, 10> SortBuffer;
  SortBuffer.Clear();
  // Order windows back to front
  for(int i = 0; i < g.OrderedWindows.Count; i++)
  {
    gui_window* Window = g.OrderedWindows[i];
    if(!(Window->Flags & WINDOW_IsChildWindow))
    {
      AddWindowToSortedBuffer(&SortBuffer, Window);
    }
  }
  g.OrderedWindows = SortBuffer;

  // Submit for drawing
  for(int i = 0; i < g.OrderedWindows.Count; i++)
  {
    gui_window* Window = g.OrderedWindows[i];

    for(int j = 0; j < Window->DrawArray.Count; j++)
    {
      const quad_instance& Quad = Window->DrawArray[j];
      switch(Quad.Type)
      {
        case QuadType_Colored:
        {
          Debug::UIPushQuad(Quad.LowerLeft, Quad.Dimensions, Quad.Color);
          break;
        }
        case QuadType_Textured:
        {
          Debug::UIPushTexturedQuad(Quad.TextureID, Quad.LowerLeft, Quad.Dimensions);
          break;
        }
        case QuadType_Clip:
        {
          Debug::UIPushClipQuad(Quad.LowerLeft, Quad.Dimensions, (int32_t)Quad.Color.A);
          break;
        }
      }
    }
    g.OrderedWindows[i]->DrawArray.Clear();
    g.OrderedWindows[i]->ChildWindows.Clear();
  }
  g.ClipRectStack.Clear();
  g.LatestClipRectIndex = 0;
}

void
UI::SliderFloat(const char* Label, float* Value, float MinValue, float MaxValue, bool Vertical, vec3 Size, float DragSize)
{
  assert(Label);
  assert(Value);
  assert(MinValue < MaxValue);
  assert(0 < Size.X && 0 < Size.Y);
  assert(Vertical && DragSize < Size.Y || !Vertical && DragSize < Size.X);

  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Label);

  const float ValueRange   = MaxValue - MinValue;
  const float NormDragSize = DragSize / (Vertical ? Size.Y : Size.X);
  float       NormValue    = ((*Value) - MinValue) / ValueRange;

  rect SliderRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);
  if(!TestIfVisible(SliderRect))
  {
    return;
  }

  bool Held     = false;
  bool Hovered  = false;
  rect DragRect = SliderBehavior(ID, SliderRect, &NormValue, NormDragSize, Vertical, &Held, &Hovered);
  *Value        = NormValue * ValueRange + MinValue;

  DrawBox(SliderRect.MinP, SliderRect.GetSize(), _GetGUIColor(ScrollbarBox), _GetGUIColor(ScrollbarBox));
  char TempBuffer[20];
  snprintf(TempBuffer, sizeof(TempBuffer), "%.0f", *Value);
  DrawTextBox(DragRect.MinP, DragRect.GetSize(), TempBuffer, Held ? _GetGUIColor(ScrollbarBox) : _GetGUIColor(ScrollbarDrag), _GetGUIColor(ScrollbarDrag));
}

static void
Scrollbar(gui_window* Window, bool Vertical)
{
  gui_context& g = *GetContext();

  assert(Window);
  if(Window->ContentsSize.X == 0 || Window->ContentsSize.Y == 0)
  {
    return;
  }
  assert(0 < Window->ContentsSize.X);
  assert(0 < Window->ContentsSize.Y);
  assert(0 < Window->SizeNoScroll.X);
  assert(0 < Window->SizeNoScroll.Y);
  assert(g.Style.StyleVars[UI::VAR_ScrollbarSize].X <= Window->Size.X);
  assert(g.Style.StyleVars[UI::VAR_ScrollbarSize].X <= Window->Size.Y);

  ui_id ID = Window->GetID(Vertical ? "##VertScrollbar" : "##HirizScrollbar");

  // WithoutScrollbars
  const float ScrollbarSize = g.Style.StyleVars[UI::VAR_ScrollbarSize].X;

  // Determine drag size
  float*      ScrollNorm = (Vertical) ? &Window->ScrollNorm.Y : &Window->ScrollNorm.X;
  const float NormDragSize =
    (Vertical) ? MaxFloat((Window->SizeNoScroll.Y < Window->ContentsSize.Y) ? Window->SizeNoScroll.Y / Window->ContentsSize.Y : 1, g.Style.StyleVars[UI::VAR_DragMinSize].X / Window->SizeNoScroll.Y)
               : MaxFloat((Window->SizeNoScroll.X < Window->ContentsSize.X) ? Window->SizeNoScroll.X / Window->ContentsSize.X : 1, g.Style.StyleVars[UI::VAR_DragMinSize].X / Window->SizeNoScroll.X);
  rect ScrollRect = (Vertical) ? NewRect(Window->Position + vec3{ Window->SizeNoScroll.X, 0 }, Window->Position + vec3{ Window->Size.X, Window->SizeNoScroll.Y })
                               : NewRect(Window->Position + vec3{ 0, Window->SizeNoScroll.Y }, Window->Position + vec3{ Window->SizeNoScroll.X, Window->Size.Y });

  bool Held;
  bool Hovering;
  rect DragRect = SliderBehavior(ID, ScrollRect, ScrollNorm, NormDragSize, Vertical, &Held, &Hovering);

  DrawBox(ScrollRect.MinP, ScrollRect.GetSize(), _GetGUIColor(ScrollbarBox), _GetGUIColor(ScrollbarBox));
  DrawBox(DragRect.MinP, DragRect.GetSize(), Held ? _GetGUIColor(ScrollbarBox) : _GetGUIColor(ScrollbarDrag), _GetGUIColor(ScrollbarDrag));

  if(Vertical)
  {
    Window->ScrollRange.Y = (Window->SizeNoScroll.Y < Window->ContentsSize.Y) ? Window->ContentsSize.Y - Window->SizeNoScroll.Y : 0; // Delta in screen space that the window content can scroll
  }
  else
  {
    Window->ScrollRange.X = (Window->SizeNoScroll.X < Window->ContentsSize.X) ? Window->ContentsSize.X - Window->SizeNoScroll.X : 0; // Delta in screen space that the window content can scroll
  }
}

void
UI::BeginWindow(const char* Name, vec3 InitialPosition, vec3 Size, window_flags_t Flags)
{
  gui_context& g      = *GetContext();
  gui_window*  Window = NULL;

  ui_id ID = { IDHash(Name, sizeof(Name), 0) };

  int WindowCount = g.Windows.GetCount();
  for(int i = 0; i < WindowCount; i++)
  {
    if(g.Windows[i].ID == ID)
    {
      Window = &g.Windows[i];
      break;
    }
  }

  if(!Window)
  {
    gui_window NewWindow  = {};
    size_t     NameLength = strlen(Name);
    assert(strlen(Name) < ARRAY_SIZE(NewWindow.Name.Name));
    strcpy(NewWindow.Name.Name, Name);
    NewWindow.ID       = ID;
    NewWindow.MoveID   = NewWindow.GetID("##Move");
    NewWindow.Flags    = Flags;
    NewWindow.Size     = Size;
    NewWindow.Position = InitialPosition;
    Window             = g.Windows.Append(NewWindow);
    g.OrderedWindows.Push(Window);
  }
  g.CurrentWindow = Window;
  if(0 < g.CurrentWindowStack.Count)
  {
    assert(Window->Flags & WINDOW_IsChildWindow);
    Window->ParentWindow = g.CurrentWindowStack.Back();
  }
  g.CurrentWindowStack.Push(g.CurrentWindow);

  if(Window->Flags & WINDOW_IsChildWindow)
  {
    assert(Window->ParentWindow);
    Window->Position          = Window->ParentWindow->CurrentPos;
    Window->IndexWithinParent = g.CurrentWindowStack.Count - 1;
    assert(0 < Window->IndexWithinParent);
    assert(Window->ParentWindow);
    Window->ParentWindow->ChildWindows.Append(Window);
  }

  Window->MaxPos = Window->CurrentPos = Window->Position;

  PushClipQuad(Window, Window->Position, Window->Size, (Window->Flags & WINDOW_IsChildWindow) ? true : false);
  Window->ClippedSizeRect = NewRect(Window->Position, Window->Position + Size); // used for hovering
  Window->ClippedSizeRect.Clip(g.ClipRectStack.Back());
  DrawBox(Window->Position, Window->Size, _GetGUIColor(WindowBackground), _GetGUIColor(WindowBorder));

  // Order matters
  //#1
  Window->SizeNoScroll = Window->Size;
  Window->SizeNoScroll -= vec3{ (Window->Flags & UI::WINDOW_UseVerticalScrollbar) ? g.Style.StyleVars[UI::VAR_ScrollbarSize].X : 0,
                                (Window->Flags & UI::WINDOW_UseHorizontalScrollbar) ? g.Style.StyleVars[UI::VAR_ScrollbarSize].X : 0 };
  if(Window->SizeNoScroll.Y < Window->ContentsSize.Y) // Add vertical Scrollbar
  {
    Window->Flags |= UI::WINDOW_UseVerticalScrollbar;
  }
  if(Window->SizeNoScroll.X < Window->ContentsSize.X) // Add horizontal Scrollbar
  {
    Window->Flags |= UI::WINDOW_UseHorizontalScrollbar;
  }
  if(Window->SizeNoScroll.Y >= Window->ContentsSize.Y) // Remove vertical Scrollbar
  {
    Window->Flags        = (Window->Flags & ~UI::WINDOW_UseVerticalScrollbar);
    Window->ScrollNorm.Y = 0;
  }
  if(Window->SizeNoScroll.X >= Window->ContentsSize.X) // Remove horizontal Scrollbar
  {
    Window->Flags        = (Window->Flags & ~UI::WINDOW_UseHorizontalScrollbar);
    Window->ScrollNorm.X = 0;
  }
  Window->SizeNoScroll = Window->Size;
  Window->SizeNoScroll -= vec3{ (Window->Flags & UI::WINDOW_UseVerticalScrollbar) ? g.Style.StyleVars[UI::VAR_ScrollbarSize].X : 0,
                                (Window->Flags & UI::WINDOW_UseHorizontalScrollbar) ? g.Style.StyleVars[UI::VAR_ScrollbarSize].X : 0 };
  //#2
  if(Window->Flags & UI::WINDOW_UseVerticalScrollbar)
    Scrollbar(g.CurrentWindow, true);
  if(Window->Flags & UI::WINDOW_UseHorizontalScrollbar)
    Scrollbar(g.CurrentWindow, false);

  //#3
  Window->CurrentPos -= { Window->ScrollNorm.X * Window->ScrollRange.X, Window->ScrollNorm.Y * Window->ScrollRange.Y };
  PushClipQuad(Window, Window->Position, Window->SizeNoScroll);
}

void
UI::EndWindow()
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();
  vec3         MinPos = Window.Position - vec3{ Window.ScrollNorm.X * Window.ScrollRange.X, Window.ScrollNorm.Y * Window.ScrollRange.Y };
  Window.ContentsSize = Window.MaxPos - MinPos;

  //----------Window mooving-----------
  // Automatically sets g.ActiveID = Window.MoveID
  ButtonBehavior(NewRect(Window.Position, Window.Position + Window.Size), Window.MoveID);
  //----------------------------------
  g.ClipRectStack.Pop();
  g.ClipRectStack.Pop();

  /*PushClipQuad(&Window, {}, { SCREEN_WIDTH, SCREEN_HEIGHT }, false);
  DrawBox(MinPos, Window.ContentsSize, { 1, 0, 1, 1 }, { 1, 1, 1, 1 });
  g.ClipRectStack.Pop();*/

  g.CurrentWindowStack.Pop();
  g.CurrentWindow = (0 < g.CurrentWindowStack.Count) ? g.CurrentWindowStack.Back() : NULL;
}

void
UI::BeginChildWindow(const char* Name, vec3 Size, window_flags_t Flags)
{
  UI::BeginWindow(Name, {}, Size, Flags | WINDOW_IsChildWindow);
}

void
UI::EndChildWindow()
{
  gui_window* Window = GetCurrentWindow();
  assert(Window->Flags & WINDOW_IsChildWindow);
  UI::EndWindow();
  AddSize(Window->Size);
}

bool
UI::ReleaseButton(const char* Text, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Text);

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);

  if(!TestIfVisible(Rect))
  {
    return false;
  }

  bool Result     = ButtonBehavior(Rect, ID);
  vec4 InnerColor = (ID == g.ActiveID) ? _GetGUIColor(ButtonPressed) : ((ID == g.HotID) ? _GetGUIColor(ButtonHover) : _GetGUIColor(ButtonNormal));
  DrawTextBox(Rect.MinP, Size, Text, InnerColor, _GetGUIColor(Border));

  return Result;
}

bool
UI::CollapsingHeader(const char* Text, bool* IsExpanded, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  ui_id ID = Window.GetID(Text);

  const rect& Rect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  AddSize(Size);

  if(!TestIfVisible(Rect))
  {
    return *IsExpanded;
  }

  bool Result = ButtonBehavior(Rect, ID);
  *IsExpanded = (Result) ? !*IsExpanded : *IsExpanded;

  // draw square icon
  int32_t TextureID = (*IsExpanded) ? g.GameState->ExpandedTextureID : g.GameState->CollapsedTextureID;
  PushTexturedQuad(&Window, Rect.MinP, { Size.Y, Size.Y }, TextureID);

  vec4 InnerColor = (ID == g.ActiveID) ? _GetGUIColor(HeaderPressed) : ((ID == g.HotID) ? _GetGUIColor(HeaderHover) : _GetGUIColor(HeaderNormal));
  DrawTextBox({ Rect.MinP.X + Size.Y, Rect.MinP.Y }, { Size.X - Size.Y, Size.Y }, Text, InnerColor, _GetGUIColor(Border));

  return *IsExpanded;
}

/*
void
UI::SliderFloat(char* Text, float* Var, float Min, float Max, float ScreenValue)
{
  ui_id ID   = {};
  ID.DataPtr = (uintptr_t)Var;
  ID.NamePtr = (uintptr_t)Text;

  if(_LayoutIntersects(Layout, Input))
  {
    SetHot(ID);
  }
  else
  {
    SetHot(NOT_ACTIVE);
  }

  if((ID == g.ActiveID)
  {
    *Var += Input->NormdMouseX * ScreenValue;
    if(!Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(NOT_ACTIVE);
    }
  }
  else if(IsHot(ID))
  {
    if(Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      SetActive(ID);
    }
  }
  char FloatTextBuffer[20];

  *Var = ClampFloat(Min, *Var, Max);
  sprintf(FloatTextBuffer, "%5.2f", (double)*Var);

  vec4 InnerColor = (ID == g.ActiveID) ? g_PressedColor : (IsHot(ID) ? g_HighlightColor : g_NormalColor);
  UI::DrawTextBox(GameState, Layout, FloatTextBuffer, InnerColor, g_BorderColor);
  Layout->X += Layout->ColumnWidth;
}
*/

static bool
ButtonBehavior(rect BB, ui_id ID, bool* OutHeld, bool* OutHovered, UI::button_flags_t Flags)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  if(!(Flags & (UI::BUTTON_PressOnClick | UI::BUTTON_PressOnHold)))
  {
    Flags = UI::BUTTON_PressOnRelease;
  }

  bool Hovered = IsHovered(BB, ID);
  if(Hovered)
  {
    SetHot(ID);
  }

  bool Result = false;
  if(ID == g.HotID)
  {
    if((Flags & UI::BUTTON_PressOnRelease) && g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      SetActive(ID);
    }
    else if((Flags & UI::BUTTON_PressOnClick) && g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      SetActive(NOT_ACTIVE);
      Result = true;
    }
  }

  bool Held = false;
  if(ID == g.ActiveID)
  {
    if(g.Input->MouseLeft.EndedDown)
    {
      Held = true;
    }
    else if((Flags & UI::BUTTON_PressOnRelease) && !g.Input->MouseLeft.EndedDown && g.Input->MouseLeft.Changed)
    {
      if(ID == g.HotID)
      {
        Result = true;
      }
      SetActive(NOT_ACTIVE);
    }
  }

  if(OutHovered)
  {
    *OutHovered = Hovered;
  }
  if(OutHeld)
  {
    *OutHeld = Held;
  }

  return Result;
}

static rect
SliderBehavior(ui_id ID, rect BB, float* ScrollNorm, float NormDragSize, bool Vertical, bool* OutHeld, bool* OutHovering)
{
  assert(BB.MinP.X < BB.MaxP.X);
  assert(BB.MinP.Y < BB.MaxP.Y);
  *ScrollNorm  = ClampFloat(0, *ScrollNorm, 1);
  NormDragSize = ClampFloat(0, NormDragSize, 1);

  gui_context& g = *GetContext();

  // Determine actual drag size
  const float RegionSize = (Vertical) ? BB.GetHeight() : BB.GetWidth();
  assert(g.Style.StyleVars[UI::VAR_DragMinSize].X <= RegionSize);

  const float DragSize          = (NormDragSize * RegionSize < g.Style.StyleVars[UI::VAR_DragMinSize].X) ? g.Style.StyleVars[UI::VAR_DragMinSize].X : NormDragSize * RegionSize;
  const float MovableRegionSize = RegionSize - DragSize;
  const float DragOffset        = (*ScrollNorm) * MovableRegionSize;

  rect DragRect;
  if(Vertical)
  {
    DragRect = NewRect(BB.MinP.X, BB.MinP.Y + DragOffset, BB.MinP.X + BB.GetWidth(), BB.MinP.Y + DragOffset + DragSize);
  }
  else
  {
    DragRect = NewRect(BB.MinP.X + DragOffset, BB.MinP.Y, BB.MinP.X + DragOffset + DragSize, BB.MinP.Y + BB.GetHeight());
  }

  bool Held     = false;
  bool Hovering = false;
  bool Pressed  = ButtonBehavior(BB, ID, &Held, &Hovering);
  if(Held)
  {
    float ScreenRegionStart = ((Vertical) ? BB.MinP.Y : BB.MinP.X) + DragSize / 2;
    float ScreenRegionEnd   = ScreenRegionStart + MovableRegionSize;

    if(Vertical)
    {
      *ScrollNorm = ClampFloat(ScreenRegionStart, (float)g.Input->MouseScreenY, ScreenRegionEnd);
    }
    else
    {
      *ScrollNorm = ClampFloat(ScreenRegionStart, (float)g.Input->MouseScreenX, ScreenRegionEnd);
    }

    // TODO(Lukas) connect to other floating point precision facilities
    *ScrollNorm = (MovableRegionSize > 0.00001f) ? (((*ScrollNorm) - ScreenRegionStart) / MovableRegionSize) : 0;

    if(Vertical)
    {
      DragRect = NewRect(BB.MinP.X, BB.MinP.Y + DragOffset, BB.MinP.X + BB.GetWidth(), BB.MinP.Y + DragOffset + DragSize);
    }
    else
    {
      DragRect = NewRect(BB.MinP.X + DragOffset, BB.MinP.Y, BB.MinP.X + DragOffset + DragSize, BB.MinP.Y + BB.GetHeight());
    }
  }
  if(OutHeld)
  {
    *OutHeld = Held;
  }
  if(OutHovering)
  {
    *OutHovering = Hovering;
  }

  return DragRect;
}

void
UI::Image(int32_t TextureID, const char* Name, vec3 Size)
{
  gui_context& g      = *GetContext();
  gui_window&  Window = *GetCurrentWindow();

  rect ImageRect = NewRect(Window.CurrentPos, Window.CurrentPos + Size);
  if(!TestIfVisible(ImageRect))
  {
    AddSize(Size);
    return;
  }
  PushTexturedQuad(&Window, Window.CurrentPos, Size, TextureID);
  AddSize(Size);
}
