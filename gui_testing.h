#include "ui.h"
#include "scene.h"
#include "shader_def.h"

void MaterialGUI(game_state* GameState, bool& ShowMaterialEditor);
void EntityGUI(game_state* GameState, bool& ShowEntityTools);
void AnimationGUI(game_state* GameState, bool& ShowAnimationEditor, bool& ShowEntityTools);
void MiscGUI(game_state* GameState, bool& ShowLightSettings, bool& ShowDisplaySet,
             bool& ShowCameraSettings, bool& ShowSceneSettings,
             bool& ShowPostProcessingSettings);

namespace UI
{
  void
  TestGui(game_state* GameState, const game_input* Input)
  {
    UI::BeginFrame(GameState, Input);

		static bool s_ShowDemoWindow     = false;
		static bool s_ShowPhysicsWindow  = false;
		static bool s_ShowProfilerWindow = false;

    UI::BeginWindow("Editor Window", { 1200, 50 }, { 700, 600 });
    {
      UI::Combo("Selection mode", (int32_t*)&GameState->SelectionMode, g_SelectionEnumStrings,
                SELECT_EnumCount, UI::StringArrayToString);

      static bool s_ShowMaterialEditor         = false;
      static bool s_ShowEntityTools            = false;
      static bool s_ShowAnimationEditor        = false;
      static bool s_ShowLightSettings          = false;
      static bool s_ShowDisplaySet             = false;
      static bool s_ShowCameraSettings         = false;
      static bool s_ShowSceneSettings          = false;
      static bool s_ShowPostProcessingSettings = false;

			UI::Checkbox("Profiler Window", &s_ShowProfilerWindow);
			UI::Checkbox("Physics Window", &s_ShowPhysicsWindow);
			UI::Checkbox("GUI Params Window", &s_ShowDemoWindow);
      EntityGUI(GameState, s_ShowEntityTools);
      MaterialGUI(GameState, s_ShowMaterialEditor);
      AnimationGUI(GameState, s_ShowAnimationEditor, s_ShowEntityTools);
      MiscGUI(GameState, s_ShowLightSettings, s_ShowDisplaySet, s_ShowCameraSettings, s_ShowSceneSettings, s_ShowPostProcessingSettings);

    }
    UI::EndWindow();

		if(s_ShowProfilerWindow)
		{
			UI::BeginWindow("Profiler Window", { 150, 500 }, { 800, 380 });
			UI::EndWindow();
		}

		if(s_ShowPhysicsWindow)
		{
			UI::BeginWindow("Physics Window", { 150, 50 }, { 500, 380 });
			{
				UI::Checkbox("Simulating Dynamics", &GameState->SimulateDynamics);
				UI::SliderInt("Iteration Count", &GameState->PGSIterationCount, 0, 200);
				UI::SliderFloat("Beta", &GameState->Beta, 0.0f, 1.0f / (FRAME_TIME_MS / 1000.0f));
				GameState->PerformDynamicsStep = UI::Button("Step Dynamics");
				UI::Checkbox("Gravity", &GameState->UseGravity);
				// UI::SliderFloat("Restitution", &GameState->Restitution, 0.0f, 1.0f);
				// UI::SliderFloat("Slop", &GameState->Slop, 0.0f, 1.0f);
				UI::Checkbox("Friction", &GameState->SimulateFriction);
				UI::SliderFloat("Mu", &GameState->Mu, 0.0f, 1.0f);

				UI::Checkbox("Draw Omega    (green)", &GameState->VisualizeOmega);
				UI::Checkbox("Draw V        (yellow)", &GameState->VisualizeV);
				UI::Checkbox("Draw Fc       (red)", &GameState->VisualizeFc);
				UI::Checkbox("Draw Friction (green)", &GameState->VisualizeFriction);
				UI::Checkbox("Draw Fc Comopnents     (Magenta)", &GameState->VisualizeFcComponents);
				UI::Checkbox("Draw Contact Points    (while)", &GameState->VisualizeContactPoints);
				UI::Checkbox("Draw Contact Manifolds (blue/red)", &GameState->VisualizeContactManifold);
				UI::DragFloat3("Net Force Start", &GameState->ForceStart.X, -INFINITY, INFINITY, 5);
				UI::DragFloat3("Net Force Vector", &GameState->Force.X, -INFINITY, INFINITY, 5);
				UI::Checkbox("Apply Force", &GameState->ApplyingForce);
				UI::Checkbox("Apply Torque", &GameState->ApplyingTorque);
				// Debug::PushLine(GameState->ForceStart, GameState->ForceStart + GameState->Force,
				//{ 1, 1, 0, 1 });
				// Debug::PushWireframeSphere(GameState->ForceStart + GameState->Force, 0.05f, { 1, 1, 0, 1
				// });
			}
			UI::EndWindow();
		}

		if(s_ShowDemoWindow)
		{
			UI::BeginWindow("window A", { 670, 50 }, { 500, 380 });
			{
				static int         s_CurrentItem = -1;
				static const char* s_Items[]     = { "Cat", "Rat", "Hat", "Pat", "meet", "with", "dad" };

				static bool s_ShowDemo = true; if(UI::CollapsingHeader("Demo", &s_ShowDemo))
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
					UI::Combo("Combo test1", &s_CurrentItem, s_Items + StartIndex,
										ARRAY_SIZE(s_Items) - StartIndex);
					UI::NewLine();

					char TempBuff[30];
					snprintf(TempBuff, sizeof(TempBuff), "Wheel %d", Input->MouseWheelScreen);
					UI::Text(TempBuff);

					snprintf(TempBuff, sizeof(TempBuff), "Mouse Screen: { %d, %d }", Input->MouseScreenX,
									 Input->MouseScreenY);
					UI::Text(TempBuff);
					snprintf(TempBuff, sizeof(TempBuff), "Mouse Normal: { %.1f, %.1f }", Input->NormMouseX,
									 Input->NormMouseY);
					UI::Text(TempBuff);
					snprintf(TempBuff, sizeof(TempBuff), "delta time: %f ms", Input->dt);
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
		}

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
        UI::Combo("Material", &ActivePathIndex, GameState->Resources.MaterialPaths,
                  GameState->Resources.MaterialPathCount, PathArrayToString);
        if(GameState->Resources.MaterialPathCount > 0)
        {
          rid NewRID = { 0 };
          if(GameState->Resources
               .GetMaterialPathRID(&NewRID,
                                   GameState->Resources.MaterialPaths[ActivePathIndex].Name))
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
        UI::Image("Material preview", GameState->IDTexture, { 500, 300 });

        material* CurrentMaterial = GameState->Resources.GetMaterial(GameState->CurrentMaterialID);

        // Select shader type
        {
          int32_t NewShaderType = CurrentMaterial->Common.ShaderType;
          UI::Combo("Shader Type", (int32_t*)&NewShaderType, g_ShaderTypeEnumStrings,
                    SHADER_EnumCount, UI::StringArrayToString, 6, 200);
          if(CurrentMaterial->Common.ShaderType != NewShaderType)
          {
            *CurrentMaterial                   = {};
            CurrentMaterial->Common.ShaderType = NewShaderType;
          }
        }
        /*
        if(UI::Button("Previous shader"))
        {
          if(CurrentMaterial->Common.ShaderType > 0)
          {
            int32_t ShaderType                 = CurrentMaterial->Common.ShaderType - 1;
            *CurrentMaterial                   = {};
            CurrentMaterial->Common.ShaderType = ShaderType;
          }
        }
        UI::SameLine();
        if(UI::Button("Next shader"))
        {
          if(CurrentMaterial->Common.ShaderType < SHADER_EnumCount - 1)
          {
            int32_t ShaderType                 = CurrentMaterial->Common.ShaderType + 1;
            *CurrentMaterial                   = {};
            CurrentMaterial->Common.ShaderType = ShaderType;
          }
        }
        UI::SameLine();
        UI::NewLine();
        */

        UI::Checkbox("Blending", &CurrentMaterial->Common.UseBlending);
        UI::SameLine();
        UI::Checkbox("Skeletel", &CurrentMaterial->Common.IsSkeletal);
        UI::SameLine();
        UI::NewLine();

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
            UI::Checkbox("Diffuse Map", &UseDIffuse);
            UI::Checkbox("Specular Map", &UseSpecular);
            UI::Checkbox("Normal Map", &NormalFlagValue);

            UI::DragFloat3("Ambient Color", &CurrentMaterial->Phong.AmbientColor.X, 0.0f, 1.0f,
                           5.0f);

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

                  UI::Combo("Diffuse Map", &ActivePathIndex, GameState->Resources.TexturePaths,
                            GameState->Resources.TexturePathCount, PathArrayToString);
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
              UI::DragFloat4("Diffuse Color", &CurrentMaterial->Phong.DiffuseColor.X, 0.0f, 1.0f,
                             5.0f);
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
                UI::Combo("Specular Map", &ActivePathIndex, GameState->Resources.TexturePaths,
                          GameState->Resources.TexturePathCount, PathArrayToString);
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
              UI::DragFloat3("Specular Color", &CurrentMaterial->Phong.SpecularColor.X, 0.0f, 1.0f,
                             5.0f);
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
                UI::Combo("Normal map", &ActivePathIndex, GameState->Resources.TexturePaths,
                          GameState->Resources.TexturePathCount, PathArrayToString);
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

            UI::SliderFloat("Shininess", &CurrentMaterial->Phong.Shininess, 1.0f, 512.0f);
          }
          break;
          case SHADER_Env:
          {
            bool Refraction = CurrentMaterial->Env.Flags & ENV_UseRefraction;
            bool NormalMap  = CurrentMaterial->Env.Flags & ENV_UseNormalMap;
            UI::Checkbox("Refraction", &Refraction);
            UI::Checkbox("Normal Map", &NormalMap);

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
                UI::Combo("Normal map", &ActivePathIndex, GameState->Resources.TexturePaths,
                          GameState->Resources.TexturePathCount, PathArrayToString);
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

            UI::SliderFloat("Refractive Index", &CurrentMaterial->Env.RefractiveIndex, 1.0f, 3.0f);
          }
          break;
          case SHADER_Toon:
          {
            UI::DragFloat3("Ambient Color", &CurrentMaterial->Toon.AmbientColor.X, 0.0f, 1.0f,
                           5.0f);
            UI::DragFloat4("Diffuse Color", &CurrentMaterial->Toon.DiffuseColor.X, 0.0f, 1.0f,
                           5.0f);
            UI::DragFloat3("Specular Color", &CurrentMaterial->Toon.SpecularColor.X, 0.0f, 1.0f,
                           5.0f);

            UI::SliderFloat("Shininess", &CurrentMaterial->Toon.Shininess, 1.0f, 512.0f);
            UI::SliderInt("LevelCount", &CurrentMaterial->Toon.LevelCount, 1, 10);
          }
          break;
          case SHADER_LightWave:
          {
            UI::DragFloat3("Ambient Color", &CurrentMaterial->LightWave.AmbientColor.X, 0.0f, 1.0f,
                           5.0f);
            UI::DragFloat4("Diffuse Color", &CurrentMaterial->LightWave.DiffuseColor.X, 0.0f, 1.0f,
                           5.0f);
            UI::DragFloat3("Specular Color", &CurrentMaterial->LightWave.SpecularColor.X, 0.0f, 1.0f,
                           5.0f);

            UI::SliderFloat("Shininess", &CurrentMaterial->LightWave.Shininess, 1.0f, 512.0f);
          }
          break;
          case SHADER_Color:
          {
            UI::DragFloat4("Color", &CurrentMaterial->Color.Color.X, 0, 1, 5);
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
                    UI::SliderInt(ShaderParamDef.Name, ParamPtr, 0, 10);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Bool:
                  {
                    bool* ParamPtr = (bool*)ParamLocation;
                    UI::Checkbox(ShaderParamDef.Name, ParamPtr);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Float:
                  {
                    float* ParamPtr = (float*)ParamLocation;
                    UI::DragFloat(ShaderParamDef.Name, ParamPtr, -100, 100, 2.0f);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Vec3:
                  {
                    vec3* ParamPtr = (vec3*)ParamLocation;
                    UI::DragFloat3(ShaderParamDef.Name, (float*)ParamPtr, 0, INFINITY, 10);
                  }
                  break;
                  case SHADER_PARAM_TYPE_Vec4:
                  {
                    vec4* ParamPtr = (vec4*)ParamLocation;
                    UI::DragFloat4(ShaderParamDef.Name, (float*)ParamPtr, 0, INFINITY, 10);
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
                      UI::Combo(ShaderParamDef.Name, &ActivePathIndex,
                                GameState->Resources.TexturePaths,
                                GameState->Resources.TexturePathCount, PathArrayToString);
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
            if(UI::Button("Save"))
            {
              ExportMaterial(&GameState->Resources, CurrentMaterial, CurrentMaterialPath);
            }
          }
        }
        UI::SameLine();
        if(UI::Button("Duplicate Current"))
        {
          GameState->CurrentMaterialID =
            GameState->Resources.CreateMaterial(*CurrentMaterial, NULL);
          printf("Created Material with rid: %d\n", GameState->CurrentMaterialID.Value);
        }
          // Bad Idea, because texture RIDs cannot be 0
#if 0
        UI::SameLine();
        if(UI::Button("Clear Material Fields"))
        {
          uint32_t ShaderType                = CurrentMaterial->Common.ShaderType;
          *CurrentMaterial                   = {};
          CurrentMaterial->Common.ShaderType = ShaderType;
        }
#endif
        UI::SameLine();
        entity* SelectedEntity = {};
        if(UI::Button("Create New"))
        {
          GameState->CurrentMaterialID =
            GameState->Resources.CreateMaterial(NewPhongMaterial(), NULL);
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
          UI::SameLine();
          if(GameState->SelectionMode == SELECT_Mesh)
          {
            if(UI::Button("Edit Selected"))
            {
              GameState->CurrentMaterialID =
                SelectedEntity->MaterialIDs[GameState->SelectedMeshIndex];
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
    UI::Combo("Model", &ActivePathIndex, GameState->Resources.ModelPaths,
              GameState->Resources.ModelPathCount, PathArrayToString);
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
      UI::DragFloat3("Translation", (float*)&Transform->Translation, -INFINITY, INFINITY, 10);
      UI::DragFloat3("Rotation", (float*)&Transform->Rotation, -INFINITY, INFINITY, 720.0f);
      UI::DragFloat3("Scale", (float*)&Transform->Scale, -INFINITY, INFINITY, 10.0f);

      // Rigid Body
      {
        rigid_body* RB = &SelectedEntity->RigidBody;
        // UI::DragFloat3("X", &RB->X.X, -INFINITY, INFINITY, 10);

        if(FloatsEqualByThreshold(Math::Length(RB->q), 0.0f, 0.00001f))
        {
          RB->q.S = 1;
          RB->q.V = {};
        }
        UI::DragFloat4("q", &RB->q.S, -INFINITY, INFINITY, 10);
        Math::Normalize(&RB->q);

        if(UI::Button("Clear v"))
        {
          RB->v = {};
        }
        UI::DragFloat3("v", &RB->v.X, -INFINITY, INFINITY, 10);
        if(UI::Button("Clear w"))
        {
          RB->w = {};
        }
        UI::DragFloat3("w", &RB->w.X, -INFINITY, INFINITY, 10);

        UI::Checkbox("Regard Gravity", &RB->RegardGravity);

        UI::DragFloat("Mass", &RB->Mass, 0, INFINITY, 10);
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
        UI::Text(TempBuffer);

        { // Inertia
          vec3 InertiaDiagonal = { RB->InertiaBody._11, RB->InertiaBody._22, RB->InertiaBody._33 };
          UI::DragFloat3("Body Space Inertia diagonal", &InertiaDiagonal.X, 0, INFINITY, 10);
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
      }

      Render::model* SelectedModel = GameState->Resources.GetModel(SelectedEntity->ModelID);
      if(SelectedModel->Skeleton)
      {
        if(!SelectedEntity->AnimController && UI::Button("Add Anim. Controller"))
        {
          SelectedEntity->AnimController =
            PushStruct(GameState->PersistentMemStack, Anim::animation_controller);
          *SelectedEntity->AnimController = {};

          SelectedEntity->AnimController->Skeleton = SelectedModel->Skeleton;
          SelectedEntity->AnimController->OutputTransforms =
            PushArray(GameState->PersistentMemStack,
                      ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT * SelectedModel->Skeleton->BoneCount,
                      Anim::transform);
          SelectedEntity->AnimController->BoneSpaceMatrices =
            PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          SelectedEntity->AnimController->ModelSpaceMatrices =
            PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
          SelectedEntity->AnimController->HierarchicalModelSpaceMatrices =
            PushArray(GameState->PersistentMemStack, SelectedModel->Skeleton->BoneCount, mat4);
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
            AttachEntityToAnimEditor(GameState, &GameState->AnimEditor,
                                     GameState->SelectedEntityIndex);
            // s_ShowAnimationEditor = true;
          }
          if(UI::Button("Play Animation"))
          {
            if(GameState->CurrentAnimationID.Value > 0)
            {
              if(GameState->Resources.GetAnimation(GameState->CurrentAnimationID)->ChannelCount ==
                 SelectedModel->Skeleton->BoneCount)
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
            UI::Combo("Animation", &ActivePathIndex, GameState->Resources.AnimationPaths,
                      GameState->Resources.AnimationPathCount, PathArrayToString);
            rid NewRID = { 0 };
            if(GameState->Resources.AnimationPathCount > 0 &&
               !GameState->Resources
                  .GetAnimationPathRID(&NewRID,
                                       GameState->Resources.AnimationPaths[ActivePathIndex].Name))
            {
              NewRID = GameState->Resources.RegisterAnimation(
                GameState->Resources.AnimationPaths[ActivePathIndex].Name);
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
              UI::Combo("Walk", &ActivePathIndex, GameState->Resources.AnimationPaths,
                        GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 &&
                 !GameState->Resources
                    .GetAnimationPathRID(&NewRID,
                                         GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(
                  GameState->Resources.AnimationPaths[ActivePathIndex].Name);
              }
              if(GameState->Resources.AnimationPathCount &&
                 SelectedEntity->AnimController->AnimationIDs[0].Value != NewRID.Value)
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
              UI::Combo("Run", &ActivePathIndex, GameState->Resources.AnimationPaths,
                        GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 &&
                 !GameState->Resources
                    .GetAnimationPathRID(&NewRID,
                                         GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(
                  GameState->Resources.AnimationPaths[ActivePathIndex].Name);
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
              UI::Combo("Idle", &ActivePathIndex, GameState->Resources.AnimationPaths,
                        GameState->Resources.AnimationPathCount, PathArrayToString);
              rid NewRID = { 0 };
              if(GameState->Resources.AnimationPathCount > 0 &&
                 !GameState->Resources
                    .GetAnimationPathRID(&NewRID,
                                         GameState->Resources.AnimationPaths[ActivePathIndex].Name))
              {
                NewRID = GameState->Resources.RegisterAnimation(
                  GameState->Resources.AnimationPaths[ActivePathIndex].Name);
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
AnimationGUI(game_state* GameState, bool& s_ShowAnimationEditor, bool& s_ShowEntityTools)
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

    if(UI::CollapsingHeader("Animation Editor", &s_ShowAnimationEditor))
    {
      if(UI::Button("Stop Editing"))
      {
        DettachEntityFromAnimEditor(GameState, &GameState->AnimEditor);
        GameState->SelectionMode = SELECT_Entity;
        s_ShowEntityTools        = true;
        s_ShowAnimationEditor    = false;
      }

      if(GameState->AnimEditor.Skeleton)
      {
        if(0 < AttachedEntity->AnimController->AnimStateCount)
        {
          Anim::animation* Animation = AttachedEntity->AnimController->Animations[0];
          if(UI::Button("Edit Attached Animation"))
          {
            int32_t AnimationPathIndex = GameState->Resources.GetAnimationPathIndex(
              AttachedEntity->AnimController->AnimationIDs[0]);
            EditAnimation::EditAnimation(&GameState->AnimEditor, Animation,
                                         GameState->Resources.AnimationPaths[AnimationPathIndex]
                                           .Name);
          }
        }
        if(UI::Button("Insert keyframe"))
        {
          EditAnimation::InsertBlendedKeyframeAtTime(&GameState->AnimEditor,
                                                     GameState->AnimEditor.PlayHeadTime);
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
            strftime(AnimGroupName, sizeof(AnimGroupName), "data/animations/%H_%M_%S.anim",
                     TimeInfo);
            Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor,
                                        AnimGroupName);
          }
          if(GameState->AnimEditor.AnimationPath[0] != '\0')
          {
            if(UI::Button("Override Animation"))
            {
              Asset::ExportAnimationGroup(GameState->TemporaryMemStack, &GameState->AnimEditor,
                                          GameState->AnimEditor.AnimationPath);
            }
          }
        }

        UI::DragFloat("Playhead Time", &GameState->AnimEditor.PlayHeadTime, -100, 100, 2.0f);
        EditAnimation::AdvancePlayHead(&GameState->AnimEditor, 0);
        if(GameState->AnimEditor.KeyframeCount > 0)
        {
          {
            int32_t ActiveBoneIndex = GameState->AnimEditor.CurrentBone;
            UI::Combo("Bone", &ActiveBoneIndex, GameState->AnimEditor.Skeleton->Bones,
                      GameState->AnimEditor.Skeleton->BoneCount, BoneArrayToString);
            EditAnimation::EditBoneAtIndex(&GameState->AnimEditor, ActiveBoneIndex);
          }

          Anim::transform* Transform =
            &GameState->AnimEditor.Keyframes[GameState->AnimEditor.CurrentKeyframe]
               .Transforms[GameState->AnimEditor.CurrentBone];
          mat4 Mat4Transform = TransformToGizmoMat4(Transform);
          UI::DragFloat3("Translation", &Transform->Translation.X, -INFINITY, INFINITY, 10.0f);
          UI::DragFloat3("Rotation", &Transform->Rotation.X, -INFINITY, INFINITY, 720.0f);
          UI::DragFloat3("Scale", &Transform->Scale.X, -INFINITY, INFINITY, 10.0f);
        }
      }
    }
  }
}

// TODO(Lukas) Add bit mask checkbox to the UI API
void
MiscGUI(game_state* GameState, bool& s_ShowLightSettings, bool& s_ShowDisplaySet,
        bool& s_ShowCameraSettings, bool& s_ShowSceneSettings,
        bool& s_ShowPostProcessingSettings)
{
  if(UI::CollapsingHeader("Light Settings", &s_ShowLightSettings))
  {
    UI::DragFloat3("Diffuse", &GameState->R.LightDiffuseColor.X, 0, 10, 5);
    UI::DragFloat3("Ambient", &GameState->R.LightAmbientColor.X, 0, 10, 5);
    UI::DragFloat3("Position", &GameState->R.LightPosition.X, -INFINITY, INFINITY, 5);
    UI::Checkbox("Show gizmo", &GameState->R.ShowLightPosition);

    UI::DragFloat3("Diffuse", &GameState->R.Sun.DiffuseColor.X, 0, 1, 5);
    UI::DragFloat3("Ambient", &GameState->R.Sun.AmbientColor.X, 0, 1, 5);

    UI::SliderFloat("Sun Z Angle", &GameState->R.Sun.RotationZ, 0.0f, 90.0f);
    UI::SliderFloat("Sun Y Angle", &GameState->R.Sun.RotationY, -180, 180);

		UI::SliderInt("Current Cascade Index", &GameState->R.Sun.CurrentCascadeIndex, 0, SHADOWMAP_CASCADE_COUNT-1);
    UI::Checkbox("Display sun-perspective depth map", &GameState->R.DrawShadowMap);
    UI::Checkbox("Real-time shadows", &GameState->R.RealTimeDirectionalShadows);
    if(UI::Button("Recompute Directional Shadows"))
    {
      GameState->R.RecomputeDirectionalShadows = true;
    }

    if(UI::Button("Clear Directional Shadows"))
    {
      GameState->R.ClearDirectionalShadows = true;
    }
  }

  if(UI::CollapsingHeader("Post-processing", &s_ShowPostProcessingSettings))
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

    UI::Checkbox("HDRTonemap", &HDRTonemap);
    UI::Checkbox("Bloom", &Bloom);
    UI::Checkbox("FXAA", &FXAA);
    UI::Checkbox("Blur", &Blur);
    UI::Checkbox("DepthOfField", &DepthOfField);
    UI::Checkbox("MotionBlur", &MotionBlur);
    UI::Checkbox("Grayscale", &Grayscale);
    UI::Checkbox("NightVision", &NightVision);
    UI::Checkbox("EdgeOutline", &EdgeOutline);
    UI::Checkbox("DepthBuffer", &GameState->R.DrawDepthBuffer);
    UI::Checkbox("SSAO", &GameState->R.RenderSSAO);
    UI::Checkbox("SimpleFog", &SimpleFog);
    UI::Checkbox("VolumetricScattering", &GameState->R.RenderVolumetricScattering);
    UI::Checkbox("Noise", &Noise);
    UI::Checkbox("Test", &Test);

    if(HDRTonemap)
    {
      GameState->R.PPEffects |= POST_HDRTonemap;
      UI::SliderFloat("HDR Exposure", &GameState->R.ExposureHDR, 0.01f, 8.0f);
    }
    else
    {
      GameState->R.PPEffects &= ~POST_HDRTonemap;
    }

    if(Bloom)
    {
      GameState->R.PPEffects |= POST_Bloom;
      UI::SliderFloat("Bloom Threshold", &GameState->R.BloomLuminanceThreshold, 0.01f, 5.0f);
      UI::SliderInt("Bloom Blur Iterations", &GameState->R.BloomBlurIterationCount, 0, 5);
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
      UI::SliderFloat("StdDev", &GameState->R.PostBlurStdDev, 0.01f, 10.0f);
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
      UI::SliderFloat("SSAO Sample Radius", &GameState->R.SSAOSamplingRadius, 0.02f, 0.2f);
#if 0
    	UI::Image("Material preview", GameState->R.SSAOTexID, { 700, (int)(700.0 * (3.0f / 5.0f)) });
#endif
    }

    //UI::Image("ScenePreview", GameState->R.LightScatterTextures[0], { 500, 300 });

    if(SimpleFog)
    {
      GameState->R.PPEffects |= POST_SimpleFog;
      UI::SliderFloat("FogDensity", &GameState->R.FogDensity, 0.01f, 0.5f);
      UI::SliderFloat("FogGradient", &GameState->R.FogGradient, 1.0f, 10.0f);
      UI::SliderFloat("FogColor", &GameState->R.FogColor, 0.0f, 1.0f);
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
  }

  if(UI::CollapsingHeader("Render Switches", &s_ShowDisplaySet))
  {
    UI::Checkbox("Draw Gizmos", &GameState->DrawGizmos);
    //UI::Checkbox("Timeline", &GameState->DrawTimeline);
    UI::Checkbox("Draw Debug Spheres", &GameState->DrawDebugSpheres);
    UI::Checkbox("Cubemap", &GameState->DrawCubemap);
  }

  if(UI::CollapsingHeader("Camera", &s_ShowCameraSettings))
  {
    UI::SliderFloat("FieldOfView", &GameState->Camera.FieldOfView, 0, 180);
    UI::SliderFloat("Near CLip Plane", &GameState->Camera.NearClipPlane, 0.01f, 500);
    UI::SliderFloat("Far  Clip Plane", &GameState->Camera.FarClipPlane,
                    GameState->Camera.NearClipPlane, 500);
    UI::SliderFloat("Speed", &GameState->Camera.Speed, 0, 100);
  }
  if(UI::CollapsingHeader("Scene", &s_ShowSceneSettings))
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
        UI::Combo("Export Path", &SelectedSceneIndex, GameState->Resources.ScenePaths,
                  GameState->Resources.ScenePathCount, PathArrayToString);
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
        UI::Combo("Import Path", &SelectedSceneIndex, GameState->Resources.ScenePaths,
                  GameState->Resources.ScenePathCount, PathArrayToString);
        UI::SameLine();
        UI::NewLine();
      }
    }
  }
}
