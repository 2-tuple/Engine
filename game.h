#pragma once

#include <SDL2/SDL_ttf.h>
#include <stdint.h>

#include "model.h"
#include "anim.h"
#include "skeleton.h"
#include "linear_math/vector.h"
#include "linear_math/matrix.h"
#include "stack_allocator.h"
#include "edit_animation.h"
#include "camera.h"

static const int32_t TEXTURE_MAX_COUNT = 20;

struct entity
{
  Render::model*              Model;
  Anim::transform             Transform;
  Anim::animation_controller* AnimController;
};

struct text_texture
{
  char*   Text;
  vec4    Color;
  int32_t FontSize;
  int32_t Texture;
};

struct sized_font
{
  TTF_Font* Font;
  int32_t   Size;
};

struct loaded_wav
{
  int16_t* AudioData;
  uint32_t AudioLength;
  uint32_t AudioSampleIndex;
};

enum engine_mode
{
  MODE_AnimationEditor,
  MODE_EntityCreation,
  MODE_EditorMenu,
  MODE_MainMenu,
  MODE_Gameplay,
  MODE_FlyCam,
};

struct game_state
{
  Memory::stack_allocator* PersistentMemStack;
  Memory::stack_allocator* TemporaryMemStack;

  EditAnimation::animation_editor AnimEditor;

  Render::model* SphereModel;
  Render::model* QuadModel;
  Render::model* CharacterModel;
  Render::model* GizmoModel;
  Render::model* Cubemap;

  int32_t Textures[TEXTURE_MAX_COUNT];
  int32_t TextureCount;
  int32_t CollapsedTexture;
  int32_t ExpandedTexture;

  sized_font Fonts[10];
  int32_t    FontCount;

  text_texture TextTextures[50];
  int32_t      TextTextureCount;

  char    TextBuffer[1024];
  int32_t CurrentCharIndex;

  // int32_t Shaders;
  int32_t ShaderPhong;
  int32_t ShaderSkeletalPhong;
  int32_t ShaderSkeletalBoneColor;
  int32_t ShaderWireframe;
  int32_t ShaderColor;
  int32_t ShaderGizmo;
  int32_t ShaderQuad;
  int32_t ShaderTexturedQuad;
  int32_t ShaderCubemap;

  vec3  LightPosition;
  vec3  LightColor;
  float AmbientStrength;
  float SpecularStrength;

  uint32_t        MagicChecksum;
  Anim::transform ModelTransform;

  bool  DrawWireframe;
  bool  DrawCubemap;
  bool  DrawBoneWeights;
  bool  DrawTimeline;
  bool  DrawGizmos;
  bool  DrawText;
  bool  IsModelSpinning;
  bool  IsAnimationPlaying;
  float EditorBoneRotationSpeed;

  camera   Camera;
  uint32_t EngineMode;
  float    GameTime;

  loaded_wav AudioBuffer;
  bool       WAVLoaded;

  int32_t TextTexture;
  int32_t CubemapTexture;
};
