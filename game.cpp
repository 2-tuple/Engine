#include "game.h"
#include "debug_drawing.h"

void
AttachEntityToAnimEditor(game_state* GameState, EditAnimation::animation_editor* Editor,
                         int32_t EntityIndex)
{
  entity* AddedEntity = {};
  if(GetEntityAtIndex(GameState, &AddedEntity, EntityIndex))
  {
    Render::model* Model = GameState->Resources.GetModel(AddedEntity->ModelID);
    assert(Model->Skeleton);
		
    /* #1 */ //*Editor = {};
		
		/* #2 */ memset(Editor, 0, sizeof(EditAnimation::animation_editor));

    Editor->Skeleton    = Model->Skeleton;
    Editor->Transform   = &AddedEntity->Transform;
    Editor->EntityIndex = EntityIndex;
  }
}

void
GetCubemapRIDs(rid* RIDs, Resource::resource_manager* Resources,
               Memory::stack_allocator* const Allocator, char* CubemapPath, char* FileFormat)
{
  char* CubemapFaces[6];
  CubemapFaces[0] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_right.") + 1));
  CubemapFaces[1] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_left.") + 1));
  CubemapFaces[2] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_top.") + 1));
  CubemapFaces[3] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_bottom.") + 1));
  CubemapFaces[4] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_back.") + 1));
  CubemapFaces[5] =
    (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(strlen("_front.") + 1));
  strcpy(CubemapFaces[0], "_right.\0");
  strcpy(CubemapFaces[1], "_left.\0");
  strcpy(CubemapFaces[2], "_top.\0");
  strcpy(CubemapFaces[3], "_bottom.\0");
  strcpy(CubemapFaces[4], "_back.\0");
  strcpy(CubemapFaces[5], "_front.\0");

  char* FileNames[6];
  FileNames[0] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[0]) + strlen(FileFormat) + 1)));
  FileNames[1] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[1]) + strlen(FileFormat) + 1)));
  FileNames[2] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[2]) + strlen(FileFormat) + 1)));
  FileNames[3] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[3]) + strlen(FileFormat) + 1)));
  FileNames[4] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[4]) + strlen(FileFormat) + 1)));
  FileNames[5] = (char*)Allocator->Alloc(Memory::SafeTruncate_size_t_To_uint32_t(
    (strlen(CubemapPath) + strlen(CubemapFaces[5]) + strlen(FileFormat) + 1)));

  for(int i = 0; i < 6; i++)
  {
    strcpy(FileNames[i], CubemapPath);
    strcat(FileNames[i], CubemapFaces[i]);
    strcat(FileNames[i], FileFormat);
    strcat(FileNames[i], "\0");
    if(!Resources->GetTexturePathRID(&RIDs[i], FileNames[i]))
    {
      RIDs[i] = Resources->RegisterTexture(FileNames[i]);
    }
  }
}

void
RegisterDebugModels(game_state* GameState)
{
  GameState->GizmoModelID       = GameState->Resources.RegisterModel("data/built/gizmo1.model");
  GameState->BoneDiamondModelID = GameState->Resources.RegisterModel("data/built/bone_diamond.model");
  GameState->QuadModelID     = GameState->Resources.RegisterModel("data/built/debug_meshes.model");
  GameState->CubemapModelID  = GameState->Resources.RegisterModel("data/built/inverse_cube.model");
  GameState->SphereModelID   = GameState->Resources.RegisterModel("data/built/sphere.model");
  GameState->LowPolySphereModelID   = GameState->Resources.RegisterModel("data/built/low_poly_sphere.model");
  GameState->UVSphereModelID = GameState->Resources.RegisterModel("data/built/uv_sphere.model");
  GameState->Resources.Models.AddReference(GameState->GizmoModelID);
  GameState->Resources.Models.AddReference(GameState->BoneDiamondModelID);
  GameState->Resources.Models.AddReference(GameState->QuadModelID);
  GameState->Resources.Models.AddReference(GameState->CubemapModelID);
  GameState->Resources.Models.AddReference(GameState->SphereModelID);
  GameState->Resources.Models.AddReference(GameState->LowPolySphereModelID);
  GameState->Resources.Models.AddReference(GameState->UVSphereModelID);
#if 1
  strcpy(GameState->R.Cubemap.Name, "data/textures/skybox2/SkyBox02b");
  strcpy(GameState->R.Cubemap.Format, "png");
#else
  strcpy(GameState->R.Cubemap.Name, "data/textures/skybox/morning");
  strcpy(GameState->R.Cubemap.Format, "tga");
#endif
  GetCubemapRIDs(GameState->R.Cubemap.FaceIDs, &GameState->Resources, GameState->TemporaryMemStack,
                 GameState->R.Cubemap.Name, GameState->R.Cubemap.Format);
  GameState->R.Cubemap.CubemapTexture = -1;
  for(int i = 0; i < 6; i++)
  {
    GameState->Resources.Textures.AddReference(GameState->R.Cubemap.FaceIDs[i]);
  }
}

uint32_t
LoadCubemap(Resource::resource_manager* Resources, rid* RIDs)
{
  path CubemapPaths[] = {
      Resources->TexturePaths[Resources->GetTexturePathIndex(RIDs[0])],
      Resources->TexturePaths[Resources->GetTexturePathIndex(RIDs[1])],
      Resources->TexturePaths[Resources->GetTexturePathIndex(RIDs[2])],
      Resources->TexturePaths[Resources->GetTexturePathIndex(RIDs[3])],
      Resources->TexturePaths[Resources->GetTexturePathIndex(RIDs[4])],
      Resources->TexturePaths[Resources->GetTexturePathIndex(RIDs[5])],
  };
  return Texture::LoadCubemapTexture(CubemapPaths);
}

void
GenerateScreenQuad(uint32_t* VAO, uint32_t* VBO)
{
  // Create VAO and VBO for screen quad
  // Position   // TexCoord
  float QuadVertices[] = { -1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                           1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
                           1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f };

  glGenVertexArrays(1, VAO);
  glGenBuffers(1, VBO);
  glBindVertexArray(*VAO);
  glBindBuffer(GL_ARRAY_BUFFER, *VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void
AddEntity(game_state* GameState, rid ModelID, rid* MaterialIDs, transform Transform)
{
  assert(0 <= GameState->EntityCount && GameState->EntityCount < ENTITY_MAX_COUNT);

  entity NewEntity      = {};
  NewEntity.ModelID     = ModelID;
  NewEntity.MaterialIDs = MaterialIDs;
  NewEntity.Transform   = Transform;
  GameState->Resources.Models.AddReference(ModelID);

  GameState->Entities[GameState->EntityCount++] = NewEntity;
}

// Note(Lukas): Currently should not be used as no references are actually added
void
RemoveReferencesAndResetAnimPlayer(Resource::resource_manager* Resources,
                                   Anim::animation_player* Player)
{
  for(int i = 0; i < Player->AnimStateCount; i++)
  {
    Resources->Animations.RemoveReference(Player->AnimationIDs[i]);
    Player->AnimationIDs[i] = {};
    Player->Animations[i]   = {};
    Player->States[i]       = {};
  }
  Player->AnimStateCount = 0;
  Player->BlendFunc = NULL;
}

void
RemoveAnimationPlayerComponent(game_state* GameState, Resource::resource_manager* Resources,
                               int32_t EntityIndex)
{
	entity* Entity;
  assert(GetEntityAtIndex(GameState, &Entity, EntityIndex));
	assert(Entity->AnimPlayer);

  // TODO(Lukas): REMOVE MEMORY LEAK!!!!!! The AnimPlayer and its arrays are still on
  // the persistent stack
  int MMControllerDataIndex = GetEntityMMDataIndex(EntityIndex, &GameState->MMEntityData);
  if(MMControllerDataIndex != -1)
	{
    RemoveMMControllerDataAtIndex(GameState->Entities, MMControllerDataIndex, Resources,
                                  &GameState->MMEntityData);
  }
  else
  {
    RemoveReferencesAndResetAnimPlayer(&GameState->Resources, Entity->AnimPlayer);
  }
  Entity->AnimPlayer = NULL;
}

bool
DeleteEntity(game_state* GameState, Resource::resource_manager* Resources, int32_t Index)
{
  if(0 <= Index && Index < GameState->EntityCount)
  {
    GameState->Resources.Models.RemoveReference(GameState->Entities[Index].ModelID);

		if(GameState->Entities[Index].AnimPlayer)
    {
      RemoveAnimationPlayerComponent(GameState, Resources, Index);
    }

    GameState->Entities[Index] = GameState->Entities[GameState->EntityCount - 1];

    // Fix up the index of the entity data which was moved into the removed spot
    int32_t MMEntityDataIndex;
    if((MMEntityDataIndex =
          GetEntityMMDataIndex(GameState->EntityCount - 1, &GameState->MMEntityData)) != -1)
    {
      mm_aos_entity_data MMEntity =
        GetAOSMMDataAtIndex(MMEntityDataIndex, &GameState->MMEntityData);
      *MMEntity.EntityIndex = Index;
    }

    --GameState->EntityCount;
    return true;
  }
  return false;
}

bool
GetEntityAtIndex(game_state* GameState, entity** OutputEntity, int32_t EntityIndex)
{
  if(GameState->EntityCount > 0)
  {
    if(0 <= EntityIndex && EntityIndex < GameState->EntityCount)
    {
      *OutputEntity = &GameState->Entities[EntityIndex];
      return true;
    }
  }

  *OutputEntity = NULL;
  return false;
}

void AttachEntityToAnimEditor(game_state* GameState, EditAnimation::animation_editor* Editor,
                              int32_t EntityIndex);

void
DettachEntityFromAnimEditor(const game_state* GameState, EditAnimation::animation_editor* Editor)
{
  assert(GameState->Entities[Editor->EntityIndex].AnimPlayer);
  assert(Editor->Skeleton);
  memset(Editor, 0, sizeof(EditAnimation::animation_editor));
	Editor->EntityIndex = -1;
}

bool
GetSelectedEntity(game_state* GameState, entity** OutputEntity)
{
  return GetEntityAtIndex(GameState, OutputEntity, GameState->SelectedEntityIndex);
}

bool
GetSelectedMesh(game_state* GameState, Render::mesh** OutputMesh)
{
  entity* Entity = NULL;
  if(GetSelectedEntity(GameState, &Entity))
  {
    Render::model* Model = GameState->Resources.GetModel(Entity->ModelID);
    if(Model->MeshCount > 0)
    {
      if(0 <= GameState->SelectedMeshIndex && GameState->SelectedMeshIndex < Model->MeshCount)
      {
        *OutputMesh = Model->Meshes[GameState->SelectedMeshIndex];
        return true;
      }
    }
  }
  return false;
}

mat4
GetEntityModelMatrix(game_state* GameState, int32_t EntityIndex)
{
  mat4 ModelMatrix = TransformToMat4(GameState->Entities[EntityIndex].Transform);
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
DrawSkeleton(const Anim::skeleton* Skeleton, const mat4* HierarchicalModelSpaceMatrices,
             mat4 MatModel, float JointSphereRadius, bool UseBoneDiamonds)
{
  const mat4 Mat4Root = Math::MulMat4(MatModel, Math::MulMat4(HierarchicalModelSpaceMatrices[0],
                                                              Skeleton->Bones[0].BindPose));
  for(int b = 0; b < Skeleton->BoneCount; b++)
  {
    mat4 Mat4Bone = Math::MulMat4(MatModel, Math::MulMat4(HierarchicalModelSpaceMatrices[b],
                                                          Skeleton->Bones[b].BindPose));
    vec3 Position = Mat4Bone.T;

    if(0 < b)
    {
      int  ParentIndex = Skeleton->Bones[b].ParentIndex;
      mat4 Mat4Parent =
        Math::MulMat4(MatModel, Math::MulMat4(HierarchicalModelSpaceMatrices[ParentIndex],
                                              Skeleton->Bones[ParentIndex].BindPose));
      vec3 ParentPosition = Mat4Parent.T;

      if(UseBoneDiamonds)
      {
        float BoneLength    = Math::Length(Position - ParentPosition);
        vec3  ParentToChild = Math::Normalized(Position - ParentPosition);

        vec3 Forward = Math::Normalized(
          Math::Cross(ParentToChild, { Mat4Root._11, Mat4Root._12, Mat4Root._13 }));
        vec3 Right = Math::Normalized(Math::Cross(ParentToChild, Forward));

        mat4 DiamondMatrix = Mat4Parent;

        DiamondMatrix.X = Right;
        DiamondMatrix.Y = ParentToChild;
        DiamondMatrix.Z = Forward;

        Debug::PushShadedBone(DiamondMatrix, BoneLength);
      }
      else
      {
        Debug::PushLine(Position, ParentPosition);
      }
    }
    Debug::PushWireframeSphere(Position, JointSphereRadius);
  }
}
