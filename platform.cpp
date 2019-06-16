#ifdef __linux__
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <assert.h>

#include "imgui.h"

#include "common.h"
#include "profile.h"
#include "load_texture.h"

static bool
ProcessInput(const game_input* OldInput, game_input* NewInput, SDL_Event* Event, SDL_Window* Window)
{
  ImGuiIO& IO = ImGui::GetIO();

  *NewInput               = *OldInput;
  NewInput->dMouseScreenX = 0;
  NewInput->dMouseScreenY = 0;
  while(SDL_PollEvent(Event) != 0)
  {
    switch(Event->type)
    {
      case SDL_QUIT:
      {
        return false;
      }
      case SDL_KEYDOWN:
      case SDL_KEYUP:
      {
        bool IsDown = (Event->type == SDL_KEYDOWN);
        // NOTE(rytis): Dear Imgui
        {
          int Key = Event->key.keysym.scancode;
          IM_ASSERT(Key >= 0 && Key < IM_ARRAYSIZE(IO.KeysDown));
          IO.KeysDown[Key] = IsDown;
          IO.KeyShift      = !!(SDL_GetModState() & KMOD_SHIFT);
          IO.KeyCtrl       = !!(SDL_GetModState() & KMOD_CTRL);
          IO.KeyAlt        = !!(SDL_GetModState() & KMOD_ALT);
          IO.KeySuper      = !!(SDL_GetModState() & KMOD_GUI);
        }

        if(IO.WantTextInput)
        {
          break;
        }

        if(Event->key.keysym.sym == SDLK_ESCAPE)
        {
          NewInput->Escape.EndedDown = IsDown;
          return false;
        }
        if(Event->key.keysym.sym == SDLK_SPACE)
        {
          NewInput->Space.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_TAB)
        {
          NewInput->Tab.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_DELETE)
        {
          NewInput->Delete.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_LCTRL)
        {
          NewInput->LeftCtrl.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_LSHIFT)
        {
          NewInput->LeftShift.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_a)
        {
          NewInput->a.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_b)
        {
          NewInput->b.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_c)
        {
          NewInput->c.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_d)
        {
          NewInput->d.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_e)
        {
          NewInput->e.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_f)
        {
          NewInput->f.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_g)
        {
          NewInput->g.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_h)
        {
          NewInput->h.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_i)
        {
          NewInput->i.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_j)
        {
          NewInput->j.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_k)
        {
          NewInput->k.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_p)
        {
          NewInput->p.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_q)
        {
          NewInput->q.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_r)
        {
          NewInput->r.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_s)
        {
          NewInput->s.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_t)
        {
          NewInput->t.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_m)
        {
          NewInput->m.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_n)
        {
          NewInput->n.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_o)
        {
          NewInput->o.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_v)
        {
          NewInput->v.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_w)
        {
          NewInput->w.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_x)
        {
          NewInput->x.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_UP)
        {
          NewInput->ArrowUp.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_DOWN)
        {
          NewInput->ArrowDown.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_RIGHT)
        {
          NewInput->ArrowRight.EndedDown = IsDown;
        }
        if(Event->key.keysym.sym == SDLK_LEFT)
        {
          NewInput->ArrowLeft.EndedDown = IsDown;
        }
        break;
      }
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      {
        bool IsDown     = (Event->type == SDL_MOUSEBUTTONDOWN);
        IO.MouseDown[0] = IsDown && (Event->button.button == SDL_BUTTON_LEFT);
        IO.MouseDown[1] = IsDown && (Event->button.button == SDL_BUTTON_RIGHT);
        IO.MouseDown[2] = IsDown && (Event->button.button == SDL_BUTTON_MIDDLE);
				//TODO: Reenable this after finishing transition to ImGui
#if 0
        if(IO.WantCaptureMouse)
        {
          break;
        }
#endif
        NewInput->MouseLeft.EndedDown   = IO.MouseDown[0];
        NewInput->MouseRight.EndedDown  = IO.MouseDown[1];
        NewInput->MouseMiddle.EndedDown = IO.MouseDown[2];
        break;
      }
      case SDL_MOUSEWHEEL:
      {
        // NOTE(rytis): Dear ImGui.
        {
          if(Event->wheel.x > 0)
          {
            IO.MouseWheelH += 1;
          }
          else if(Event->wheel.x < 0)
          {
            IO.MouseWheelH -= 1;
          }
          if(Event->wheel.y > 0)
          {
            IO.MouseWheel += 1;
          }
          else if(Event->wheel.y < 0)
          {
            IO.MouseWheel -= 1;
          }
        }

        if(Event->wheel.direction == SDL_MOUSEWHEEL_NORMAL)
        {
          NewInput->MouseWheelScreen -= Event->wheel.y;
        }
        else
        {
          NewInput->MouseWheelScreen += Event->wheel.y;
        }
        break;
      }
      case SDL_MOUSEMOTION:
      {
        NewInput->MouseScreenX = Event->motion.x;
        NewInput->MouseScreenY = Event->motion.y;
        NewInput->dMouseScreenX += Event->motion.xrel;
        NewInput->dMouseScreenY += Event->motion.yrel;
        break;
      }
      case SDL_TEXTINPUT:
      {
        IO.AddInputCharactersUTF8(Event->text.text);
        break;
      }
    }
  }

  for(uint32_t Index = 0; Index < sizeof(NewInput->Buttons) / sizeof(game_button_state); Index++)
  {
    NewInput->Buttons[Index].Changed =
      (OldInput->Buttons[Index].EndedDown == NewInput->Buttons[Index].EndedDown) ? false : true;
  }

  if(NewInput->Tab.EndedDown && NewInput->Tab.Changed)
  {
    NewInput->IsMouseInEditorMode = !NewInput->IsMouseInEditorMode;
    SDL_SetRelativeMouseMode((NewInput->IsMouseInEditorMode) ? SDL_FALSE : SDL_TRUE);
    SDL_WarpMouseInWindow(Window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

    NewInput->MouseScreenX  = SCREEN_WIDTH / 2;
    NewInput->MouseScreenY  = SCREEN_HEIGHT / 2;
    NewInput->dMouseScreenX = 0;
    NewInput->dMouseScreenY = 0;
  }
  NewInput->ToggledEditorMode = (OldInput->IsMouseInEditorMode != NewInput->IsMouseInEditorMode);

  NewInput->MouseX  = NewInput->MouseScreenX;
  NewInput->MouseY  = SCREEN_HEIGHT - NewInput->MouseScreenY;
  NewInput->dMouseX = NewInput->dMouseScreenX;
  NewInput->dMouseY = -NewInput->dMouseScreenY;

  NewInput->NormMouseX  = (float)NewInput->MouseX / (float)(SCREEN_WIDTH);
  NewInput->NormMouseY  = (float)NewInput->MouseY / (float)(SCREEN_HEIGHT);
  NewInput->NormdMouseX = (float)NewInput->dMouseX / (float)(SCREEN_WIDTH);
  NewInput->NormdMouseY = (float)NewInput->dMouseY / (float)(SCREEN_HEIGHT);

  IO.MousePos = ImVec2(float(NewInput->MouseScreenX), float(NewInput->MouseScreenY));

  NewInput->dMouseWheelScreen = NewInput->MouseWheelScreen - OldInput->MouseWheelScreen;

  return true;
}

static bool
Init(SDL_Window** Window)
{
  bool Success = true;
  if(SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    printf("SDL error: init failed. %s\n", SDL_GetError());
    Success = false;
  }
  else
  {
    // Set Opengl content version to 4.3 core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#if 1
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

    // Create an SDL window
    SDL_SetRelativeMouseMode(SDL_TRUE);
    // TODO(rytis): Print screen is not working correctly. Might have something to do with SDL
    // window creation or input handling.
#ifdef FULL_SCREEN
    *Window =
      SDL_CreateWindow("ngpe - Non general-purpose engine", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
#else
    *Window =
      SDL_CreateWindow("ngpe - Non general-purpose engine", 200, 100, SCREEN_WIDTH, SCREEN_HEIGHT,
                       SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
#endif

    if(!Window)
    {
      printf("SDL error: failed to load window. %s\n", SDL_GetError());
      Success = false;
    }
    else
    {
      // Establish an OpenGL context with SDL
      SDL_GLContext GraphicsContext = SDL_GL_CreateContext(*Window);
      if(!GraphicsContext)
      {
        printf("SDL error: failed to establish an opengl context. %s\n", SDL_GetError());
        Success = false;
      }
      else
      {
        glewExperimental = GL_TRUE;
        GLenum GlewError = glewInit();
        if(GlewError != GLEW_OK)
        {
          printf("GLEW error: initialization failed. %s\n", glewGetErrorString(GlewError));
          Success = false;
        }

        if(SDL_GL_SetSwapInterval(1) < 0)
        {
          printf("SDL error: unable to set swap interval. %s\n", glewGetErrorString(GlewError));
          Success = false;
        }
      }
    }
  }

  // NOTE(rytis): Dear Imgui setup.
  {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.IniFilename = 0;
    IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    int32_t Width, Height;
    uint8_t* Pixels = 0;
    IO.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);
    GLuint FontAtlas = Texture::LoadTexture(Pixels, Width, Height);
    IO.Fonts->TexID = (ImTextureID)(uintptr_t)FontAtlas;

    IO.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    IO.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    IO.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    IO.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    IO.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    IO.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    IO.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    IO.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    IO.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    IO.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    IO.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    IO.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    IO.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    IO.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    IO.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    IO.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    IO.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    IO.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    IO.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    IO.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    IO.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

    // TODO(rytis): Clipboard, cursors.
  }

  return Success;
}

int
main(int argc, char* argv[])
{
  Platform::SetHighDPIAwareness();

  SDL_Window* Window = nullptr;
  if(!Init(&Window))
  {
    return -1;
  }

  Texture::SetTextureVerticalFlipOnLoad();

  // Init TrueType Font API
  if(TTF_Init() == -1)
  {
    printf("TrueType Fonts could not be initialized!\n");
    return -1;
  }

  game_memory GameMemory;
  {
    GameMemory.TemporaryMemorySize  = Mibibytes(50);
    GameMemory.PersistentMemorySize = Mibibytes(400);

    GameMemory.TemporaryMemory  = malloc(GameMemory.TemporaryMemorySize);
    GameMemory.PersistentMemory = malloc(GameMemory.PersistentMemorySize);

    if(GameMemory.TemporaryMemory && GameMemory.PersistentMemory)
    {
      GameMemory.HasBeenInitialized = true;
    }
    else
    {
      assert("error: unable to initialize memory" && 0);
      GameMemory.TemporaryMemorySize  = 0;
      GameMemory.PersistentMemorySize = 0;
      GameMemory.HasBeenInitialized   = false;
    }
  }

  SDL_Event Event;

  ImGuiIO& IO = ImGui::GetIO();
  game_input OldInput = {};
  game_input NewInput = {};
  if(!NewInput.IsMouseInEditorMode)
  {
    SDL_WarpMouseInWindow(Window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
  }

  ProcessInput(&OldInput, &NewInput, &Event, Window);
  OldInput = NewInput;

  Platform::InitPerformanceFrequency();
  int64_t LastFrameStart = Platform::GetCurrentCounter();
  while(true)
  {
    int64_t CurrentFrameStart = Platform::GetCurrentCounter();

    if(!ProcessInput(&OldInput, &NewInput, &Event, Window))
    {
      break;
    }
    NewInput.dt = Platform::GetTimeInSeconds(LastFrameStart, CurrentFrameStart);
    if(NewInput.LeftCtrl.EndedDown)
    {
      NewInput.dt *= SLOW_MOTION_COEFFICIENT;
    }
#if 0
    NewInput.dt = 1.0f/240;
#endif
    IO.DeltaTime = NewInput.dt;
    IO.DisplaySize.x = SCREEN_WIDTH;
    IO.DisplaySize.y = SCREEN_HEIGHT;

    GameUpdateAndRender(GameMemory, &NewInput);

    SDL_GL_SwapWindow(Window);

    int DelayTime = FRAME_TIME_MS - 1;
    SDL_Delay(DelayTime);

    OldInput       = NewInput;
    LastFrameStart = CurrentFrameStart;
  }

  ImGui::DestroyContext();
  free(GameMemory.TemporaryMemory);
  free(GameMemory.PersistentMemory);
  TTF_Quit();
  SDL_DestroyWindow(Window);
  SDL_Quit();

  return 0;
}
