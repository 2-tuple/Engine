#pragma once

#include "load_shader.h"

#include "linear_math/matrix.h"
#include "linear_math/vector.h"

#include "game.h"
#include "mesh.h"
#include "model.h"
#include "file_io.h"
#include "asset.h"
#include "load_texture.h"
#include "misc.h"
#include "intersection_testing.h"
#include "load_asset.h"
#include "render_data.h"
#include "material_upload.h"

#include "text.h"

#include "debug_drawing.h"
#include "camera.h"

#include "editor_ui.h"
#include <limits.h>

#include "file_tree_walk.h"

mat4
GetEntityModelMatrix(game_state* GameState, int32_t EntityIndex)
{
  mat4 ModelMatrix = TransformToMat4(&GameState->Entities[EntityIndex].Transform);
  return ModelMatrix;
}

mat4
GetEntityMVPMatrix(game_state* GameState, int32_t EntityIndex)
{
  mat4 ModelMatrix = GetEntityModelMatrix(GameState, EntityIndex);
  mat4 MVPMatrix   = Math::MulMat4(GameState->Camera.VPMatrix, ModelMatrix);
  return MVPMatrix;
}

void
AddEntity(game_state* GameState, rid ModelID, int32_t* MaterialIndices, Anim::transform Transform)
{
  assert(0 <= GameState->EntityCount && GameState->EntityCount < ENTITY_MAX_COUNT);

  entity NewEntity          = {};
  NewEntity.ModelID         = ModelID;
  NewEntity.MaterialIndices = MaterialIndices;
  NewEntity.Transform       = Transform;

  GameState->Entities[GameState->EntityCount++] = NewEntity;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  assert(GameMemory.HasBeenInitialized);
  //---------------------BEGIN INIT -------------------------
  if(GameState->MagicChecksum != 123456)
  {
    GameState->WAVLoaded     = false;
    GameState->MagicChecksum = 123456;

    // SEGMENT MEMORY
    uint32_t PersistentStackSize =
      (uint32_t)(((float)GameMemory.PersistentMemorySize * 0.5f) - sizeof(game_state));
    uint32_t ModelStackSize = GameMemory.PersistentMemorySize - PersistentStackSize;
    assert(0 < ModelStackSize);

    uint8_t* PersistentStackStart = (uint8_t*)GameMemory.PersistentMemory + sizeof(game_state);
    uint8_t* ModelStackStart      = PersistentStackStart + PersistentStackSize;

    GameState->TemporaryMemStack =
      Memory::CreateStackAllocatorInPlace(GameMemory.TemporaryMemory,
                                          GameMemory.TemporaryMemorySize);

    GameState->PersistentMemStack =
      Memory::CreateStackAllocatorInPlace(PersistentStackStart, PersistentStackSize);

    GameState->Resources.ModelStack.Create(ModelStackStart, ModelStackSize);

// END MEMORY SEGMENTATION

#if 0
    CheckedLoadAndSetUpModel(PersistentMemStack, "./data/built/sponza.model", &TempModelPtr);
    AddModel(&GameState->R, TempModelPtr);
#endif
    // --------LOAD MODELS/ACTORS--------

    GameState->GizmoModelID = GameState->Resources.RegisterModel("./data/built/gizmo1.model");
    GameState->QuadModelID  = GameState->Resources.RegisterModel("./data/built/debug_meshes.model");
    GameState->CubemapModelID =
      GameState->Resources.RegisterModel("./data/built/inverse_cube.model");
    GameState->SphereModelID   = GameState->Resources.RegisterModel("./data/built/sphere.model");
    GameState->UVSphereModelID = GameState->Resources.RegisterModel("./data/built/uv_sphere.model");

    AddModel(&GameState->R, GameState->Resources.RegisterModel("./data/built/armadillo.model"));
    AddModel(&GameState->R,
             GameState->Resources.RegisterModel("./data/built/multimesh_soldier.actor"));
    AddModel(&GameState->R, GameState->Resources.RegisterModel("./data/built/sponza1.model"));

    // -----------LOAD SHADERS------------
    GameState->R.ShaderPhong =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "./shaders/phong");
    GameState->R.ShaderCubemap =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "./shaders/cubemap");
    GameState->R.ShaderGizmo =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "./shaders/gizmo");
    GameState->R.ShaderQuad =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "./shaders/debug_quad");
    GameState->R.ShaderColor =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "./shaders/color");
    GameState->R.ShaderTexturedQuad =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "./shaders/debug_textured_quad");
    GameState->R.ShaderID =
      CheckedLoadCompileFreeShader(GameState->TemporaryMemStack, "./shaders/id");
    //------------LOAD TEXTURES-----------
    GameState->CollapsedTextureID = Texture::LoadTexture("./data/textures/collapsed.bmp");
    GameState->ExpandedTextureID  = Texture::LoadTexture("./data/textures/expanded.bmp");
    assert(GameState->CollapsedTextureID);
    assert(GameState->ExpandedTextureID);

    GameState->Font = Text::LoadFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14, 8, 1);

    // ======Set GL state
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    // Create index framebuffer for mesh picking
    glGenFramebuffers(1, &GameState->IndexFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
    glGenTextures(1, &GameState->IDTexture);
    glBindTexture(GL_TEXTURE_2D, GameState->IDTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA,
                 GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           GameState->IDTexture, 0);
    glGenRenderbuffers(1, &GameState->DepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, GameState->DepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREEN_WIDTH, SCREEN_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              GameState->DepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      assert(0 && "error: incomplete frambuffer!\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // -------InitGameState
    GameState->Camera.Position      = { 0, 1.6f, 2 };
    GameState->Camera.Up            = { 0, 1, 0 };
    GameState->Camera.Forward       = { 0, 0, -1 };
    GameState->Camera.Right         = { 1, 0, 0 };
    GameState->Camera.Rotation      = { 0 };
    GameState->Camera.NearClipPlane = 0.001f;
    GameState->Camera.FarClipPlane  = 1000.0f;
    GameState->Camera.FieldOfView   = 70.0f;
    GameState->Camera.MaxTiltAngle  = 90.0f;
    GameState->Camera.Speed         = 2.0f;

    GameState->PreviewCamera          = GameState->Camera;
    GameState->PreviewCamera.Position = { 0, 0, 3 };
    GameState->PreviewCamera.Rotation = {};
    UpdateCamera(&GameState->PreviewCamera, Input);

    GameState->R.LightPosition = { 0.7f, 1, 1 };

    GameState->R.LightSpecularColor = { 1, 1, 1 };
    GameState->R.LightDiffuseColor  = { 1, 1, 1 };
    GameState->R.LightAmbientColor  = { 0.3f, 0.3f, 0.3f };
    GameState->R.ShowLightPosition  = true;

    GameState->DrawCubemap             = false;
    GameState->DrawTimeline            = true;
    GameState->DrawGizmos              = true;
    GameState->IsAnimationPlaying      = false;
    GameState->EditorBoneRotationSpeed = 45.0f;

    {
      AddMaterial(&GameState->R, NewPhongMaterial());
    }
  }
  //---------------------END INIT -------------------------

  //----------------------UPDATE------------------------
  UpdateCamera(&GameState->Camera, Input);

  if(Input->IsMouseInEditorMode)
  {
    // GUI
    IMGUIControlPanel(GameState, Input);

    // ANIMATION TIMELINE
    if(GameState->SelectionMode == SELECT_Bone && GameState->DrawTimeline &&
       GameState->AnimEditor.Skeleton)
    {
      VisualizeTimeline(GameState);
    }

    // Selection
    if(Input->MouseRight.EndedDown && Input->MouseRight.Changed)
    {
      // Draw entities to ID buffer
      // SORT_MESH_INSTANCES(ByEntity);
      glDisable(GL_BLEND);
      glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
      glClearColor(0.9f, 0.9f, 0.9f, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glUseProgram(GameState->R.ShaderID);
      for(int e = 0; e < GameState->EntityCount; e++)
      {
        entity* CurrentEntity;
        assert(GetEntityAtIndex(GameState, &CurrentEntity, e));
        glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "mat_mvp"), 1, GL_FALSE,
                           GetEntityMVPMatrix(GameState, e).e);
        if(CurrentEntity->AnimController)
        {
          glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "g_boneMatrices"),
                             CurrentEntity->AnimController->Skeleton->BoneCount, GL_FALSE,
                             (float*)CurrentEntity->AnimController->HierarchicalModelSpaceMatrices);
        }
        else
        {
          mat4 Mat4Zeros = {};
          glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderID, "g_boneMatrices"), 1,
                             GL_FALSE, Mat4Zeros.e);
        }
        if(GameState->SelectionMode == SELECT_Mesh)
        {
          Render::mesh* SelectedMesh = {};
          if(GetSelectedMesh(GameState, &SelectedMesh))
          {
            glBindVertexArray(SelectedMesh->VAO);

            glDrawElements(GL_TRIANGLES, SelectedMesh->IndiceCount, GL_UNSIGNED_INT, 0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          }
        }
        Render::model* CurrentModel = GameState->Resources.GetModel(GameState->Entities[e].ModelID);
        for(int m = 0; m < CurrentModel->MeshCount; m++)
        {
          glBindVertexArray(CurrentModel->Meshes[m]->VAO);
          assert(e < USHRT_MAX);
          assert(m < USHRT_MAX);
          uint16_t EntityID = (uint16_t)e;
          uint16_t MeshID   = (uint16_t)m;
          uint32_t R        = (EntityID & 0x00FF) >> 0;
          uint32_t G        = (EntityID & 0xFF00) >> 8;
          uint32_t B        = (MeshID & 0x00FF) >> 0;
          uint32_t A        = (MeshID & 0xFF00) >> 8;

          vec4 EntityColorID = { (float)R / 255.0f, (float)G / 255.0f, (float)B / 255.0f,
                                 (float)A / 255.0f };
          glUniform4fv(glGetUniformLocation(GameState->R.ShaderID, "g_id"), 1,
                       (float*)&EntityColorID);
          glDrawElements(GL_TRIANGLES, CurrentModel->Meshes[m]->IndiceCount, GL_UNSIGNED_INT, 0);
        }
      }
      glFlush();
      glFinish();
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      uint16_t IDColor[2] = {};
      glReadPixels(Input->MouseX, Input->MouseY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, IDColor);
      GameState->SelectedEntityIndex = (uint32_t)IDColor[0];
      GameState->SelectedMeshIndex   = (uint32_t)IDColor[1];
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if(GameState->IsEntityCreationMode && Input->MouseLeft.EndedDown && Input->MouseLeft.Changed)
    {
      GameState->IsEntityCreationMode = false;
      vec3 RayDir =
        GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                             GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
      raycast_result RaycastResult =
        RayIntersectPlane(GameState->Camera.Position, RayDir, {}, { 0, 1, 0 });
      if(RaycastResult.Success && GameState->R.ModelCount > 0)
      {
        Anim::transform NewTransform = {};
        NewTransform.Translation     = RaycastResult.IntersectP;
        NewTransform.Scale           = { 1, 1, 1 };

        if(GameState->R.ModelCount > 0)
        {
          Render::model* Model =
            GameState->Resources.GetModel(GameState->R.Models[GameState->CurrentModelIndex]);
          int32_t* MaterialIndices =
            PushArray(GameState->PersistentMemStack, Model->MeshCount, int32_t);
          if(0 <= GameState->CurrentMaterialIndex &&
             GameState->CurrentMaterialIndex < MATERIAL_MAX_COUNT)
          {
            for(int m = 0; m < Model->MeshCount; m++)
            {
              MaterialIndices[m] = GameState->CurrentMaterialIndex;
            }
          }
          AddEntity(GameState, GameState->R.Models[GameState->CurrentModelIndex], MaterialIndices,
                    NewTransform);
        }
      }
    }
  }

  // -----------ENTITY ANIMATION UPDATE-------------
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    Anim::animation_controller* Controller = GameState->Entities[e].AnimController;
    if(Controller)
    {
      Anim::UpdateController(Controller, Input->dt);
    }
  }

  //----------ANIMATION EDITOR INTERACTION-----------
  if(Input->IsMouseInEditorMode && GameState->SelectionMode == SELECT_Bone &&
     GameState->AnimEditor.Skeleton)
  {
    if(Input->Space.EndedDown && Input->Space.Changed)
    {
      GameState->IsAnimationPlaying = !GameState->IsAnimationPlaying;
    }
    if(GameState->IsAnimationPlaying)
    {
      EditAnimation::PlayAnimation(&GameState->AnimEditor, Input->dt);
    }
    if(Input->i.EndedDown && Input->i.Changed)
    {
      InsertBlendedKeyframeAtTime(&GameState->AnimEditor, GameState->AnimEditor.PlayHeadTime);
    }
    if(Input->LeftShift.EndedDown)
    {
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
      if(Input->n.EndedDown && Input->n.Changed)
      {
        EditAnimation::EditNextBone(&GameState->AnimEditor);
      }
      if(Input->ArrowLeft.EndedDown)
      {
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, -1 * Input->dt);
      }
      if(Input->ArrowRight.EndedDown)
      {
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 1 * Input->dt);
      }
    }
    if(Input->LeftCtrl.EndedDown)
    {
      if(Input->c.EndedDown && Input->c.Changed)
      {
        EditAnimation::CopyKeyframeToClipboard(&GameState->AnimEditor,
                                               GameState->AnimEditor.CurrentKeyframe);
      }
      else if(Input->x.EndedDown && Input->x.Changed)
      {
        EditAnimation::CopyKeyframeToClipboard(&GameState->AnimEditor,
                                               GameState->AnimEditor.CurrentKeyframe);
        DeleteCurrentKeyframe(&GameState->AnimEditor);
      }
      else if(Input->v.EndedDown && Input->v.Changed && GameState->AnimEditor.Skeleton)
      {
        EditAnimation::InsertKeyframeFromClipboardAtTime(&GameState->AnimEditor,
                                                         GameState->AnimEditor.PlayHeadTime);
      }
    }
    if(Input->Delete.EndedDown && Input->Delete.Changed)
    {
      EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
    }
    if(GameState->AnimEditor.KeyframeCount > 0)
    {
      EditAnimation::CalculateHierarchicalmatricesAtTime(&GameState->AnimEditor);
    }

    // Bone Selection
    for(int i = 0; i < GameState->AnimEditor.Skeleton->BoneCount; i++)
    {
      mat4 Mat4Bone =
        Math::MulMat4(TransformToMat4(GameState->AnimEditor.Transform),
                      Math::MulMat4(GameState->AnimEditor.HierarchicalModelSpaceMatrices[i],
                                    GameState->AnimEditor.Skeleton->Bones[i].BindPose));

      const float BoneSphereRadius = 0.1f;

      vec3 Position = Math::GetMat4Translation(Mat4Bone);
      vec3 RayDir =
        GetRayDirFromScreenP({ Input->NormMouseX, Input->NormMouseY },
                             GameState->Camera.ProjectionMatrix, GameState->Camera.ViewMatrix);
      raycast_result RaycastResult =
        RayIntersectSphere(GameState->Camera.Position, RayDir, Position, BoneSphereRadius);
      if(RaycastResult.Success)
      {
        Debug::PushWireframeSphere(&GameState->Camera, Position, BoneSphereRadius, { 1, 1, 0, 1 });
        if(Input->MouseRight.EndedDown && Input->MouseRight.Changed)
        {
          EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, i);
        }
      }
      else
      {
        Debug::PushWireframeSphere(&GameState->Camera, Position, BoneSphereRadius);
      }

      if(i == GameState->AnimEditor.CurrentBone)
      {
        Debug::PushGizmo(&GameState->Camera, &Mat4Bone);
      }
    }
    // Copy editor poses to entity anim controller
    assert(0 <= GameState->AnimEditor.EntityIndex &&
           GameState->AnimEditor.EntityIndex < GameState->EntityCount);
    {
      memcpy(GameState->Entities[GameState->AnimEditor.EntityIndex]
               .AnimController->HierarchicalModelSpaceMatrices,
             GameState->AnimEditor.HierarchicalModelSpaceMatrices,
             sizeof(mat4) * GameState->AnimEditor.Skeleton->BoneCount);
    }
  }
  //---------------------RENDERING----------------------------

  GameState->R.MeshInstanceCount = 0;
  // Put entiry data to darw drawing queue every frame to avoid erroneous indirection due to
  // sorting
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    Render::model* CurrentModel = GameState->Resources.GetModel(GameState->Entities[e].ModelID);
    for(int m = 0; m < CurrentModel->MeshCount; m++)
    {
      mesh_instance MeshInstance = {};
      MeshInstance.Mesh          = CurrentModel->Meshes[m];
      MeshInstance.Material    = &GameState->R.Materials[GameState->Entities[e].MaterialIndices[m]];
      MeshInstance.EntityIndex = e;
      AddMeshInstance(&GameState->R, MeshInstance);
    }
  }

  {
    // Draw scene to backbuffer
    // SORT(MeshInstances, ByMaterial, MyMesh);
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    material*     PreviousMaterial = nullptr;
    Render::mesh* PreviousMesh     = nullptr;
    uint32_t      CurrentShaderID  = 0;
    for(int i = 0; i < GameState->R.MeshInstanceCount; i++)
    {
      material*     CurrentMaterial    = GameState->R.MeshInstances[i].Material;
      Render::mesh* CurrentMesh        = GameState->R.MeshInstances[i].Mesh;
      int           CurrentEntityIndex = GameState->R.MeshInstances[i].EntityIndex;
      if(CurrentMaterial != PreviousMaterial)
      {
        if(PreviousMaterial)
        {
          glBindTexture(GL_TEXTURE_2D, 0);
          glBindVertexArray(0);
        }
        CurrentShaderID = SetMaterial(&GameState->R, &GameState->Camera, CurrentMaterial);

        PreviousMaterial = CurrentMaterial;
      }
      if(CurrentMesh != PreviousMesh)
      {
        glBindVertexArray(CurrentMesh->VAO);
        PreviousMesh = CurrentMesh;
      }
      if(CurrentMaterial->Common.IsSkeletal &&
         GameState->Entities[CurrentEntityIndex].AnimController)
      {
        glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "g_boneMatrices"),
                           GameState->Entities[CurrentEntityIndex]
                             .AnimController->Skeleton->BoneCount,
                           GL_FALSE,
                           (float*)GameState->Entities[CurrentEntityIndex]
                             .AnimController->HierarchicalModelSpaceMatrices);
      }
      else
      {
        mat4 Mat4Zeros = {};
        glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "g_boneMatrices"), 1, GL_FALSE,
                           Mat4Zeros.e);
      }
      glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_mvp"), 1, GL_FALSE,
                         GetEntityMVPMatrix(GameState, CurrentEntityIndex).e);
      glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_model"), 1, GL_FALSE,
                         GetEntityModelMatrix(GameState, CurrentEntityIndex).e);
      glDrawElements(GL_TRIANGLES, CurrentMesh->IndiceCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
  }

  // Higlight entity
  entity* SelectedEntity;
  if(Input->IsMouseInEditorMode && GetSelectedEntity(GameState, &SelectedEntity))
  {
    // Higlight mesh
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDepthFunc(GL_LEQUAL);
    vec4 ColorRed = vec4{ 1, 1, 0, 1 };
    glUseProgram(GameState->R.ShaderColor);
    glUniform4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_color"), 1, (float*)&ColorRed);
    glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "mat_mvp"), 1, GL_FALSE,
                       GetEntityMVPMatrix(GameState, GameState->SelectedEntityIndex).e);
    if(SelectedEntity->AnimController)
    {
      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_boneMatrices"),
                         SelectedEntity->AnimController->Skeleton->BoneCount, GL_FALSE,
                         (float*)SelectedEntity->AnimController->HierarchicalModelSpaceMatrices);
    }
    else
    {
      mat4 Mat4Zeros = {};
      glUniformMatrix4fv(glGetUniformLocation(GameState->R.ShaderColor, "g_boneMatrices"), 1,
                         GL_FALSE, Mat4Zeros.e);
    }
    if(GameState->SelectionMode == SELECT_Mesh)
    {
      Render::mesh* SelectedMesh = {};
      if(GetSelectedMesh(GameState, &SelectedMesh))
      {
        glBindVertexArray(SelectedMesh->VAO);

        glDrawElements(GL_TRIANGLES, SelectedMesh->IndiceCount, GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }
    }
    // Highlight Entity
    else if(GameState->SelectionMode == SELECT_Entity)
    {
      entity* SelectedEntity = {};
      if(GetSelectedEntity(GameState, &SelectedEntity))
      {
        mat4 Mat4EntityTransform = TransformToMat4(&SelectedEntity->Transform);
        Debug::PushGizmo(&GameState->Camera, &Mat4EntityTransform);
        Render::model* Model = GameState->Resources.GetModel(SelectedEntity->ModelID);
        for(int m = 0; m < Model->MeshCount; m++)
        {
          glBindVertexArray(Model->Meshes[m]->VAO);
          glDrawElements(GL_TRIANGLES, Model->Meshes[m]->IndiceCount, GL_UNSIGNED_INT, 0);
        }
      }
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  // Draw material preview to texture
  {
    material* PreviewMaterial = &GameState->R.Materials[GameState->CurrentMaterialIndex];
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->IndexFBO);
    glClearColor(0.7f, 0.7f, 0.7f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    uint32_t ShaderID = SetMaterial(&GameState->R, &GameState->PreviewCamera, PreviewMaterial);

    if(PreviewMaterial->Common.IsSkeletal)
    {
      mat4 Mat4Zeros = {};
      glUniformMatrix4fv(glGetUniformLocation(ShaderID, "g_boneMatrices"), 1, GL_FALSE,
                         Mat4Zeros.e);
    }
    glEnable(GL_BLEND);
    mat4           PreviewSphereMatrix = Math::Mat4Ident();
    Render::model* UVSphereModel       = GameState->Resources.GetModel(GameState->UVSphereModelID);
    glBindVertexArray(UVSphereModel->Meshes[0]->VAO);
    glUniformMatrix4fv(glGetUniformLocation(ShaderID, "mat_mvp"), 1, GL_FALSE,
                       Math::MulMat4(GameState->PreviewCamera.VPMatrix, Math::Mat4Ident()).e);
    glUniformMatrix4fv(glGetUniformLocation(ShaderID, "mat_model"), 1, GL_FALSE,
                       PreviewSphereMatrix.e);

    glDrawElements(GL_TRIANGLES, UVSphereModel->Meshes[0]->IndiceCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  Debug::DrawWireframeSpheres(GameState);
  glClear(GL_DEPTH_BUFFER_BIT);
  Debug::DrawGizmos(GameState);
  Debug::DrawColoredQuads(GameState);
  Debug::DrawTexturedQuads(GameState);
  Text::ClearTextRequestCounts();
}
