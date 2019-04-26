#include "scene.h"
#include "stack_alloc.h"
#include "file_io.h"
#include "asset.h"
#include "anim.h"
#include "rid.h"
#include "text.h"
#include <stdlib.h>
#include "profile.h"

struct rid_path_pair
{
  rid  RID;
  path Path;
};

struct scene
{
  int32_t EntityCount;
  entity* Entities;

  int32_t                     AnimControllerCount;
  Anim::animation_controller* AnimControllers;

  int32_t        ModelCount;
  rid_path_pair* ModelIDPaths;

  int32_t        AnimationCount;
  rid_path_pair* AnimationIDPaths;

  int32_t        MaterialCount;
  rid_path_pair* MaterialIDPaths;

  // AuxillaryElements
  // int32_t PlayerEntityIndex;
  camera Camera;

  vec3 LightPosition;
};

void
ExportScene(game_state* GameState, const char* Path)
{
  GameState->TemporaryMemStack->Clear();

  scene*   Scene     = PushStruct(GameState->TemporaryMemStack, scene);
  uint64_t AssetBase = (uint64_t)Scene;

  *Scene = {};

  Scene->EntityCount = GameState->EntityCount;
  if(Scene->EntityCount > 0)
  {
    Scene->Entities = PushArray(GameState->TemporaryMemStack, Scene->EntityCount, entity);
    memcpy(Scene->Entities, GameState->Entities, Scene->EntityCount * sizeof(entity));

    for(int e = 0; e < Scene->EntityCount; e++)
    {
      Render::model* CurrentModel    = GameState->Resources.GetModel(Scene->Entities[e].ModelID);
      Scene->Entities[e].MaterialIDs = PushArray(GameState->TemporaryMemStack, CurrentModel->MeshCount, rid);
      for(int m = 0; m < CurrentModel->MeshCount; m++)
      {
        Scene->Entities[e].MaterialIDs[m] = GameState->Entities[e].MaterialIDs[m];
      }
      Scene->Entities[e].MaterialIDs = (rid*)((uint64_t)Scene->Entities[e].MaterialIDs - AssetBase);
    }

    Scene->AnimControllers = (Anim::animation_controller*)GameState->TemporaryMemStack->GetMarker().Address;
    for(int e = 0; e < Scene->EntityCount; e++)
    {
      if(GameState->Entities[e].AnimController)
      {
        Scene->Entities[e].AnimController  = PushStruct(GameState->TemporaryMemStack, Anim::animation_controller);
        *Scene->Entities[e].AnimController = *GameState->Entities[e].AnimController;

        Scene->Entities[e].AnimController = (Anim::animation_controller*)((uint64_t)Scene->Entities[e].AnimController - AssetBase);
        ++Scene->AnimControllerCount;
      }
    }
    if(Scene->AnimControllerCount == 0)
    {
      Scene->AnimControllers = NULL;
    }
  }

  Scene->ModelIDPaths = (rid_path_pair*)GameState->TemporaryMemStack->GetMarker().Address;
  for(int i = 1; i <= MODEL_MAX_COUNT; i++)
  {
    Render::model* Model = {};
    char*          Path  = {};
    GameState->Resources.Models.Get({ i }, &Model, &Path);
    if(Path && Model)
    {
      rid_path_pair* NewPair = PushStruct(GameState->TemporaryMemStack, rid_path_pair);
      NewPair->RID           = { i };
      NewPair->Path          = {};
      strcpy(NewPair->Path.Name, Path);
      ++Scene->ModelCount;
    }
  }
  if(!Scene->ModelCount)
  {
    Scene->ModelIDPaths = NULL;
  }

  Scene->AnimationIDPaths = (rid_path_pair*)GameState->TemporaryMemStack->GetMarker().Address;
  for(int i = 1; i <= ANIMATION_MAX_COUNT; i++)
  {
    Anim::animation* Animation = {};
    char*            Path      = {};
    GameState->Resources.Animations.Get({ i }, &Animation, &Path);
    if(Path && Animation)
    {
      rid_path_pair* NewPair = PushStruct(GameState->TemporaryMemStack, rid_path_pair);
      NewPair->RID           = { i };
      NewPair->Path          = {};
      strcpy(NewPair->Path.Name, Path);
      ++Scene->AnimationCount;
    }
  }
  if(!Scene->AnimationCount)
  {
    Scene->AnimationIDPaths = NULL;
  }

  Scene->MaterialIDPaths = (rid_path_pair*)GameState->TemporaryMemStack->GetMarker().Address;
  for(int i = 1; i <= MATERIAL_MAX_COUNT; i++)
  {
    material* Material = {};
    char*     Path     = {};
    GameState->Resources.Materials.Get({ i }, &Material, &Path);
    if(Path && Material)
    {
      rid_path_pair* NewPair = PushStruct(GameState->TemporaryMemStack, rid_path_pair);
      NewPair->RID           = { i };
      NewPair->Path          = {};
      strcpy(NewPair->Path.Name, Path);
      ++Scene->MaterialCount;
    }
  }
  if(!Scene->MaterialCount)
  {
    Scene->MaterialIDPaths = NULL;
  }

  // Saving camera and light parameters
  Scene->Camera        = GameState->Camera;
  Scene->LightPosition = GameState->R.LightPosition;

  Scene->Entities         = (entity*)((uint64_t)Scene->Entities - AssetBase);
  Scene->AnimControllers  = (Anim::animation_controller*)((uint64_t)Scene->AnimControllers - AssetBase);
  Scene->ModelIDPaths     = (rid_path_pair*)((uint64_t)Scene->ModelIDPaths - AssetBase);
  Scene->AnimationIDPaths = (rid_path_pair*)((uint64_t)Scene->AnimationIDPaths - AssetBase);
  Scene->MaterialIDPaths  = (rid_path_pair*)((uint64_t)Scene->MaterialIDPaths - AssetBase);

  uint32_t TotalSize = GameState->TemporaryMemStack->GetUsedSize();

  Platform::WriteEntireFile(Path, TotalSize, Scene);
}

void
ImportScene(game_state* GameState, const char* Path)
{
	TIMED_BLOCK(ImportScene);
  printf("---------IMPORTING-SCENE: %s---------\n", Path);
  memset(&GameState->AnimEditor, 0, sizeof(EditAnimation::animation_editor));
  GameState->SelectionMode = SELECT_Entity;

  // Load the scene into the temp memory stack and fix pointers
  GameState->TemporaryMemStack->NullifyClear();

  debug_read_file_result ReadResult = Platform::ReadEntireFile(GameState->TemporaryMemStack, Path);
  assert(ReadResult.Contents);

  scene*   Scene     = (scene*)ReadResult.Contents;
  uint64_t AssetBase = (uint64_t)Scene;

  Scene->Entities = (entity*)((uint64_t)Scene->Entities + AssetBase);
  for(int e = 0; e < Scene->EntityCount; e++)
  {
    if(Scene->Entities[e].AnimController)
    {
      Scene->Entities[e].AnimController = (Anim::animation_controller*)((uint64_t)Scene->Entities[e].AnimController + AssetBase);
    }
    Scene->Entities[e].MaterialIDs = (rid*)((uint64_t)Scene->Entities[e].MaterialIDs + AssetBase);
  }

  Scene->AnimControllers  = (Anim::animation_controller*)((uint64_t)Scene->AnimControllers + AssetBase);
  Scene->ModelIDPaths     = (rid_path_pair*)((uint64_t)Scene->ModelIDPaths + AssetBase);
  Scene->AnimationIDPaths = (rid_path_pair*)((uint64_t)Scene->AnimationIDPaths + AssetBase);
  Scene->MaterialIDPaths  = (rid_path_pair*)((uint64_t)Scene->MaterialIDPaths + AssetBase);

  // Apply saved rid and path pairings to resource manager
  GameState->Resources.WipeAllModelData();
  GameState->Resources.WipeAllAnimationData();
  GameState->Resources.WipeAllTextureData();
  GameState->Resources.WipeAllMaterialData();

  RegisterDebugModels(GameState);
  for(int i = 0; i < Scene->ModelCount; i++)
  {
    assert(0 < Scene->ModelIDPaths[i].RID.Value);
    GameState->Resources.AssociateModelIDToPath(Scene->ModelIDPaths[i].RID, Scene->ModelIDPaths[i].Path.Name);
  }
  for(int i = 0; i < Scene->AnimationCount; i++)
  {
    assert(0 < Scene->AnimationIDPaths[i].RID.Value);
    GameState->Resources.AssociateAnimationIDToPath(Scene->AnimationIDPaths[i].RID, Scene->AnimationIDPaths[i].Path.Name);
  }
  for(int i = 0; i < Scene->MaterialCount; i++)
  {
    assert(Scene->MaterialIDPaths[i].RID.Value > 0);
    GameState->Resources.AssociateMaterialIDToPath(Scene->MaterialIDPaths[i].RID, Scene->MaterialIDPaths[i].Path.Name);
  }

  // Apply loaded scene to game state
  assert(Scene->EntityCount <= ENTITY_MAX_COUNT);

  memcpy(GameState->Entities, Scene->Entities, Scene->EntityCount * sizeof(entity));
  for(int e = 0; e < Scene->EntityCount; e++)
  {
    assert(GameState->Entities[e].ModelID.Value > 0);
    GameState->Resources.Models.AddReference(GameState->Entities[e].ModelID);
    Render::model* Model = GameState->Resources.GetModel(GameState->Entities[e].ModelID);
    if(Scene->Entities[e].AnimController)
    {
      // allocate memory for animation controller and assigin skeleton
      GameState->Entities[e].AnimController  = PushStruct(GameState->PersistentMemStack, Anim::animation_controller);
      *GameState->Entities[e].AnimController = *Scene->Entities[e].AnimController;

      for(int a = 0; a < GameState->Entities[e].AnimController->AnimStateCount; a++)
      {
        GameState->Entities[e].AnimController->Animations[a] = NULL;
        GameState->Resources.Animations.AddReference(GameState->Entities[e].AnimController->AnimationIDs[a]);
      }

      assert(Model->Skeleton);
      GameState->Entities[e].AnimController->Skeleton = Model->Skeleton;
      GameState->Entities[e].AnimController->OutputTransforms =
        PushArray(GameState->PersistentMemStack,
                  ANIM_CONTROLLER_OUTPUT_BLOCK_COUNT * Model->Skeleton->BoneCount, transform);
      GameState->Entities[e].AnimController->BoneSpaceMatrices  = PushArray(GameState->PersistentMemStack, Model->Skeleton->BoneCount, mat4);
      GameState->Entities[e].AnimController->ModelSpaceMatrices = PushArray(GameState->PersistentMemStack, Model->Skeleton->BoneCount, mat4);
      GameState->Entities[e].AnimController->HierarchicalModelSpaceMatrices = PushArray(GameState->PersistentMemStack, Model->Skeleton->BoneCount, mat4);
    }

    GameState->Entities[e].MaterialIDs = PushArray(GameState->PersistentMemStack, Model->MeshCount, rid);
    for(int m = 0; m < Model->MeshCount; m++)
    {
      GameState->Entities[e].MaterialIDs[m] = Scene->Entities[e].MaterialIDs[m];
    }
  }
  GameState->EntityCount = Scene->EntityCount;

  // Saving camera and light parameters
  GameState->Camera            = Scene->Camera;
  GameState->R.LightPosition   = Scene->LightPosition;
  GameState->CurrentMaterialID = { 0 };
  GameState->SelectedEntityIndex = -1;

  return;
}
