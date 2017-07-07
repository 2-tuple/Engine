#include "ui.h"
#include "scene.h"

void MaterialGUI(game_state* GameState, bool& g_ShowMaterialEditor);
void EntityGUI(game_state* GameState, bool& g_ShowEntityTools);
void AnimationGUI(game_state* GameState, bool& g_ShowAnimationEditor, bool& g_ShowEntityTools);
void MiscGUI(game_state* GameState, bool& g_ShowLightSettings, bool& g_ShowDisplaySet, bool& g_DrawMemoryMaps, bool& g_ShowCameraSettings, bool& g_ShowSceneSettings);

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    UI::BeginFrame(GameState, Input);

    UI::BeginWindow("Editor Window", { 600, 300 }, { 700, 600 });
    {
      UI::Combo("Selection mode", (int32_t*)&GameState->SelectionMode, g_SelectionEnumStrings, SELECT_EnumCount, UI::StringArrayToString);

      static bool g_ShowMaterialEditor  = false;
      static bool g_ShowEntityTools     = false;
      static bool g_ShowAnimationEditor = false;
      static bool g_ShowLightSettings   = false;
      static bool g_ShowDisplaySet      = false;
      static bool g_DrawMemoryMaps      = false;
      static bool g_ShowCameraSettings  = false;
      static bool g_ShowSceneSettings   = false;

      EntityGUI(GameState, g_ShowEntityTools);
      MaterialGUI(GameState, g_ShowMaterialEditor);
      AnimationGUI(GameState, g_ShowAnimationEditor, g_ShowEntityTools);
      MiscGUI(GameState, g_ShowLightSettings, g_ShowDisplaySet, g_DrawMemoryMaps, g_ShowCameraSettings, g_ShowSceneSettings);
    }
    UI::EndWindow();

    UI::BeginWindow("Collision window", { 1500, 800 }, { 400, 200 });
    {
      entity* Entity;
      if(GetSelectedEntity(GameState, &Entity))
      {
        if(UI::Button("Assign to A"))
        {
          GameState->EntityA   = GameState->SelectedEntityIndex;
          GameState->AssignedA = true;
        }

        if(UI::Button("Assign to B"))
        {
          GameState->EntityB   = GameState->SelectedEntityIndex;
          GameState->AssignedB = true;
        }
      }
    }
    UI::EndWindow();

    UI::BeginWindow("window A", { 50, 300 }, { 500, 380 });
    {
      static int         s_CurrentItem = -1;
      static const char* s_Items[]     = { "Cat", "Rat", "Hat", "Pat", "meet", "with", "dad" };

      static bool s_ShowDemo = false;
      if(UI::CollapsingHeader("Demo", &s_ShowDemo))
      {
        static bool s_Checkbox0 = false;
        static bool s_Checkbox1 = false;

        {
          gui_style& Style     = *UI::GetStyle();
          int32_t    Thickness = (int32_t)Style.Vars[UI::VAR_BorderThickness];
          UI::SliderInt("Border Thickness ", &Thickness, 0, 10);
          Style.Vars[UI::VAR_BorderThickness] = Thickness;

          UI::Text("Hold ctrl when dragging to snap to whole values");
          UI::DragFloat4("Window background", &Style.Colors[UI::COLOR_WindowBackground].X, 0, 1, 5);
          UI::SliderFloat("Horizontal Padding", &Style.Vars[UI::VAR_BoxPaddingX], 0, 10);
          UI::SliderFloat("Vertical Padding", &Style.Vars[UI::VAR_BoxPaddingY], 0, 10);
          UI::SliderFloat("Horizontal Spacing", &Style.Vars[UI::VAR_SpacingX], 0, 10);
          UI::SliderFloat("Vertical Spacing", &Style.Vars[UI::VAR_SpacingY], 0, 10);
          UI::SliderFloat("Internal Spacing", &Style.Vars[UI::VAR_InternalSpacing], 0, 10);
        }
        UI::Combo("Combo test", &s_CurrentItem, s_Items, ARRAY_SIZE(s_Items), 5, 150);
        int StartIndex = 3;
        UI::Combo("Combo test1", &s_CurrentItem, s_Items + StartIndex, ARRAY_SIZE(s_Items) - StartIndex);
        UI::NewLine();

        char TempBuff[30];
        snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
        UI::Text(TempBuff);

        snprintf(TempBuff, sizeof(TempBuff), "Mouse Screen: { %d, %d }", Input->MouseScreenX, Input->MouseScreenY);
        UI::Text(TempBuff);
        snprintf(TempBuff, sizeof(TempBuff), "Mouse Normal: { %.1f, %.1f }", Input->NormMouseX, Input->NormMouseY);
        UI::Text(TempBuff);

        UI::Checkbox("Show Image", &s_Checkbox0);
        if(s_Checkbox0)
        {
          UI::SameLine();
          UI::Checkbox("Put image in frame", &s_Checkbox1);
          UI::NewLine();
          if(s_Checkbox1)
          {
            UI::BeginChildWindow("Image frame", { 300, 170 });
          }

          UI::Image("material preview", GameState->IDTexture, { 400, 220 });

          if(s_Checkbox1)
          {
            UI::EndChildWindow();
          }
        }
      }
    }
    UI::EndWindow();

    UI::EndFrame();
  }
}

char*
PathArrayToString(void* Data, int Index)
{
  path* Paths = (path*)Data;
  return Paths[Index].Name;
}

void
MaterialGUI(game_state* GameState, bool& ShowMaterialEditor)
{
  if(GameState->SelectionMode == SELECT_Mesh || GameState->SelectionMode == SELECT_Entity)
  {
    if(UI::CollapsingHeader("Material Editor", &ShowMaterialEditor))
    {
      {
        int32_t ActivePathIndex = 0;
        if(GameState->CurrentMaterialID.Value > 0)
        {
          ActivePathIndex = GameState->Resources.GetMaterialPathIndex(GameState->CurrentMaterialID);
        }
        UI::Combo("Material", &ActivePathIndex, GameState->Resources.MaterialPaths, GameState->Resources.MaterialPathCount, PathArrayToString);
        if(GameState->Resources.MaterialPathCount > 0)
        {
          rid NewRID = { 0 };
          if(GameState->Resources.GetMaterialPathRID(&NewRID, GameState->Resources.MaterialPaths[ActivePathIndex].Name))
          {
            GameState->CurrentMaterialID = NewRID;
          }
          else
          {
            GameState->CurrentMaterialID = GameState->Resources.RegisterMaterial(GameState->Resources.MaterialPaths[ActivePathIndex].Name);
          }
        }
      }
      if(GameState->CurrentMaterialID.Value > 0)
      {
        // Draw material preview to texture
        UI::Image("Material preview", GameState->IDTexture, { 360, 200 });

        material* CurrentMaterial = GameState->Resources.GetMaterial(GameState->CurrentMaterialID);
        if(UI::Button("Previous shader"))
        {
          if(CurrentMaterial->Common.ShaderType > 0)
          {
            uint32_t ShaderType                 = CurrentMaterial->Common.ShaderType - 1;
            *CurrentMaterial                    = {};
            CurrentMaterial->Common.ShaderType  = ShaderType;
            CurrentMaterial->Common.UseBlending = true;
          }
        }
        UI::SameLine();
        if(UI::Button("Next shader"))
        {
          if(CurrentMaterial->Common.ShaderType < SHADER_EnumCount - 1)
          {
            uint32_t ShaderType                 = CurrentMaterial->Common.ShaderType + 1;
            *CurrentMaterial                    = {};
            CurrentMaterial->Common.ShaderType  = ShaderType;
            CurrentMaterial->Common.UseBlending = true;
          }
        }
        UI::SameLine();
        UI::NewLine();

        UI::Checkbox("Blending", &CurrentMaterial->Common.UseBlending);
        UI::SameLine();
        {
          bool SkeletalFlagValue = (CurrentMaterial->Phong.Flags & PHONG_UseSkeleton);

          UI::Checkbox("Skeletel", &SkeletalFlagValue);
          if(SkeletalFlagValue)
          {
            CurrentMaterial->Phong.Flags |= PHONG_UseSkeleton;
            CurrentMaterial->Common.IsSkeletal = true;
          }
          else
          {
            CurrentMaterial->Phong.Flags &= ~PHONG_UseSkeleton;
            CurrentMaterial->Common.IsSkeletal = false;
          }
        }
        UI::SameLine();
        UI::NewLine();

        switch(CurrentMaterial->Common.ShaderType)
        {
          case SHADER_Phong:
          {
            bool UseDIffuse      = (CurrentMaterial->Phong.Flags & PHONG_UseDiffuseMap);
            bool UseSpecular     = CurrentMaterial->Phong.Flags & PHONG_UseSpecularMap;
            bool NormalFlagValue = CurrentMaterial->Phong.Flags & PHONG_UseNormalMap;
            UI::Checkbox("Diffuse Map", &UseDIffuse);
            UI::Checkbox("Specular Map", &UseSpecular);
            UI::Checkbox("Normal Map", &NormalFlagValue);

            UI::DragFloat3("Ambient Color", &CurrentMaterial->Phong.AmbientColor.X, 0.0f, 1.0f, 5.0f);

            if(UseDIffuse)
            {
              {

                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Phong.DiffuseMapID.Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.DiffuseMapID);
                }
                if(GameState->Resources.TexturePathCount > 0)
                {
                  CurrentMaterial->Phong.Flags |= PHONG_UseDiffuseMap;

                  UI::Combo("Diffuse Map", &ActivePathIndex, GameState->Resources.TexturePaths, GameState->Resources.TexturePathCount, PathArrayToString);
                  rid NewRID;
                  if(GameState->Resources.GetTexturePathRID(&NewRID, GameState->Resources.TexturePaths[ActivePathIndex].Name))
                  {
                    CurrentMaterial->Phong.DiffuseMapID = NewRID;
                  }
                  else
                  {
                    CurrentMaterial->Phong.DiffuseMapID = GameState->Resources.RegisterTexture(GameState->Resources.TexturePaths[ActivePathIndex].Name);
                  }
                  assert(CurrentMaterial->Phong.DiffuseMapID.Value > 0);
                }
              }
            }
            else
            {
              CurrentMaterial->Phong.Flags &= ~PHONG_UseDiffuseMap;
              UI::DragFloat4("Diffuse Color", &CurrentMaterial->Phong.DiffuseColor.X, 0.0f, 1.0f, 5.0f);
            }

            if(UseSpecular)
            {
              {
                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Phong.SpecularMapID.Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.SpecularMapID);
                }
                UI::Combo("Specular Map", &ActivePathIndex, GameState->Resources.TexturePaths, GameState->Resources.TexturePathCount, PathArrayToString);
                if(GameState->Resources.TexturePathCount > 0)
                {
                  CurrentMaterial->Phong.Flags |= PHONG_UseSpecularMap;

                  rid NewRID;
                  if(GameState->Resources.GetTexturePathRID(&NewRID, GameState->Resources.TexturePaths[ActivePathIndex].Name))
                  {
                    CurrentMaterial->Phong.SpecularMapID = NewRID;
                  }
                  else
                  {
                    CurrentMaterial->Phong.SpecularMapID = GameState->Resources.RegisterTexture(GameState->Resources.TexturePaths[ActivePathIndex].Name);
                  }
                  assert(CurrentMaterial->Phong.SpecularMapID.Value > 0);
                }
              }
            }
            else
            {
              CurrentMaterial->Phong.Flags &= ~PHONG_UseSpecularMap;
              UI::DragFloat3("Specular Color", &CurrentMaterial->Phong.SpecularColor.X, 0.0f, 1.0f, 5.0f);
            }

            if(NormalFlagValue)
            {
              {
                int32_t ActivePathIndex = 0;
                if(CurrentMaterial->Phong.NormalMapID.Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetTexturePathIndex(CurrentMaterial->Phong.NormalMapID);
                }
                UI::Combo("Normal map", &ActivePathIndex, GameState->Resources.TexturePaths, GameState->Resources.TexturePathCount, PathArrayToString);
                if(GameState->Resources.TexturePathCount > 0)
                {
                  CurrentMaterial->Phong.Flags |= PHONG_UseNormalMap;

                  rid NewRID;
                  if(GameState->Resources.GetTexturePathRID(&NewRID, GameState->Resources.TexturePaths[ActivePathIndex].Name))
                  {
                    CurrentMaterial->Phong.NormalMapID = NewRID;
                  }
                  else
                  {
                    CurrentMaterial->Phong.NormalMapID = GameState->Resources.RegisterTexture(GameState->Resources.TexturePaths[ActivePathIndex].Name);
                  }
                  assert(CurrentMaterial->Phong.NormalMapID.Value > 0);
                }
              }
            }
            else
            {
              CurrentMaterial->Phong.Flags &= ~PHONG_UseNormalMap;
            }

            UI::SliderFloat("Shininess", &CurrentMaterial->Phong.Shininess, 1.0f, 512.0f);
          }
          break;
          case SHADER_Color:
          {
            UI::DragFloat4("Color", &CurrentMaterial->Color.Color.X, 0, 1, 5);
          }
          break;
        }
        if(GameState->Resources.MaterialPathCount > 0 && GameState->CurrentMaterialID.Value > 0)
        {
          int CurrentMaterialPathIndex = GameState->Resources.GetMaterialPathIndex(GameState->CurrentMaterialID);
          if(CurrentMaterialPathIndex != -1)
          {
            char* CurrentMaterialPath = GameState->Resources.MaterialPaths[CurrentMaterialPathIndex].Name;
            if(UI::Button("Save"))
            {
              ExportMaterial(&GameState->Resources, CurrentMaterial, CurrentMaterialPath);
            }
          }
        }
        UI::SameLine();
        if(UI::Button("Duplicate Current"))
        {
          GameState->CurrentMaterialID = GameState->Resources.CreateMaterial(*CurrentMaterial, NULL);
          printf("Created Material with rid: %d\n", GameState->CurrentMaterialID.Value);
        }
        UI::SameLine();
        if(UI::Button("Clear Material Fields"))
        {
          uint32_t ShaderType                = CurrentMaterial->Common.ShaderType;
          *CurrentMaterial                   = {};
          CurrentMaterial->Common.ShaderType = ShaderType;
        }
        UI::SameLine();
        UI::NewLine();
        entity* SelectedEntity = {};
        if(UI::Button("Create New"))
        {
          GameState->CurrentMaterialID = GameState->Resources.CreateMaterial(NewPhongMaterial(), NULL);
          printf("Created Material with rid: %d\n", GameState->CurrentMaterialID.Value);
        }
        UI::SameLine();
        if(GetSelectedEntity(GameState, &SelectedEntity))
        {
          if(UI::Button("Apply To Selected"))
          {
            if(GameState->CurrentMaterialID.Value > 0)
            {
              if(GameState->SelectionMode == SELECT_Mesh)
              {
                SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex] = GameState->CurrentMaterialID;
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
          UI::SameLine();
          if(GameState->SelectionMode == SELECT_Mesh)
          {
            if(UI::Button("Edit Selected"))
            {
              GameState->CurrentMaterialID = SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex];
            }
          }
        }
        UI::SameLine();
        UI::NewLine();
      }
    }
  }
}

void
EntityGUI(game_state* GameState, bool& s_ShowEntityTools)
{
  if(UI::CollapsingHeader("Entity Tools", &s_ShowEntityTools))
  {
    static int32_t ActivePathIndex = 0;
    UI::Combo("Model", &ActivePathIndex, GameState->Resources.ModelPaths, GameState->Resources.ModelPathCount, PathArrayToString);
    {
      rid NewRID = { 0 };
      if(!GameState->Resources.GetModelPathRID(&NewRID, GameState->Resources.ModelPaths[ActivePathIndex].Name))
      {
        NewRID                    = GameState->Resources.RegisterModel(GameState->Resources.ModelPaths[ActivePathIndex].Name);
        GameState->CurrentModelID = NewRID;
      }
      else
      {
        GameState->CurrentModelID = NewRID;
      }
    }

    if(UI::Button("Create Entity"))
    {
      GameState->IsEntityCreationMode = !GameState->IsEntityCreationMode;
    }

    entity* SelectedEntity = {};
    if(GetSelectedEntity(GameState, &SelectedEntity))
    {
      if(UI::Button("Delete Entity"))
      {
        DeleteEntity(GameState, GameState->SelectedEntityIndex);
        GameState->SelectedEntityIndex = -1;
      }

      Anim::transform* Transform = &SelectedEntity->Transform;
      UI::DragFloat3("Translation", (float*)&Transform->Translation, -INFINITY, INFINITY, 10.0f);
      UI::DragFloat3("Rotation", (float*)&Transform->Rotation, -INFINITY, INFINITY, 720.0f);
      UI::DragFloat3("Scale", (float*)&Transform->Scale, -INFINITY, INFINITY, 10.0f);

      Render::model* SelectedModel = GameState->Resources.GetModel(SelectedEntity->ModelID);
      if(SelectedModel->Skeleton)
      {
        if(!SelectedEntity->AnimController && UI::Button("Add Anim. Controller"))
        {
          SelectedEntity->AnimController  = PushStruct(GameState->PersistentMemStack, Anim::animation_controller);
          *SelectedEntity->AnimController = {};

          SelectedEntity->AnimController->Skeleton           = SelectedModel->Skeleton;
          SelectedEntity->AnimController->OutputTransforms   = PushArray(GameState->PersistentMemStack, ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT * SelectedModel->Skeleton->BoneCount, Anim::transform);
          SelectedEntity->AnimController->BoneSpaceMatrices  = PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          SelectedEntity->AnimController->ModelSpaceMatrices = PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          SelectedEntity->AnimController->HierarchicalModelSpaceMatrices = PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
        }
        else if(SelectedEntity->AnimController && UI::Button("Delete Anim. Controller"))
        {
          SelectedEntity->AnimController = 0;
        }
        else if(SelectedEntity->AnimController)
        {
          if(UI::Button("Animate Selected Entity"))
          {
            GameState->SelectionMode = SELECT_Bone;
            AttachEntityToAnimEditor(GameState, &GameState->AnimEditor, GameState->SelectedEntityIndex);
            // g_ShowAnimationEditor = true;
          }
          if(UI::Button("Play Animation"))
          {
            if(GameState->CurrentAnimationID.Value > 0)
            {
              if(GameState->Resources.GetAnimation(GameState->CurrentAnimationID)->ChannelCount == SelectedModel->Skeleton->BoneCount)
              {
                if(SelectedEntity->AnimController->AnimStateCount == 0)
                {
                  SelectedEntity->AnimController->AnimationIDs[0] = GameState->CurrentAnimationID;
                  SelectedEntity->AnimController->AnimStateCount  = 1;
                }
                else
                {
                  SelectedEntity->AnimController->AnimationIDs[0] = GameState->CurrentAnimationID;
                }
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 0);
              }
              else if(SelectedEntity->AnimController->AnimStateCount != 0)
              {
                StopAnimation(SelectedEntity->AnimController, 0);
              }
            }
          }
          {
            static int32_t ActivePathIndex = 0;
            UI::Combo("Animation", &ActivePathIndex, GameState->Resources.AnimationPaths, GameState->Resources.AnimationPathCount, PathArrayToString);
            rid NewRID = { 0 };
            if(GameState->Resources.AnimationPathCount > 0 && !GameState->Resources.GetAnimationPathRID(&NewRID, GameState->Resources.AnimationPaths[ActivePathIndex].Name))
            {
              NewRID = GameState->Resources.RegisterAnimation(GameState->Resources.AnimationPaths[ActivePathIndex].Name);
            }
            GameState->CurrentAnimationID = NewRID;
          }
#if 0
            UI::Row(&Layout);
            char CurrentAnimationIDString[30];
            sprintf(CurrentAnimationIDString, "Current Anim ID: %d",
                    GameState->CurrentAnimationID.Value);
            UI::DrawTextBox(GameState, &Layout, CurrentAnimationIDString);
#endif
          if(UI::Button("Play as entity"))
          {
            Gameplay::ResetPlayer();
            GameState->PlayerEntityIndex = GameState->SelectedEntityIndex;
            StartAnimationAtGlobalTime(SelectedEntity->AnimController, 0, true);
            StartAnimationAtGlobalTime(SelectedEntity->AnimController, 1, true);
            StartAnimationAtGlobalTime(SelectedEntity->AnimController, 2, true);
          }
          if(GameState->PlayerEntityIndex == GameState->SelectedEntityIndex)
          {
            UI::SliderFloat("Acceleration", &g_Acceleration, 0, 40);
            UI::SliderFloat("Deceleration", &g_Decceleration, 0, 40);
            UI::SliderFloat("Max Speed", &g_MaxSpeed, 0, 50);
            UI::SliderFloat("Playback Rate", &g_MovePlaybackRate, 0.1f, 10);

            { // Walk animation
              static int32_t ActivePathIndex = 0;
#if 0
                if(SelectedEntity->AnimController->AnimationIDs[0].Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetAnimationPathIndex(
                    SelectedEntity->AnimController->AnimationIDs[0]);
                }
#endif
              UI::Combo("Walk", &ActivePathIndex, GameState->Resources.AnimationPaths, GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 && !GameState->Resources.GetAnimationPathRID(&NewRID, GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(GameState->Resources.AnimationPaths[ActivePathIndex].Name);
              }
              if(GameState->Resources.AnimationPathCount && SelectedEntity->AnimController->AnimationIDs[0].Value != NewRID.Value)
              {
                Anim::SetAnimation(SelectedEntity->AnimController, NewRID, 0);
                printf("Setting walk\n");
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 0, true);
              }
            }
            { // Run animation
              static int32_t ActivePathIndex = 0;
#if 0
                if(SelectedEntity->AnimController->AnimationIDs[1].Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetAnimationPathIndex(
                    SelectedEntity->AnimController->AnimationIDs[1]);
                }
#endif
              UI::Combo("Run", &ActivePathIndex, GameState->Resources.AnimationPaths, GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 && !GameState->Resources.GetAnimationPathRID(&NewRID, GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(GameState->Resources.AnimationPaths[ActivePathIndex].Name);
              }
              if(SelectedEntity->AnimController->AnimationIDs[1].Value != NewRID.Value)
              {
                Anim::SetAnimation(SelectedEntity->AnimController, NewRID, 1);
                printf("Setting run\n");
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 1, true);
              }
            }
            { // Idle animation
              static int32_t ActivePathIndex = 0;
#if 0
                if(SelectedEntity->AnimController->AnimationIDs[2].Value > 0)
                {
                  ActivePathIndex = GameState->Resources.GetAnimationPathIndex(
                    SelectedEntity->AnimController->AnimationIDs[2]);
                }
#endif
              UI::Combo("Idle", &ActivePathIndex, GameState->Resources.AnimationPaths, GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 && !GameState->Resources.GetAnimationPathRID(&NewRID, GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(GameState->Resources.AnimationPaths[ActivePathIndex].Name);
              }
              if(SelectedEntity->AnimController->AnimationIDs[2].Value != NewRID.Value)
              {
                Anim::SetAnimation(SelectedEntity->AnimController, NewRID, 2);
                printf("Setting idle\n");
                Anim::StartAnimationAtGlobalTime(SelectedEntity->AnimController, 2, true);
              }
            }
          }
        }
      }
    }
  }
}

char*
BoneArrayToString(void* Data, int Index)
{
  Anim::bone* Bones = (Anim::bone*)Data;
  return Bones[Index].Name;
}

void
AnimationGUI(game_state* GameState, bool& g_ShowAnimationEditor, bool& g_ShowEntityTools)
{
  if(GameState->SelectionMode == SELECT_Bone && GameState->AnimEditor.Skeleton)
  {
    entity* AttachedEntity;
    if(GetEntityAtIndex(GameState, &AttachedEntity, GameState->AnimEditor.EntityIndex))
    {
      Render::model* AttachedModel = GameState->Resources.GetModel(AttachedEntity->ModelID);
      assert(AttachedModel->Skeleton == GameState->AnimEditor.Skeleton);
    }
    else
    {
      assert(0 && "no entity found in GameState->AnimEditor");
    }

    if(UI::CollapsingHeader("Animation Editor", &g_ShowAnimationEditor))
    {
      if(UI::Button("Stop Editing"))
      {
        DettachEntityFromAnimEditor(GameState, &GameState->AnimEditor);
        GameState->SelectionMode = SELECT_Entity;
        g_ShowEntityTools        = true;
        g_ShowAnimationEditor    = false;
      }

      if(GameState->AnimEditor.Skeleton)
      {
        if(0 < AttachedEntity->AnimController->AnimStateCount)
        {
          Anim::animation* Animation = AttachedEntity->AnimController->Animations[0];
          if(UI::Button("Edit Attached Animation"))
          {
            int32_t AnimationPathIndex = GameState->Resources.GetAnimationPathIndex(AttachedEntity->AnimController->AnimationIDs[0]);
            EditAnimation::EditAnimation(&GameState->AnimEditor, Animation, GameState->Resources.AnimationPaths[AnimationPathIndex].Name);
          }
        }
        if(UI::Button("Insert keyframe"))
        {
          EditAnimation::InsertBlendedKeyframeAtTime(&GameState->AnimEditor, GameState->AnimEditor.PlayHeadTime);
        }
        if(GameState->AnimEditor.KeyframeCount > 0)
        {
          if(UI::Button("Delete keyframe"))
          {
            EditAnimation::DeleteCurrentKeyframe(&GameState->AnimEditor);
          }
          if(UI::Button("Export Animation"))
          {
            time_t     CurrentTime;
            struct tm* TimeInfo;
            char       AnimGroupName[30];
            time(&CurrentTime);
            TimeInfo = localtime(&CurrentTime);
            strftime(AnimGroupName, sizeof(AnimGroupName), "data/animations/%H_%M_%S.anim", TimeInfo);
            Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor, AnimGroupName);
          }
          if(GameState->AnimEditor.AnimationPath[0] != '\0')
          {
            if(UI::Button("Override Animation"))
            {
              Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor, GameState->AnimEditor.AnimationPath);
            }
          }
        }

        UI::DragFloat("Playhead Time", &GameState->AnimEditor.PlayHeadTime, -100, 100, 2.0f);
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 0);
        if(GameState->AnimEditor.KeyframeCount > 0)
        {
          {
            int32_t ActiveBoneIndex = GameState->AnimEditor.CurrentBone;
            UI::Combo("Bone", &ActiveBoneIndex, GameState->AnimEditor.Skeleton->Bones, GameState->AnimEditor.Skeleton->BoneCount, BoneArrayToString);
            EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, ActiveBoneIndex);
          }

          Anim::transform* Transform     = &GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe].Transforms[GameState->AnimEditor.CurrentBone];
          mat4             Mat4Transform = TransformToGizmoMat4(Transform);
          UI::DragFloat3("Translation", &Transform->Translation.X, -INFINITY, INFINITY, 10.0f);
          UI::DragFloat3("Rotation", &Transform->Rotation.X, -INFINITY, INFINITY, 720.0f);
          UI::DragFloat3("Scale", &Transform->Scale.X, -INFINITY, INFINITY, 10.0f);
        }
      }
    }
  }
}

void
MiscGUI(game_state* GameState, bool& g_ShowLightSettings, bool& g_ShowDisplaySet, bool& g_DrawMemoryMaps, bool& g_ShowCameraSettings, bool& g_ShowSceneSettings)
{
  if(UI::CollapsingHeader("Light Settings", &g_ShowLightSettings))
  {
    UI::DragFloat3("Diffuse", &GameState->R.LightDiffuseColor.X, 0, 1, 5);
    UI::DragFloat3("Specular", &GameState->R.LightSpecularColor.X, 0, 1, 5);
    UI::DragFloat3("Ambient", &GameState->R.LightAmbientColor.X, 0, 1, 5);
    UI::DragFloat3("Position", &GameState->R.LightPosition.X, -INFINITY, INFINITY, 5);
    UI::Checkbox("Show gizmo", &GameState->R.ShowLightPosition);
  }

  if(UI::CollapsingHeader("Render Switches", &g_ShowDisplaySet))
  {
    UI::Checkbox("Memory Visualization", &g_DrawMemoryMaps);
    UI::Checkbox("Draw Gizmos", &GameState->DrawGizmos);
    UI::Checkbox("Timeline", &GameState->DrawTimeline);
    UI::Checkbox("Draw Debug Spheres", &GameState->DrawDebugSpheres);
    UI::Checkbox("Cubemap", &GameState->DrawCubemap);
  }
  if(UI::CollapsingHeader("Camera", &g_ShowCameraSettings))
  {
    UI::SliderFloat("FieldOfView", &GameState->Camera.FieldOfView, 0, 180);
    UI::SliderFloat("Near CLip Plane", &GameState->Camera.NearClipPlane, 0.01f, 500);
    UI::SliderFloat("Far  Clip Plane", &GameState->Camera.FarClipPlane, GameState->Camera.NearClipPlane, 500);
    UI::SliderFloat("Speed", &GameState->Camera.Speed, 0, 100);
  }
  if(UI::CollapsingHeader("Scene", &g_ShowSceneSettings))
  {
    if(UI::Button("Export As New"))
    {
      struct tm* TimeInfo;
      time_t     CurrentTime;
      char       PathName[60];
      time(&CurrentTime);
      TimeInfo = localtime(&CurrentTime);
      strftime(PathName, sizeof(PathName), "data/scenes/%H_%M_%S.scene", TimeInfo);
      ExportScene(GameState, PathName);
    }
    if(GameState->Resources.ScenePathCount > 0)
    {
      {
        static int32_t SelectedSceneIndex = 0;
        if(UI::Button("Export"))
        {
          ExportScene(GameState, GameState->Resources.ScenePaths[SelectedSceneIndex].Name);
        }
        UI::SameLine();
        UI::Combo("Export Path", &SelectedSceneIndex, GameState->Resources.ScenePaths, GameState->Resources.ScenePathCount, PathArrayToString);
        UI::SameLine();
        UI::NewLine();
      }
      {
        static int32_t SelectedSceneIndex = 0;
        if(UI::Button("Import"))
        {
          ImportScene(GameState, GameState->Resources.ScenePaths[SelectedSceneIndex].Name);
        }
        UI::SameLine();
        UI::Combo("Import Path", &SelectedSceneIndex, GameState->Resources.ScenePaths, GameState->Resources.ScenePathCount, PathArrayToString);
        UI::SameLine();
        UI::NewLine();
      }
    }
  }
}
