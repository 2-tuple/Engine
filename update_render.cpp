#include "load_shader.h"

#include "linear_math/matrix.h"
#include "linear_math/vector.h"
#include "linear_math/distribution.h"

#include "game.h"
#include "mesh.h"
#include "model.h"
#include "asset.h"
#include "load_texture.h"
#include "misc.h"
#include "intersection_testing.h"
#include "render_data.h"
#include "material_upload.h"
#include "material_io.h"

#include "text.h"

#include "debug_drawing.h"
#include "camera.h"
#include "player_controller.h"

#include "shader_def.h"
#include "profile.h"
#include "dynamics.h"
#include "gui_testing.h"

#include "initialization.h"
#include "edit_mode_interaction.h"
#include "rendering.h"
#include "post_processing.h"

extern bool g_VisualizeContactPoints;
extern bool g_VisualizeContactManifold;

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	BEGIN_FRAME();

  game_state* GameState = (game_state*)GameMemory.PersistentMemory;
  assert(GameMemory.HasBeenInitialized);

  // GAME STATE INITIALIZATION (ONLY RUN ON FIRST FRAME)
  if(GameState->MagicChecksum != 123456)
  {
		TIMED_BLOCK(FirstInit);
		PartitionMemoryInitAllocators(&GameMemory, GameState);
		LoadInitialResources(GameState);
		SetGameStatePODFields(GameState);
  }
	
	BEGIN_TIMED_BLOCK(Update)

	BEGIN_TIMED_BLOCK(FilesystemUpdate);

  GameState->Resources.UpdateHardDriveAssetPathLists();
  GameState->Resources.DeleteUnused();
  GameState->Resources.ReloadModified();

	END_TIMED_BLOCK(FilesystemUpdate);

  if(GameState->CurrentMaterialID.Value > 0 && GameState->Resources.MaterialPathCount <= 0)
  {
    GameState->CurrentMaterialID = {};
  }
  if(GameState->CurrentMaterialID.Value <= 0)
  {
    if(GameState->Resources.MaterialPathCount > 0)
    {
      GameState->CurrentMaterialID =
        GameState->Resources.RegisterMaterial(GameState->Resources.MaterialPaths[0].Name);
    }
    else
    {
      GameState->CurrentMaterialID =
        GameState->Resources.CreateMaterial(NewPhongMaterial(), "data/materials/default.mat");
    }
  }

  if(Input->IsMouseInEditorMode)
  {
		EditWorldAndInteractWithGUI(GameState, Input);
  }

  //--------------------WORLD UPDATE------------------------

  UpdateCamera(&GameState->Camera, Input);

	{
		TIMED_BLOCK(Physics);

		assert(GameState->EntityCount <= RIGID_BODY_MAX_COUNT);
		GameState->Physics.RBCount = GameState->EntityCount;

		{
			g_VisualizeContactPoints = GameState->Physics.Switches.VisualizeContactPoints;
			g_VisualizeContactManifold = GameState->Physics.Switches.VisualizeContactManifold;
			//Copy entity transform state into the physics world
			//Note: valid entiteis are always stored without gaps in their array
			for(int i = 0; i < GameState->EntityCount; i++)
			{
				//Copy rigid body from entity (Mainly needed when loading scenes)
				GameState->Physics.RigidBodies[i] = GameState->Entities[i].RigidBody;

				GameState->Physics.RigidBodies[i].q =
					Math::EulerToQuat(GameState->Entities[i].Transform.Rotation);
				GameState->Physics.RigidBodies[i].X = GameState->Entities[i].Transform.Translation;

				GameState->Physics.RigidBodies[i].R =
					Math::Mat4ToMat3(Math::Mat4Rotate(GameState->Entities[i].Transform.Rotation));

				GameState->Physics.RigidBodies[i].Mat4Scale =
					Math::Mat4Scale(GameState->Entities[i].Transform.Scale);

				GameState->Physics.RigidBodies[i].Collider =
					GameState->Resources.GetModel(GameState->Entities[i].ModelID)->Meshes[0];

				const rigid_body& RB = GameState->Physics.RigidBodies[i];
				if(GameState->Physics.Switches.VisualizeOmega)
				{
					Debug::PushLine(RB.X, RB.X + RB.w, { 0, 1, 0, 1 });
					Debug::PushWireframeSphere(RB.X + RB.w, 0.05f, { 0, 1, 0, 1 });
				}
				if(GameState->Physics.Switches.VisualizeV)
				{
					Debug::PushLine(RB.X, RB.X + RB.v, { 1, 1, 0, 1 });
					Debug::PushWireframeSphere(RB.X + RB.v, 0.05f, { 1, 1, 0, 1 });
				}
			}
		}

		//Actual physics work
		SimulateDynamics(&GameState->Physics);

		for(int i = 0; i < GameState->EntityCount; i++)
		{
			GameState->Entities[i].RigidBody             = GameState->Physics.RigidBodies[i];
			GameState->Entities[i].Transform.Rotation    = Math::QuatToEuler(GameState->Physics.RigidBodies[i].q);
			GameState->Entities[i].Transform.Translation = GameState->Physics.RigidBodies[i].X;
		}
	}

  if(GameState->PlayerEntityIndex != -1)
  {
    entity* PlayerEntity = {};
    if(GetEntityAtIndex(GameState, &PlayerEntity, GameState->PlayerEntityIndex))
    {
      Gameplay::UpdatePlayer(PlayerEntity, Input);
    }
  }

  if(GameState->R.ShowLightPosition)
  {
    mat4 Mat4LightPosition = Math::Mat4Translate(GameState->R.LightPosition);
    Debug::PushGizmo(&GameState->Camera, &Mat4LightPosition);
  }

  GameState->R.CumulativeTime += Input->dt;

	BEGIN_TIMED_BLOCK(AnimationSystem);
  // -----------ENTITY ANIMATION UPDATE-------------
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    Anim::animation_controller* Controller = GameState->Entities[e].AnimController;
    if(Controller)
    {
      for(int i = 0; i < Controller->AnimStateCount; i++)
      {
        assert(Controller->AnimationIDs[i].Value > 0);
        Controller->Animations[i] = GameState->Resources.GetAnimation(Controller->AnimationIDs[i]);
      }
      Anim::UpdateController(Controller, Input->dt, Controller->BlendFunc);
    }
  }

  //----------ANIMATION EDITOR INTERACTION-----------
  if(Input->IsMouseInEditorMode && GameState->SelectionMode == SELECT_Bone &&
     GameState->AnimEditor.Skeleton)
  {
		AnimationEditorInteraction(GameState, Input);
  }
	END_TIMED_BLOCK(AnimationSystem);
	END_TIMED_BLOCK(Update);
	
  //---------------------RENDERING----------------------------
	BEGIN_TIMED_BLOCK(Render);
	
  // RENDER QUEUE SUBMISSION
  GameState->R.MeshInstanceCount = 0;
  for(int e = 0; e < GameState->EntityCount; e++)
  {
    Render::model* CurrentModel = GameState->Resources.GetModel(GameState->Entities[e].ModelID);
    for(int m = 0; m < CurrentModel->MeshCount; m++)
    {
      mesh_instance MeshInstance = {};
      MeshInstance.Mesh     = CurrentModel->Meshes[m];
      MeshInstance.Material =
        GameState->Resources.GetMaterial(GameState->Entities[e].MaterialIDs[m]);
      MeshInstance.EntityIndex = e;
      AddMeshInstance(&GameState->R, MeshInstance);
    }
  }

  RenderGBufferDataToTextures(GameState);
	
  // Saving previous frame entity MVP matrix (USED ONLY FOR MOTION BLUR)
  {
    for(int e = 0; e < GameState->EntityCount; e++)
    {
      GameState->PrevFrameMVPMatrices[e] = GetEntityMVPMatrix(GameState, e);
    }
  }


	RenderShadowmapCascadesToTextures(GameState);

	if(GameState->R.RenderSSAO)
	{
		RenderSSAOToTexture(GameState);
	}

  if(GameState->R.RenderVolumetricScattering)
  {
		RenderVolumeLightingToTexture(GameState);
	}

  {
    glBindFramebuffer(GL_FRAMEBUFFER, GameState->R.HdrFBOs[0]);
    glClearColor(0.3f, 0.4f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderMainSceneObjects(GameState);

    if(GameState->DrawCubemap)
		{
			RenderCubemap(GameState);
		}

    entity* SelectedEntity;
    if(Input->IsMouseInEditorMode && GetSelectedEntity(GameState, &SelectedEntity))
    {
			RenderObjectSelectionHighlighting(GameState, SelectedEntity);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  // RENDERING MATERIAL PREVIEW TO TEXTURE
  // TODO(Lukas) only render preview if material uses time or parameters were changed
  if(GameState->CurrentMaterialID.Value > 0)
  {
		RenderMaterialPreviewToTexture(GameState);
  }

	//--------------Post Processing-----------------
	PerformPostProcessing(GameState);

  //---------------DEBUG DRAWING------------------
	BEGIN_TIMED_BLOCK(DebugDrawingSubmission);
  if(GameState->DrawDebugSpheres)
  {
		Debug::DrawWireframeSpheres(GameState);
	}
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  if(GameState->DrawGizmos)
  {
    Debug::DrawGizmos(GameState);
  }
  if(GameState->DrawDebugLines)
  {
		Debug::DrawLines(GameState);
	} Debug::DrawQuads(GameState);
  Debug::ClearDrawArrays();
  Text::ClearTextRequestCounts();
	END_TIMED_BLOCK(DebugDrawingSubmission);

	END_TIMED_BLOCK(Render);
	END_FRAME();
}
