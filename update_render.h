#pragma once

#include "load_shader.h"

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "mesh.h"
#include "model.h"
#include "file_io.h"
#include "asset.h"
#include "builder/pack.h"
#include "load_bmp.h"
#include "misc.h"

#include "debug_drawing.h"
#include "camera.h"
#define demo 1

static const vec3 g_BoneColors[] = {
  { 0.41f, 0.93f, 0.23f }, { 0.14f, 0.11f, 0.80f }, { 0.35f, 0.40f, 0.77f },
  { 0.96f, 0.24f, 0.15f }, { 0.20f, 0.34f, 0.44f }, { 0.37f, 0.34f, 0.14f },
  { 0.22f, 0.99f, 0.77f }, { 0.80f, 0.70f, 0.11f }, { 0.81f, 0.92f, 0.18f },
  { 0.51f, 0.86f, 0.13f }, { 0.80f, 0.94f, 0.10f }, { 0.70f, 0.42f, 0.52f },
  { 0.26f, 0.50f, 0.61f }, { 0.10f, 0.21f, 0.81f }, { 0.96f, 0.22f, 0.63f },
  { 0.77f, 0.22f, 0.79f }, { 0.30f, 0.00f, 0.07f }, { 0.98f, 0.28f, 0.02f },
  { 0.92f, 0.42f, 0.14f }, { 0.47f, 0.31f, 0.72f },
};
static const float EDITOR_BONE_ROTATION_SPEED_DEG = 45.0f;

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  assert(GameMemory.HasBeenInitialized);
  //---------------------BEGIN INIT -------------------------
  if(GameState->MagicChecksum != 123456)
  {
    GameState->WAVLoaded     = false;
    GameState->MagicChecksum = 123456;
    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace((uint8_t*)GameMemory.PersistentMemory +
                                            sizeof(game_state),
                                          GameMemory.PersistentMemorySize - sizeof(game_state));
    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory,
                                          GameMemory.TemporaryMemorySize);

    // -------BEGIN ASSETS
    Memory::stack_allocator* TemporaryMemStack  = GameState->TemporaryMemStack;
    Memory::stack_allocator* PersistentMemStack = GameState->PersistentMemStack;

    // Set Up Gizmo
    debug_read_file_result AssetReadResult =
      ReadEntireFile(PersistentMemStack, "./data/gizmo.model");

    assert(AssetReadResult.Contents);
    Asset::asset_file_header* AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

    UnpackAsset(AssetHeader);
    GameState->GizmoModel = (Render::model*)AssetHeader->Model;
    for(int i = 0; i < GameState->GizmoModel->MeshCount; i++)
    {
      SetUpMesh(GameState->GizmoModel->Meshes[i]);
    }

// Set Up Gizmo
    AssetReadResult = ReadEntireFile(PersistentMemStack, "./data/debug_meshes.model");

    assert(AssetReadResult.Contents);
    AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;

    UnpackAsset(AssetHeader);
    GameState->QuadModel = (Render::model*)AssetHeader->Model;
    for(int i = 0; i < GameState->QuadModel->MeshCount; i++)
    {
      SetUpMesh(GameState->QuadModel->Meshes[i]);
    }

    // Set Up Model
#if demo
    AssetReadResult = ReadEntireFile(PersistentMemStack, "./data/soldier_test0.actor");
#else
    AssetReadResult = ReadEntireFile(PersistentMemStack, "./data/crysis_soldier.actor");
#endif

    assert(AssetReadResult.Contents);
    AssetHeader = (Asset::asset_file_header*)AssetReadResult.Contents;
    UnpackAsset(AssetHeader);
    GameState->CharacterModel = (Render::model*)AssetHeader->Model;
    for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
    {
      SetUpMesh(GameState->CharacterModel->Meshes[i]);
    }
    Render::PrintModelHeader(GameState->CharacterModel);

    GameState->Skeleton            = (Anim::skeleton*)AssetHeader->Skeleton;
    GameState->AnimEditor.Skeleton = (Anim::skeleton*)AssetHeader->Skeleton;

// Set Up Texture
#if demo
    AddTexture(GameState, Texture::LoadBMPTexture("./data/hand_dif.bmp"));
    AddTexture(GameState, Texture::LoadBMPTexture("./data/helmet_diff.bmp"));
    AddTexture(GameState, Texture::LoadBMPTexture("./data/glass_dif.bmp"));
    AddTexture(GameState, Texture::LoadBMPTexture("./data/body_dif.bmp"));
    AddTexture(GameState, Texture::LoadBMPTexture("./data/leg_dif.bmp"));
    AddTexture(GameState, Texture::LoadBMPTexture("./data/arm_dif.bmp"));
    AddTexture(GameState, Texture::LoadBMPTexture("./data/body_dif.bmp"));
#else
    AddTexture(GameState, Texture::LoadBMPTexture("./data/body_dif.bmp"));
#endif

    // -------BEGIN LOADING SHADERS
    // Diffuse
    Memory::marker LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderSkeletalPhong =
      Shader::LoadShader(TemporaryMemStack, "./shaders/skeletal_phong");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderSkeletalPhong < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    // Bone Color
    LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderSkeletalBoneColor =
      Shader::LoadShader(TemporaryMemStack, "./shaders/skeletal_bone_color");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderSkeletalBoneColor < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    // Wireframe
    LoadStart                  = TemporaryMemStack->GetMarker();
    GameState->ShaderWireframe = Shader::LoadShader(TemporaryMemStack, "./shaders/wireframe");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderWireframe < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    LoadStart              = TemporaryMemStack->GetMarker();
    GameState->ShaderGizmo = Shader::LoadShader(TemporaryMemStack, "./shaders/gizmo");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderGizmo < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    //
    // debug_ui
    LoadStart             = TemporaryMemStack->GetMarker();
    GameState->ShaderQuad = Shader::LoadShader(TemporaryMemStack, "./shaders/debug_quad");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderQuad < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }
    LoadStart = TemporaryMemStack->GetMarker();
    GameState->ShaderTexturedQuad =
      Shader::LoadShader(TemporaryMemStack, "./shaders/debug_textured_quad");
    TemporaryMemStack->FreeToMarker(LoadStart);
    if(GameState->ShaderTexturedQuad < 0)
    {
      printf("Shader loading failed!\n");
      assert(false);
    }

    // -------END ASSET LOADING
    // ======Set GL state
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // -------InitGameState
    GameState->ModelTransform.Rotation = { 0, 0, 0 };
    GameState->ModelTransform.Scale    = { 1, 1, 1 };
    GameState->Camera.Position         = { 0, 1, 2 };
    GameState->Camera.Up               = { 0, 1, 0 };
    GameState->Camera.Forward          = { 0, 0, -1 };
    GameState->Camera.Right            = { 1, 0, 0 };
    GameState->Camera.Rotation         = {};
    GameState->Camera.FieldOfView      = 70.0f;
    GameState->Camera.NearClipPlane    = 0.001f;
    GameState->Camera.FarClipPlane     = 100.0f;
    GameState->Camera.MaxTiltAngle     = 90.0f;

    GameState->LightPosition    = { 2.25f, 1.0f, 1.0f };
    GameState->LightColor       = { 0.7f, 0.7f, 0.75f };
    GameState->AmbientStrength  = 0.7f;
    GameState->SpecularStrength = 0.6f;

    GameState->DrawWireframe   = false;
    GameState->DrawBoneWeights = false;
    GameState->DrawGizmos      = false;
  }
  //---------------------END INIT -------------------------

  //----------------------UPDATE------------------------
  GameState->GameTime += Input->dt;

  UpdateCamera(&GameState->Camera, Input);
	GameState->ModelTransform.Rotation.Y += 45.0f * Input->dt;
  mat4 ModelMatrix = Math::MulMat4(Math::Mat4Rotate(GameState->ModelTransform.Rotation),
                                   Math::Mat4Scale(GameState->ModelTransform.Scale));
  mat4 MVPMatrix = Math::MulMat4(GameState->Camera.VPMatrix, ModelMatrix);
  //--------------ANIMAITION UPDATE

  if(Input->i.EndedDown && Input->i.Changed)
  {
    InsertBlendedKeyframeAtTime(&GameState->AnimEditor, GameState->AnimEditor.PlayHeadTime);
  }
  if(Input->LeftShift.EndedDown)
  {
    if(Input->x.EndedDown)
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.X -= EDITOR_BONE_ROTATION_SPEED_DEG * Input->dt;
    }
    if(Input->n.EndedDown && Input->n.Changed)
    {
      EditAnimation::EditPreviousBone(&GameState->AnimEditor);
    }
    if(Input->ArrowLeft.EndedDown && Input->ArrowLeft.Changed)
    {
      EditAnimation::JumpToPreviousKeyframe(&GameState->AnimEditor);
    }
    if(Input->ArrowRight.EndedDown && Input->ArrowRight.Changed)
    {
      EditAnimation::JumpToNextKeyframe(&GameState->AnimEditor);
    }
  }
  else
  {
    if(Input->x.EndedDown)
    {
      GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
        .Transforms[GameState->AnimEditor.CurrentBone]
        .Rotation.X += EDITOR_BONE_ROTATION_SPEED_DEG * Input->dt;
    }
    if(Input->n.EndedDown && Input->n.Changed)
    {
      EditAnimation::EditNextBone(&GameState->AnimEditor);
    }
    if(Input->ArrowLeft.EndedDown)
    {
      EditAnimation::AdvancePlayHead(&GameState->AnimEditor, -0.01f); // * Input->dt;
    }
    if(Input->ArrowRight.EndedDown)
    {
      EditAnimation::AdvancePlayHead(&GameState->AnimEditor, +0.01f); // * Input->dt;
    }
  }
  if(Input->m.EndedDown && Input->m.Changed)
  {
    EditAnimation::MoveCurrentKeyframeToPlayHead(&GameState->AnimEditor);
  }
  if(Input->Delete.EndedDown && Input->Delete.Changed)
  {
    EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
  }

  PrintAnimEditorState(&GameState->AnimEditor);

  if(GameState->AnimEditor.KeyframeCount > 0)
  {
    EditAnimation::CalculateHierarchicalmatricesAtTime(&GameState->AnimEditor);
  }

#if 1
#endif

  //---------------------RENDERING----------------------------
  if(Input->b.EndedDown && Input->b.Changed)
  {
    GameState->DrawBoneWeights = !GameState->DrawBoneWeights;
  }
  if(Input->g.EndedDown && Input->g.Changed)
  {
    GameState->DrawGizmos = !GameState->DrawGizmos;
  }
  if(Input->f.EndedDown && Input->f.Changed)
  {
    GameState->DrawWireframe = !GameState->DrawWireframe;
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if(GameState->DrawBoneWeights)
  {
    // Bone Color Shader
    glUseProgram(GameState->ShaderSkeletalBoneColor);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "mat_mvp"), 1,
                       GL_FALSE, MVPMatrix.e);
    glUniform3fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "g_bone_colors"), 20,
                 (float*)&g_BoneColors);
    glUniform1i(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "g_selected_bone_index"),
                GameState->AnimEditor.CurrentBone);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalBoneColor, "g_bone_matrices"),
                       20, GL_FALSE, (float*)GameState->AnimEditor.HierarchicalModelSpaceMatrices);
    for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
    {
      glBindVertexArray(GameState->CharacterModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->CharacterModel->Meshes[i]->IndiceCount,
                     GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
    }
  }
  else
  {
    // Regular Shader
    glUseProgram(GameState->ShaderSkeletalPhong);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "g_bone_matrices"), 20,
                       GL_FALSE, (float*)GameState->AnimEditor.HierarchicalModelSpaceMatrices);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderSkeletalPhong, "mat_model"), 1,
                       GL_FALSE, ModelMatrix.e);
    glUniform1f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "ambient_strength"),
                GameState->AmbientStrength);
    glUniform1f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "specular_strength"),
                GameState->SpecularStrength);
    glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "light_position"),
                GameState->LightPosition.X, GameState->LightPosition.Y, GameState->LightPosition.Z);
    glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "light_color"),
                GameState->LightColor.X, GameState->LightColor.Y, GameState->LightColor.Z);
    glUniform3f(glGetUniformLocation(GameState->ShaderSkeletalPhong, "camera_position"),
                GameState->Camera.Position.X, GameState->Camera.Position.Y,
                GameState->Camera.Position.Z);
    for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
    {
      glBindTexture(GL_TEXTURE_2D, GameState->Textures[i]);
      glBindVertexArray(GameState->CharacterModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->CharacterModel->Meshes[i]->IndiceCount,
                     GL_UNSIGNED_INT, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
  }
  if(GameState->DrawWireframe)
  {
    // WireframeShader Shader
    glUseProgram(GameState->ShaderWireframe);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUniformMatrix4fv(glGetUniformLocation(GameState->ShaderWireframe, "mat_mvp"), 1, GL_FALSE,
                       MVPMatrix.e);
    for(int i = 0; i < GameState->CharacterModel->MeshCount; i++)
    {
      glBindVertexArray(GameState->CharacterModel->Meshes[i]->VAO);
      glDrawElements(GL_TRIANGLES, GameState->CharacterModel->Meshes[i]->IndiceCount,
                     GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  if(GameState->DrawGizmos && GameState->Skeleton)
  {
    mat4 BoneGizmos[SKELETON_MAX_BONE_COUNT];
    for(int i = 0; i < GameState->Skeleton->BoneCount; i++)
    {
      BoneGizmos[i] =
        Math::MulMat4(ModelMatrix,
                      Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices[i],
                                    GameState->Skeleton->Bones[i].BindPose));
    }

    DEBUGDrawGizmos(GameState, &ModelMatrix, 1);
    DEBUGDrawGizmos(GameState, BoneGizmos, GameState->Skeleton->BoneCount);
  }

  // Drawing animation editor timeline
  {
    const int KeyframeCount = GameState->AnimEditor.KeyframeCount;
    if(KeyframeCount > 0)
    {
      const EditAnimation::animation_editor* Editor = &GameState->AnimEditor;

      const float TimelineStartX = 0.20f;
      const float TimelineStartY = 0.1f;
      const float TimelineWidth  = 0.6f;
      const float TimelineHeight = 0.05f;
      const float FirstTime      = MinFloat(Editor->PlayHeadTime, Editor->SampleTimes[0]);
      const float LastTime =
        MaxFloat(Editor->PlayHeadTime, Editor->SampleTimes[Editor->KeyframeCount - 1]);
      float KeyframeSpacing = KEYFRAME_MIN_TIME_DIFFERENCE_APART * 0.5f; // seconds

      float TimeDiff = MaxFloat((LastTime - FirstTime), 1.0f);

      float KeyframeWidth = KeyframeSpacing / TimeDiff;

      DEBUGDrawQuad(GameState, vec3{ TimelineStartX, TimelineStartY }, TimelineWidth,
                    TimelineHeight);
      for(int i = 0; i < KeyframeCount; i++)
      {
        float PosPercentage = (Editor->SampleTimes[i] - FirstTime) / TimeDiff;
        DEBUGDrawCenteredQuad(GameState,
                              vec3{ TimelineStartX + PosPercentage * TimelineWidth,
                                    TimelineStartY + TimelineHeight * 0.5f },
                              KeyframeWidth, 0.05f, { 0.5f, 0.3f, 0.3f });
      }
      float PlayheadPercentage = (Editor->PlayHeadTime - FirstTime) / TimeDiff;
      DEBUGDrawCenteredQuad(GameState,
                            vec3{ TimelineStartX + PlayheadPercentage * TimelineWidth,
                                  TimelineStartY + TimelineHeight * 0.5f },
                            KeyframeWidth, 0.05f, { 1, 0, 0 });
      float CurrentPercentage =
        (Editor->SampleTimes[Editor->CurrentKeyframe] - FirstTime) / TimeDiff;
      DEBUGDrawCenteredQuad(GameState,
                            vec3{ TimelineStartX + CurrentPercentage * TimelineWidth,
                                  TimelineStartY + TimelineHeight * 0.5f },
                            0.003f, 0.05f, { 0.5f, 0.5f, 0.5f });
    }
  }
}

