#pragma once

#include "game.h"
void MainMenuBar(bool* ShowRenderWindow, bool* ShowEntityWindow, bool* ShowMaterialWindow,
                 bool* ShowSettingsWindow, bool* ShowImGuiDemoWindow);
void RenderWindow(game_state* GameState);
void EntityEditor(game_state* GameState);
void MaterialEditor(game_state* GameState);
void SettingsEditor(game_state* GameState);

bool
PathArrayToString(void* Data, int Index, const char** OutStrPtr)
{
  path* Paths = (path*)Data;
  *OutStrPtr  = Paths[Index].Name;
  return true;
}

void
RunEditor(game_state* GameState, const game_input* Input)
{
  static bool s_ShowRenderWindow    = false;
  static bool s_ShowEntityWindow    = false;
  static bool s_ShowMaterialWindow  = false;
  static bool s_ShowSettingsWindow  = false;
  static bool s_ShowImGuiDemoWindow = false;
  MainMenuBar(&s_ShowRenderWindow, &s_ShowEntityWindow, &s_ShowMaterialWindow,
              &s_ShowSettingsWindow, &s_ShowImGuiDemoWindow);
  if(s_ShowImGuiDemoWindow)
  {
    TIMED_BLOCK(ImGuiDemo);
    ImGui::ShowDemoWindow();
  }
  if(s_ShowRenderWindow)
  {
    RenderWindow(GameState);
  }
  if(s_ShowEntityWindow)
  {
    EntityEditor(GameState);
  }
  if(s_ShowMaterialWindow)
  {
    MaterialEditor(GameState);
  }
  if(s_ShowSettingsWindow)
  {
    SettingsEditor(GameState);
  }
}

void
MainMenuBar(bool* ShowRenderWindow, bool* ShowEntityWindow, bool* ShowMaterialWindow,
            bool* ShowSettingsWindow, bool* ShowImGuiDemoWindow)
{
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("File"))
    {
      ImGui::MenuItem("LoadScene", NULL);
      ImGui::MenuItem("Export Scene", NULL);
      ImGui::MenuItem("Export Scene As", NULL);
      ImGui::EndMenu();
    }
    if(ImGui::BeginMenu("Window"))
    {
      ImGui::MenuItem("Render Window", NULL, ShowRenderWindow);
      ImGui::MenuItem("Entity Window", NULL, ShowEntityWindow);
      ImGui::MenuItem("Material Window", NULL, ShowMaterialWindow);
      ImGui::MenuItem("Settings Window", NULL, ShowSettingsWindow);
      if(ImGui::BeginMenu("ImGui"))
      {
        ImGui::MenuItem("ImGui Demo", NULL, ShowImGuiDemoWindow);
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void
RenderWindow(game_state* GameState)
{
  ImGui::Begin("Render Window");
  static int DrawnTextureIndex = 0;
  ImGui::
    Combo("What To Draw", &DrawnTextureIndex,
          "Game Image\0Shadow Cascade\0Normals\0Depth\0World Position\0SSAO\0Volumetric Lighting");
  float ImageWidth  = ImGui::GetWindowWidth();
  float ImageHeight = ImageWidth * (float(SCREEN_HEIGHT) / float(SCREEN_WIDTH));
  ImGui::Image((void*)(uint64_t)GameState->R.SSAOTexID, ImVec2(ImageWidth, ImageHeight));
  ImGui::End();
}

void
EntityEditor(game_state* GameState)
{
  ImGui::Begin("EntityEditor");
  entity* SelectedEntity = {};
  GetSelectedEntity(GameState, &SelectedEntity);

  if(SelectedEntity)
  {
    if(ImGui::Button("Delete Entity"))
    {
      DeleteEntity(GameState, &GameState->Resources, GameState->SelectedEntityIndex);
      GameState->SelectedEntityIndex = -1;
    }
  }
  if(ImGui::Button("Create"))
  {
    GameState->IsEntityCreationMode = !GameState->IsEntityCreationMode;
  }
  ImGui::SameLine();

  {
    static int32_t ActivePathIndex = 0;
    ImGui::Combo("Model Asset", &ActivePathIndex, PathArrayToString,
                 GameState->Resources.ModelPaths, GameState->Resources.ModelPathCount);
    {
      rid NewRID = { 0 };
      if(!GameState->Resources
            .GetModelPathRID(&NewRID, GameState->Resources.ModelPaths[ActivePathIndex].Name))
      {
        NewRID =
          GameState->Resources.RegisterModel(GameState->Resources.ModelPaths[ActivePathIndex].Name);
        GameState->CurrentModelID = NewRID;
      }
      else
      {
        GameState->CurrentModelID = NewRID;
      }
    }
  }

#if 1
  if(SelectedEntity)
  {
    // static bool s_ShowTransformComponent = false;
    if(ImGui::TreeNode("Transform Component"))
    {
      transform* Transform = &SelectedEntity->Transform;
      ImGui::DragFloat3("Translation", (float*)&Transform->T, -INFINITY, INFINITY, 10);
      // ImGui::DragFloat3("Rotation", (float*)&Transform->Rotation, -INFINITY, INFINITY, 720.0f);
      ImGui::DragFloat3("Scale", (float*)&Transform->S, -INFINITY, INFINITY, 10.0f);
      ImGui::TreePop();
    }

    // static bool s_ShowPhysicsComponent = false;
    if(ImGui::TreeNode("Physics Component"))
    // Rigid Body
    {
      rigid_body* RB = &SelectedEntity->RigidBody;
      // ImGui::DragFloat3("X", &RB->X.X, -INFINITY, INFINITY, 10);

      if(FloatsEqualByThreshold(Math::Length(RB->q), 0.0f, 0.00001f))
      {
        RB->q.S = 1;
        RB->q.V = {};
      }
      // ImGui::DragFloat4("q", &RB->q.S, -INFINITY, INFINITY, 10);
      // Math::Normalize(&RB->q);

      if(ImGui::Button("Clear v"))
      {
        RB->v = {};
      }
      ImGui::SameLine();
      ImGui::DragFloat3("v", &RB->v.X, -INFINITY, INFINITY, 10);
      if(ImGui::Button("Clear w"))
      {
        RB->w = {};
      }
      ImGui::SameLine();
      ImGui::DragFloat3("w", &RB->w.X, -INFINITY, INFINITY, 10);

      ImGui::Checkbox("Regard Gravity", &RB->RegardGravity);

      ImGui::DragFloat("Mass", &RB->Mass, 0, INFINITY, 10);
      if(0 < RB->Mass)
      {
        RB->MassInv = 1.0f / RB->Mass;
      }
      else
      {
        RB->MassInv = 0;
      }
      char TempBuffer[40];
      snprintf(TempBuffer, sizeof(TempBuffer), "Mass Inv.: %f", (double)RB->MassInv);
      ImGui::Text(TempBuffer);

      { // Inertia
        vec3 InertiaDiagonal = { RB->InertiaBody._11, RB->InertiaBody._22, RB->InertiaBody._33 };
        ImGui::DragFloat3("Body Space Inertia diagonal", &InertiaDiagonal.X, 0, INFINITY, 10);
        RB->InertiaBody     = {};
        RB->InertiaBody._11 = InertiaDiagonal.X;
        RB->InertiaBody._22 = InertiaDiagonal.Y;
        RB->InertiaBody._33 = InertiaDiagonal.Z;

        RB->InertiaBodyInv = {};
        if(InertiaDiagonal != vec3{})
        {
          RB->InertiaBodyInv._11 = 1.0f / InertiaDiagonal.X;
          RB->InertiaBodyInv._22 = 1.0f / InertiaDiagonal.Y;
          RB->InertiaBodyInv._33 = 1.0f / InertiaDiagonal.Z;
        }
      }
      ImGui::TreePop();
    }

    Render::model* SelectedModel = GameState->Resources.GetModel(SelectedEntity->ModelID);
    if(SelectedModel->Skeleton)
    {
      if(!SelectedEntity->AnimPlayer && ImGui::Button("Add Animation Player"))
      {
        SelectedEntity->AnimPlayer =
          PushStruct(GameState->PersistentMemStack, Anim::animation_player);
        *SelectedEntity->AnimPlayer = {};

        SelectedEntity->AnimPlayer->Skeleton = SelectedModel->Skeleton;
        SelectedEntity->AnimPlayer->OutputTransforms =
          PushArray(GameState->PersistentMemStack,
                    ANIM_PLAYER_OUTPUT_BLOCK_COUNT * SelectedModel->Skeleton->BoneCount, transform);
        SelectedEntity->AnimPlayer->BoneSpaceMatrices =
          PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
        SelectedEntity->AnimPlayer->ModelSpaceMatrices =
          PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
        SelectedEntity->AnimPlayer->HierarchicalModelSpaceMatrices =
          PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
      }
      else if(SelectedEntity->AnimPlayer)
      {
        bool ShowAnimtionPlayerComponent = false;
        bool RemovedAnimPlayer           = false;

        ShowAnimtionPlayerComponent = ImGui::TreeNode("Animation Player Component");
        if(GameState->SelectedEntityIndex != GameState->AnimEditor.EntityIndex)
        {
          ImGui::PushID("Remove Anim Player");
          ImGui::SameLine();
          if(ImGui::Button("Remove"))
          {
            RemovedAnimPlayer = true;
          }
          ImGui::PopID();
        }

        if(ShowAnimtionPlayerComponent)
        {
#if 0
            if(ImGui::Button("Animate Selected Entity"))
            {
              GameState->SelectionMode = SELECT_Bone;
              AttachEntityToAnimEditor(GameState, &GameState->AnimEditor,
                                       GameState->SelectedEntityIndex);
              // s_ShowAnimationEditor = true;
            }
#endif
          // TODO(Lukas) Remove this flaming garbage!!!
          if(GameState->ShowDebugFeatures)
          {
            if(GetEntityMMDataIndex(GameState->SelectedEntityIndex, &GameState->MMEntityData) == -1)
            {
              static bool    Mirror                 = false;
              static bool    Loop                   = false;
              static int32_t SelectedAnimationIndex = -1;
              ImGui::Combo("Animation", &SelectedAnimationIndex, PathArrayToString,
                           GameState->Resources.AnimationPaths,
                           GameState->Resources.AnimationPathCount);
              bool ClickedAddAnimation = ImGui::Button("Start");
              ImGui::SameLine();
              ImGui::Checkbox("Mirror", &Mirror);
              ImGui::SameLine();
              ImGui::Checkbox("Loop", &Loop);
              ImGui::Checkbox("Preview In Root Space", &GameState->PreviewAnimationsInRootSpace);
              bool ClickedStop = (SelectedEntity->AnimPlayer->AnimationIDs[0].Value > 0)
                                   ? ImGui::Button("Stop")
                                   : false;
              if(SelectedAnimationIndex >= 0 && ClickedAddAnimation)
              {
                rid NewRID = GameState->Resources.ObtainAnimationPathRID(
                  GameState->Resources.AnimationPaths[SelectedAnimationIndex].Name);
                if(GameState->Resources.GetAnimation(NewRID)->ChannelCount ==
                   SelectedEntity->AnimPlayer->Skeleton->BoneCount)
                {
                  if(SelectedEntity->AnimPlayer->AnimationIDs[0].Value > 0)
                  {
                    GameState->Resources.Animations.RemoveReference(
                      SelectedEntity->AnimPlayer->AnimationIDs[0]);
                  }
                  Anim::SetAnimation(SelectedEntity->AnimPlayer, NewRID, 0);
                  SelectedEntity->AnimPlayer->AnimStateCount = 1;
                  Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimPlayer, 0, Loop, 0);
                  SelectedEntity->AnimPlayer->States[0].Mirror = Mirror;
                  SelectedEntity->AnimPlayer->BlendFunc        = Anim::PreviewBlendFunc;
                  GameState->Resources.Animations.AddReference(NewRID);
                }
              }
              else if(ClickedStop && SelectedEntity->AnimPlayer->AnimationIDs[0].Value > 0)
              {
                RemoveReferencesAndResetAnimPlayer(&GameState->Resources,
                                                   SelectedEntity->AnimPlayer);
              }
            }
          }

          {
            // Outside data used
            mm_entity_data&             MMEntityData        = GameState->MMEntityData;
            int                         SelectedEntityIndex = GameState->SelectedEntityIndex;
            const spline_system&        SplineSystem        = GameState->SplineSystem;
            Resource::resource_manager* Resources           = &GameState->Resources;
            const Anim::skeleton*       Skeleton            = SelectedEntity->AnimPlayer->Skeleton;
            entity*                     Entities            = GameState->Entities;

            bool RemovedMatchingAnimPlayer = false;
            // Actual UI
            int32_t MMEntityIndex             = -1;
            bool    ShowMMControllerComponent = false;
            if((MMEntityIndex = GetEntityMMDataIndex(SelectedEntityIndex, &MMEntityData)) == -1)
            {
              if(MMEntityData.Count < MM_CONTROLLER_MAX_COUNT &&
                 ImGui::Button("Add Matched Animation Controller"))
              {
                RemoveReferencesAndResetAnimPlayer(Resources, SelectedEntity->AnimPlayer);

                MMEntityData.Count++;
                mm_aos_entity_data MMControllerData =
                  GetAOSMMDataAtIndex(MMEntityData.Count - 1, &MMEntityData);

                SetDefaultMMControllerFileds(&MMControllerData);
                *MMControllerData.EntityIndex = SelectedEntityIndex;
              }
            }
            else
            {
              ShowMMControllerComponent = ImGui::TreeNode("Matched Anim. Controller Component");
              ImGui::SameLine();
              ImGui::PushID("Remove Matching Anim. Controller");
              RemovedMatchingAnimPlayer = ImGui::Button("Remove");
              ImGui::PopID();

              if(ShowMMControllerComponent)
              {
                mm_aos_entity_data MMEntity = GetAOSMMDataAtIndex(MMEntityIndex, &MMEntityData);

                {
                  // Pick the mm controller
                  static int32_t ShownPathIndex = -1;
                  int32_t        UsedPathIndex  = -1;
                  if(MMEntity.MMControllerRID->Value > 0)
                  {
                    UsedPathIndex = Resources->GetMMControllerPathIndex(*MMEntity.MMControllerRID);
                  }
                  bool ClickedAdd = false;
                  {
                    if(ShownPathIndex != UsedPathIndex)
                    {
                      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.8f, 0.4f, 1 });
                      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 1, 1, 0.6f, 1 });
                    }
                    ImGui::PushID(&ShowMMControllerComponent);
                    ClickedAdd = ImGui::Button("Set");
                    ImGui::PopID();
                    if(ShownPathIndex != UsedPathIndex)
                    {
                      ImGui::PopStyleColor();
                      ImGui::PopStyleColor();
                    }
                  }
                  ImGui::SameLine();
                  ImGui::Combo("Controller", &ShownPathIndex, PathArrayToString,
                               &Resources->MMControllerPaths, Resources->MMControllerPathCount);
                  if(ClickedAdd)
                  {
                    if(ShownPathIndex != -1)
                    {
                      rid NewRID = Resources->ObtainMMControllerPathRID(
                        Resources->MMControllerPaths[ShownPathIndex].Name);
                      mm_controller_data* MMController = Resources->GetMMController(NewRID);
                      if(MMController->Params.FixedParams.Skeleton.BoneCount == Skeleton->BoneCount)
                      {
                        if(MMEntity.MMControllerRID->Value > 0 &&
                           MMEntity.MMControllerRID->Value != NewRID.Value)
                        {
                          Resources->MMControllers.RemoveReference(*MMEntity.MMControllerRID);
                        }
                        *MMEntity.MMControllerRID = NewRID;
                        Resources->MMControllers.AddReference(NewRID);
                      }
                    }
                    else if(MMEntity.MMControllerRID->Value > 0)
                    {
                      Resources->MMControllers.RemoveReference(*MMEntity.MMControllerRID);
                      *MMEntity.MMControllerRID = {};
                      *MMEntity.MMController    = NULL;
                    }
                    MMEntity.BlendStack->Clear();
                  }
                }

                char TempBuffer[40];

                if(MMEntity.MMControllerRID->Value > 0)
                {
                  path    MMControllerPath;
                  int32_t ControllerPathIndex =
                    Resources->GetMMControllerPathIndex(*MMEntity.MMControllerRID);
                  MMControllerPath = Resources->MMControllerPaths[ControllerPathIndex];
                  char TempBuffer[40];
                  sprintf(TempBuffer, "Attached: %s", strrchr(MMControllerPath.Name, '/') + 1);
                  ImGui::Text(TempBuffer);
                }

                sprintf(TempBuffer, "MM Entity Index: %d", MMEntityIndex);
                ImGui::Text(TempBuffer);

                // static bool s_ShowInputControlParameters = false;
                if(ImGui::TreeNode("Movement Control Options"))
                {
                  ImGui::SliderFloat("Maximum Speed", &MMEntity.InputController->MaxSpeed, 0.0f,
                                     5.0f);
                  ImGui::Checkbox("Strafe", &MMEntity.InputController->UseStrafing);
                  ImGui::Checkbox("Use Smoothed Goal", &MMEntity.InputController->UseSmoothGoal);
                  // static bool s_ShowSmoothTrajectoryParams = false;
                  if(MMEntity.InputController->UseSmoothGoal &&
                     ImGui::TreeNode("Smooth Goal Params"))
                  {
                    ImGui::SliderFloat("Position Bias", &MMEntity.InputController->PositionBias,
                                       0.0f, 5.0f);
                    ImGui::SliderFloat("Direction Bias", &MMEntity.InputController->DirectionBias,
                                       0.0f, 5.0f);
                    ImGui::TreePop();
                  }
                  ImGui::Checkbox("Use Trajectory Control", MMEntity.FollowSpline);

                  if(*MMEntity.FollowSpline == true)
                  {
                    /*static bool s_ShowTrajectoryControlParameters = true;
                    if(ImGui::TreeNode("Trajectory Control Params",
                                    &s_ShowTrajectoryControlParameters))
                    {*/
                    ImGui::Combo("Trajectory Index", &MMEntity.SplineState->SplineIndex,
                                 (const char**)&g_SplineIndexNames[0], SplineSystem.Splines.Count);
                    /*ImGui::Checkbox("Loop Back To Start", &MMEntity.SplineState->Loop);
                    ImGui::Checkbox("Following Positive Direction",
                                 &MMEntity.SplineState->MovingInPositive);

                    ImGui::TreePop();
                  }*/
                  }
                  ImGui::TreePop();
                }
                ImGui::TreePop();
              }
              if(RemovedMatchingAnimPlayer)
              {
                RemoveMMControllerDataAtIndex(Entities, MMEntityIndex, Resources, &MMEntityData);
              }
            }
          }
          ImGui::TreePop();
        }
        if(RemovedAnimPlayer)
        {
          RemoveAnimationPlayerComponent(GameState, &GameState->Resources,
                                         GameState->SelectedEntityIndex);
        }
      }
    }
  }
#endif
  ImGui::End();
}

void
MaterialEditor(game_state* GameState)
{
  ImGui::Begin("Material Editor");
  if(GameState->SelectionMode == SELECT_Mesh || GameState->SelectionMode == SELECT_Entity)
  {
    {
      int32_t ActivePathIndex = 0;
      if(GameState->CurrentMaterialID.Value > 0)
      {
        ActivePathIndex = GameState->Resources.GetMaterialPathIndex(GameState->CurrentMaterialID);
      }
      ImGui::Combo("Material", &ActivePathIndex, PathArrayToString,
                   GameState->Resources.MaterialPaths, GameState->Resources.MaterialPathCount);
      if(GameState->Resources.MaterialPathCount > 0)
      {
        rid NewRID = { 0 };
        if(GameState->Resources
             .GetMaterialPathRID(&NewRID, GameState->Resources.MaterialPaths[ActivePathIndex].Name))
        {
          GameState->CurrentMaterialID = NewRID;
        }
        else
        {
          GameState->CurrentMaterialID = GameState->Resources.RegisterMaterial(
            GameState->Resources.MaterialPaths[ActivePathIndex].Name);
        }
      }
    }
    if(0 < GameState->CurrentMaterialID.Value)
    {
      // Draw material preview to texture
      float ImageWidth  = MaxFloat(300, MinFloat(700, ImGui::GetContentRegionAvail().x));
      float ImageHeight = ImageWidth * (float(SCREEN_HEIGHT) / float(SCREEN_WIDTH));
      ImGui::Image((void*)(uint64_t)GameState->IDTexture, ImVec2(ImageWidth, ImageHeight));

      material* CurrentMaterial = GameState->Resources.GetMaterial(GameState->CurrentMaterialID);

      // Select shader type
      {
        int32_t NewShaderType = CurrentMaterial->Common.ShaderType;
        ImGui::PushItemWidth(200);
        ImGui::Combo("Shader Type", (int32_t*)&NewShaderType, g_ShaderTypeEnumStrings,
                     SHADER_EnumCount, 6);
        ImGui::PopItemWidth();
        if(CurrentMaterial->Common.ShaderType != NewShaderType)
        {
          *CurrentMaterial                   = {};
          CurrentMaterial->Common.ShaderType = NewShaderType;
        }
      }

      ImGui::Checkbox("Blending", &CurrentMaterial->Common.UseBlending);
      ImGui::SameLine();
      ImGui::Checkbox("Skeletel", &CurrentMaterial->Common.IsSkeletal);

      if(CurrentMaterial->Common.ShaderType == SHADER_Phong)
      {
        if(CurrentMaterial->Common.IsSkeletal)
        {
          CurrentMaterial->Phong.Flags |= PHONG_UseSkeleton;
        }
        else
        {
          CurrentMaterial->Phong.Flags &= ~PHONG_UseSkeleton;
        }
      }

      switch(CurrentMaterial->Common.ShaderType)
      {
        case SHADER_Phong:
        {
          bool UseDIffuse      = (CurrentMaterial->Phong.Flags & PHONG_UseDiffuseMap);
          bool UseSpecular     = CurrentMaterial->Phong.Flags & PHONG_UseSpecularMap;
          bool NormalFlagValue = CurrentMaterial->Phong.Flags & PHONG_UseNormalMap;
          ImGui::Checkbox("Diffuse Map", &UseDIffuse);
          ImGui::Checkbox("Specular Map", &UseSpecular);
          ImGui::Checkbox("Normal Map", &NormalFlagValue);

          ImGui::ColorEdit3("Ambient Color", (float*)&CurrentMaterial->Phong.AmbientColor);

          if(UseDIffuse)
          {
            {
              int32_t ActivePathIndex = 0;
              if(CurrentMaterial->Phong.DiffuseMapID.Value > 0)
              {
                ActivePathIndex =
                  GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.DiffuseMapID);
              }
              if(GameState->Resources.TexturePathCount > 0)
              {
                CurrentMaterial->Phong.Flags |= PHONG_UseDiffuseMap;

                ImGui::Combo("Diffuse Map", &ActivePathIndex, PathArrayToString,
                             GameState->Resources.TexturePaths,
                             GameState->Resources.TexturePathCount);
                rid NewRID;
                if(GameState->Resources
                     .GetTexturePathRID(&NewRID,
                                        GameState->Resources.TexturePaths[ActivePathIndex].Name))
                {
                  CurrentMaterial->Phong.DiffuseMapID = NewRID;
                }
                else
                {
                  CurrentMaterial->Phong.DiffuseMapID = GameState->Resources.RegisterTexture(
                    GameState->Resources.TexturePaths[ActivePathIndex].Name);
                }
                assert(CurrentMaterial->Phong.DiffuseMapID.Value > 0);
              }
            }
          }
          else
          {
            CurrentMaterial->Phong.Flags &= ~PHONG_UseDiffuseMap;
            ImGui::ColorEdit4("Diffuse Color", (float*)&CurrentMaterial->Phong.DiffuseColor.X);
          }

          if(UseSpecular)
          {
            {
              int32_t ActivePathIndex = 0;
              if(CurrentMaterial->Phong.SpecularMapID.Value > 0)
              {
                ActivePathIndex =
                  GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.SpecularMapID);
              }
              ImGui::Combo("Specular Map", &ActivePathIndex, PathArrayToString,
                           GameState->Resources.TexturePaths,
                           GameState->Resources.TexturePathCount);
              if(GameState->Resources.TexturePathCount > 0)
              {
                CurrentMaterial->Phong.Flags |= PHONG_UseSpecularMap;

                rid NewRID;
                if(GameState->Resources
                     .GetTexturePathRID(&NewRID,
                                        GameState->Resources.TexturePaths[ActivePathIndex].Name))
                {
                  CurrentMaterial->Phong.SpecularMapID = NewRID;
                }
                else
                {
                  CurrentMaterial->Phong.SpecularMapID = GameState->Resources.RegisterTexture(
                    GameState->Resources.TexturePaths[ActivePathIndex].Name);
                }
                assert(CurrentMaterial->Phong.SpecularMapID.Value > 0);
              }
            }
          }
          else
          {
            CurrentMaterial->Phong.Flags &= ~PHONG_UseSpecularMap;
            ImGui::ColorEdit3("Specular Color", (float*)&CurrentMaterial->Phong.SpecularColor.X);
          }

          if(NormalFlagValue)
          {
            {
              int32_t ActivePathIndex = 0;
              if(CurrentMaterial->Phong.NormalMapID.Value > 0)
              {
                ActivePathIndex =
                  GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.NormalMapID);
              }
              ImGui::Combo("Normal map", &ActivePathIndex, PathArrayToString,
                           GameState->Resources.TexturePaths,
                           GameState->Resources.TexturePathCount);
              if(GameState->Resources.TexturePathCount > 0)
              {
                CurrentMaterial->Phong.Flags |= PHONG_UseNormalMap;

                rid NewRID;
                if(GameState->Resources
                     .GetTexturePathRID(&NewRID,
                                        GameState->Resources.TexturePaths[ActivePathIndex].Name))
                {
                  CurrentMaterial->Phong.NormalMapID = NewRID;
                }
                else
                {
                  CurrentMaterial->Phong.NormalMapID = GameState->Resources.RegisterTexture(
                    GameState->Resources.TexturePaths[ActivePathIndex].Name);
                }
                assert(CurrentMaterial->Phong.NormalMapID.Value > 0);
              }
            }
          }
          else
          {
            CurrentMaterial->Phong.Flags &= ~PHONG_UseNormalMap;
          }

          ImGui::SliderFloat("Shininess", &CurrentMaterial->Phong.Shininess, 1.0f, 512.0f);
        }
        break;
        case SHADER_Env:
        {
          bool Refraction = CurrentMaterial->Env.Flags & ENV_UseRefraction;
          bool NormalMap  = CurrentMaterial->Env.Flags & ENV_UseNormalMap;
          ImGui::Checkbox("Refraction", &Refraction);
          ImGui::Checkbox("Normal Map", &NormalMap);

          if(Refraction)
          {
            CurrentMaterial->Env.Flags |= ENV_UseRefraction;
          }
          else
          {
            CurrentMaterial->Env.Flags &= ~ENV_UseRefraction;
          }

          if(NormalMap)
          {
            {
              int32_t ActivePathIndex = 0;
              if(CurrentMaterial->Env.NormalMapID.Value > 0)
              {
                ActivePathIndex =
                  GameState->Resources.GetTexturePathIndex(CurrentMaterial->Env.NormalMapID);
              }
              ImGui::Combo("Normal map", &ActivePathIndex, PathArrayToString,
                           GameState->Resources.TexturePaths,
                           GameState->Resources.TexturePathCount);
              if(GameState->Resources.TexturePathCount > 0)
              {
                CurrentMaterial->Env.Flags |= ENV_UseNormalMap;

                rid NewRID;
                if(GameState->Resources
                     .GetTexturePathRID(&NewRID,
                                        GameState->Resources.TexturePaths[ActivePathIndex].Name))
                {
                  CurrentMaterial->Env.NormalMapID = NewRID;
                }
                else
                {
                  CurrentMaterial->Env.NormalMapID = GameState->Resources.RegisterTexture(
                    GameState->Resources.TexturePaths[ActivePathIndex].Name);
                }
                assert(CurrentMaterial->Env.NormalMapID.Value > 0);
              }
            }
          }
          else
          {
            CurrentMaterial->Env.Flags &= ~ENV_UseNormalMap;
          }

          ImGui::SliderFloat("Refractive Index", &CurrentMaterial->Env.RefractiveIndex, 1.0f, 3.0f);
        }
        break;
        case SHADER_Toon:
        {
          ImGui::DragFloat3("Ambient Color", &CurrentMaterial->Toon.AmbientColor.X, 0.0f, 1.0f,
                            5.0f);
          ImGui::DragFloat4("Diffuse Color", &CurrentMaterial->Toon.DiffuseColor.X, 0.0f, 1.0f,
                            5.0f);
          ImGui::DragFloat3("Specular Color", &CurrentMaterial->Toon.SpecularColor.X, 0.0f, 1.0f,
                            5.0f);

          ImGui::SliderFloat("Shininess", &CurrentMaterial->Toon.Shininess, 1.0f, 512.0f);
          ImGui::SliderInt("LevelCount", &CurrentMaterial->Toon.LevelCount, 1, 10);
        }
        break;
        case SHADER_LightWave:
        {
          ImGui::DragFloat3("Ambient Color", &CurrentMaterial->LightWave.AmbientColor.X, 0.0f, 1.0f,
                            5.0f);
          ImGui::DragFloat4("Diffuse Color", &CurrentMaterial->LightWave.DiffuseColor.X, 0.0f, 1.0f,
                            5.0f);
          ImGui::DragFloat3("Specular Color", &CurrentMaterial->LightWave.SpecularColor.X, 0.0f,
                            1.0f, 5.0f);

          ImGui::SliderFloat("Shininess", &CurrentMaterial->LightWave.Shininess, 1.0f, 512.0f);
        }
        break;
        case SHADER_Color:
        {
          ImGui::DragFloat4("Color", &CurrentMaterial->Color.Color.X, 0, 1, 5);
        }
        break;
        default:
        {
          struct shader_def* ShaderDef = NULL;
          assert(GetShaderDef(&ShaderDef, CurrentMaterial->Common.ShaderType));
          {
            named_shader_param_def ShaderParamDef = {};
            ResetShaderDefIterator(ShaderDef);
            while(GetNextShaderParam(&ShaderParamDef, ShaderDef))
            {
              void* ParamLocation =
                (void*)(((uint8_t*)CurrentMaterial) + ShaderParamDef.OffsetIntoMaterial);
              switch(ShaderParamDef.Type)
              {
                case SHADER_PARAM_TYPE_Int:
                {
                  int32_t* ParamPtr = (int32_t*)ParamLocation;
                  ImGui::SliderInt(ShaderParamDef.Name, ParamPtr, 0, 10);
                }
                break;
                case SHADER_PARAM_TYPE_Bool:
                {
                  bool* ParamPtr = (bool*)ParamLocation;
                  ImGui::Checkbox(ShaderParamDef.Name, ParamPtr);
                }
                break;
                case SHADER_PARAM_TYPE_Float:
                {
                  float* ParamPtr = (float*)ParamLocation;
                  ImGui::DragFloat(ShaderParamDef.Name, ParamPtr, -100, 100, 2.0f);
                }
                break;
                case SHADER_PARAM_TYPE_Vec3:
                {
                  vec3* ParamPtr = (vec3*)ParamLocation;
                  ImGui::ColorEdit3(ShaderParamDef.Name, (float*)ParamPtr);
                }
                break;
                case SHADER_PARAM_TYPE_Vec4:
                {
                  vec4* ParamPtr = (vec4*)ParamLocation;
                  ImGui::ColorEdit4(ShaderParamDef.Name, (float*)ParamPtr);
                }
                break;
                case SHADER_PARAM_TYPE_Map:
                {
                  int32_t ActivePathIndex        = 0;
                  rid*    CurrentParamTextureRID = (rid*)ParamLocation;
                  if(0 < CurrentParamTextureRID->Value)
                  {
                    ActivePathIndex =
                      GameState->Resources.GetTexturePathIndex(*CurrentParamTextureRID);
                  }
                  if(0 < GameState->Resources.TexturePathCount)
                  {
                    ImGui::Combo(ShaderParamDef.Name, &ActivePathIndex, PathArrayToString,
                                 GameState->Resources.TexturePaths,
                                 GameState->Resources.TexturePathCount);
                    rid NewRID;
                    if(GameState->Resources.GetTexturePathRID(&NewRID,
                                                              GameState->Resources
                                                                .TexturePaths[ActivePathIndex]
                                                                .Name))
                    {
                      *CurrentParamTextureRID = NewRID;
                    }
                    else
                    {
                      *CurrentParamTextureRID = GameState->Resources.RegisterTexture(
                        GameState->Resources.TexturePaths[ActivePathIndex].Name);
                    }
                    assert(0 < CurrentParamTextureRID->Value);
                  }
                }
                break;
              }
            }
          }
        }
      }
      if(GameState->Resources.MaterialPathCount > 0 && GameState->CurrentMaterialID.Value > 0)
      {
        int CurrentMaterialPathIndex =
          GameState->Resources.GetMaterialPathIndex(GameState->CurrentMaterialID);
        if(CurrentMaterialPathIndex != -1)
        {
          char* CurrentMaterialPath =
            GameState->Resources.MaterialPaths[CurrentMaterialPathIndex].Name;
          if(ImGui::Button("Save"))
          {
            ExportMaterial(&GameState->Resources, CurrentMaterial, CurrentMaterialPath);
          }
        }
      }
      ImGui::SameLine();
      if(ImGui::Button("Duplicate Current"))
      {
        GameState->CurrentMaterialID = GameState->Resources.CreateMaterial(*CurrentMaterial, NULL);
        printf("Created Material with rid: %d\n", GameState->CurrentMaterialID.Value);
      }

      ImGui::SameLine();
      entity* SelectedEntity = {};
      if(ImGui::Button("Create New"))
      {
        GameState->CurrentMaterialID =
          GameState->Resources.CreateMaterial(NewPhongMaterial(), NULL);
        printf("Created Material with rid: %d\n", GameState->CurrentMaterialID.Value);
      }
      if(GetSelectedEntity(GameState, &SelectedEntity))
      {
        ImGui::SameLine();
        if(ImGui::Button("Apply To Selected"))
        {
          if(GameState->CurrentMaterialID.Value > 0)
          {
            if(GameState->SelectionMode == SELECT_Mesh)
            {
              SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex] =
                GameState->CurrentMaterialID;
            }
            else if(GameState->SelectionMode == SELECT_Entity)
            {
              Render::model* Model = GameState->Resources.GetModel(SelectedEntity->ModelID);
              for(int m = 0; m < Model->MeshCount; m++)
              {
                SelectedEntity->MaterialIDs[m] = GameState->CurrentMaterialID;
              }
            }
          }
        }
        if(GameState->SelectionMode == SELECT_Mesh)
        {
          ImGui::SameLine();
          if(ImGui::Button("Edit Selected"))
          {
            GameState->CurrentMaterialID =
              SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex];
          }
        }
      }
    }
  }
  ImGui::End();
}

void
SettingsEditor(game_state* GameState)
{
  ImGui::Begin("Settings Window");
  if(ImGui::TreeNode("Camera"))
  {
    ImGui::SliderFloat("FieldOfView", &GameState->Camera.FieldOfView, 0, 180);
    ImGui::SliderFloat("Near CLip Plane", &GameState->Camera.NearClipPlane, 0.01f, 500);
    ImGui::SliderFloat("Far  Clip Plane", &GameState->Camera.FarClipPlane,
                       GameState->Camera.NearClipPlane, 500);
    ImGui::Checkbox("Orbit Selected", &GameState->Camera.OrbitSelected);
    if(GameState->Camera.OrbitSelected)
    {
      ImGui::SliderFloat("Orbit Radius", &GameState->Camera.OrbitRadius, 0, 30);
    }
    else
    {
      ImGui::SliderFloat("Speed", &GameState->Camera.Speed, 0, 100);
    }
    ImGui::TreePop();
  }
  if(ImGui::TreeNode("Lighting"))
  {
    ImGui::DragFloat3("Diffuse", &GameState->R.LightDiffuseColor.X, 0, 10, 5);
    ImGui::DragFloat3("Ambient", &GameState->R.LightAmbientColor.X, 0, 10, 5);
    ImGui::DragFloat3("Position", &GameState->R.LightPosition.X, -INFINITY, INFINITY, 5);
    ImGui::Checkbox("Show gizmo", &GameState->R.ShowLightPosition);

    ImGui::DragFloat3("Diffuse", &GameState->R.Sun.DiffuseColor.X, 0, 1, 5);
    ImGui::DragFloat3("Ambient", &GameState->R.Sun.AmbientColor.X, 0, 1, 5);

    ImGui::SliderFloat("Sun Z Angle", &GameState->R.Sun.RotationZ, 0.0f, 90.0f);
    ImGui::SliderFloat("Sun Y Angle", &GameState->R.Sun.RotationY, -180, 180);

    if(1 < SHADOWMAP_CASCADE_COUNT)
    {
      ImGui::SliderInt("Current Cascade Index", &GameState->R.Sun.CurrentCascadeIndex, 0,
                       SHADOWMAP_CASCADE_COUNT - 1);
    }
    ImGui::Checkbox("Display sun-perspective depth map", &GameState->R.DrawShadowMap);
    ImGui::Checkbox("Real-time shadows", &GameState->R.RealTimeDirectionalShadows);
    if(ImGui::Button("Recompute Directional Shadows"))
    {
      GameState->R.RecomputeDirectionalShadows = true;
    }

    if(ImGui::Button("Clear Directional Shadows"))
    {
      GameState->R.ClearDirectionalShadows = true;
    }
    ImGui::TreePop();
  }

  if(ImGui::TreeNode("Post-processing"))
  {
    bool HDRTonemap   = GameState->R.PPEffects & POST_HDRTonemap;
    bool Bloom        = GameState->R.PPEffects & POST_Bloom;
    bool FXAA         = GameState->R.PPEffects & POST_FXAA;
    bool Blur         = GameState->R.PPEffects & POST_Blur;
    bool DepthOfField = GameState->R.PPEffects & POST_DepthOfField;
    bool Grayscale    = GameState->R.PPEffects & POST_Grayscale;
    bool NightVision  = GameState->R.PPEffects & POST_NightVision;
    bool MotionBlur   = GameState->R.PPEffects & POST_MotionBlur;
    bool EdgeOutline  = GameState->R.PPEffects & POST_EdgeOutline;
    bool SimpleFog    = GameState->R.PPEffects & POST_SimpleFog;
    bool Noise        = GameState->R.PPEffects & POST_Noise;
    bool Test         = GameState->R.PPEffects & POST_Test;

    ImGui::Checkbox("HDRTonemap", &HDRTonemap);
    ImGui::Checkbox("Bloom", &Bloom);
    ImGui::Checkbox("FXAA", &FXAA);
    ImGui::Checkbox("Blur", &Blur);
    ImGui::Checkbox("DepthOfField", &DepthOfField);
    ImGui::Checkbox("MotionBlur", &MotionBlur);
    ImGui::Checkbox("Grayscale", &Grayscale);
    ImGui::Checkbox("NightVision", &NightVision);
    ImGui::Checkbox("EdgeOutline", &EdgeOutline);
    ImGui::Checkbox("DepthBuffer", &GameState->R.DrawDepthBuffer);
    ImGui::Checkbox("SSAO", &GameState->R.RenderSSAO);
    ImGui::Checkbox("SimpleFog", &SimpleFog);
    ImGui::Checkbox("VolumetricScattering", &GameState->R.RenderVolumetricScattering);
    ImGui::Checkbox("Noise", &Noise);
    ImGui::Checkbox("Test", &Test);

    if(HDRTonemap)
    {
      GameState->R.PPEffects |= POST_HDRTonemap;
      ImGui::SliderFloat("HDR Exposure", &GameState->R.ExposureHDR, 0.01f, 8.0f);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_HDRTonemap;
    }

    if(Bloom)
    {
      GameState->R.PPEffects |= POST_Bloom;
      ImGui::SliderFloat("Bloom Threshold", &GameState->R.BloomLuminanceThreshold, 0.01f, 5.0f);
      ImGui::SliderInt("Bloom Blur Iterations", &GameState->R.BloomBlurIterationCount, 0, 5);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Bloom;
    }

    if(FXAA)
    {
      GameState->R.PPEffects |= POST_FXAA;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_FXAA;
    }

    if(Blur)
    {
      GameState->R.PPEffects |= POST_Blur;
      ImGui::SliderFloat("StdDev", &GameState->R.PostBlurStdDev, 0.01f, 10.0f);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Blur;
    }

    if(DepthOfField)
    {
      GameState->R.PPEffects |= POST_DepthOfField;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_DepthOfField;
    }

    if(MotionBlur)
    {
      GameState->R.PPEffects |= POST_MotionBlur;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_MotionBlur;
    }

    if(Grayscale)
    {
      GameState->R.PPEffects |= POST_Grayscale;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Grayscale;
    }

    if(NightVision)
    {
      GameState->R.PPEffects |= POST_NightVision;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_NightVision;
    }

    if(EdgeOutline)
    {
      GameState->R.PPEffects |= POST_EdgeOutline;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_EdgeOutline;
    }

    if(GameState->R.RenderSSAO)
    {
      ImGui::SliderFloat("SSAO Sample Radius", &GameState->R.SSAOSamplingRadius, 0.02f, 0.2f);
#if 0
    	ImGui::Image("Material preview", GameState->R.SSAOTexID, { 700, (int)(700.0 * (3.0f / 5.0f)) });
#endif
    }

    // ImGui::Image("ScenePreview", GameState->R.LightScatterTextures[0], { 500, 300 });

    if(SimpleFog)
    {
      GameState->R.PPEffects |= POST_SimpleFog;
      ImGui::SliderFloat("FogDensity", &GameState->R.FogDensity, 0.01f, 0.5f);
      ImGui::SliderFloat("FogGradient", &GameState->R.FogGradient, 1.0f, 10.0f);
      ImGui::SliderFloat("FogColor", &GameState->R.FogColor, 0.0f, 1.0f);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_SimpleFog;
    }

    if(Noise)
    {
      GameState->R.PPEffects |= POST_Noise;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Noise;
    }

    if(Test)
    {
      GameState->R.PPEffects |= POST_Test;
    }
    else
    {
      GameState->R.PPEffects &= ~POST_Test;
    }
    ImGui::TreePop();
  }

  if(ImGui::TreeNode("What To Draw"))
  {
    ImGui::Checkbox("Cubemap", &GameState->DrawCubemap);
    ImGui::Checkbox("Draw Gizmos", &GameState->DrawGizmos);
    ImGui::Checkbox("Draw Debug Lines", &GameState->DrawDebugLines);
    ImGui::Checkbox("Draw Debug Spheres", &GameState->DrawDebugSpheres);
    ImGui::Checkbox("Draw Actor Meshes", &GameState->DrawActorMeshes);
    ImGui::Checkbox("Draw Actor Skeletons", &GameState->DrawActorSkeletons);
    ImGui::Checkbox("Draw Shadowmap Cascade Volumes", &GameState->DrawShadowCascadeVolumes);
    ImGui::Checkbox("Draw Trajectory Waypoints", &GameState->DrawTrajectoryWaypoints);
    ImGui::Checkbox("Draw Trajectory Lines", &GameState->DrawTrajectoryLines);
    ImGui::Checkbox("Draw Trajectory Splines", &GameState->DrawTrajectorySplines);
    ImGui::Checkbox("Draw Splines Looped", &GameState->DrawSplinesLooped);
    ImGui::Checkbox("Show Debug Features", &GameState->ShowDebugFeatures);
    ImGui::TreePop();
  }
  ImGui::End();
}
