#pragma once

enum shader_type
{
  SHADER_Phong,
  SHADER_Color,
  //  SHADER_ID,

  SHADER_EnumCount,
};

union material {
  // material_header;
  struct material_header
  {
    uint32_t ShaderType;
    bool     UseBlending;
  } Common;

  struct
  {
    material_header Common;
    uint32_t        TextureIndex0;
    float           AmbientStrength;
    float           SpecularStrength;
  } Phong;
  struct
  {
    material_header Common;
    vec4            Color;
  } Color;
};

const int32_t TEXTURE_MAX_COUNT       = 20;
const int32_t MATERIAL_MAX_COUNT      = 20;
const int32_t MESH_INSTANCE_MAX_COUNT = 10000;
const int32_t MODEL_MAX_COUNT         = 10;

struct mesh_instance
{
  Render::mesh* Mesh;
  material*     Material;
  int32_t       EntityIndex;
};

struct render_data
{
  // Materials
  material      Materials[MATERIAL_MAX_COUNT];
  int32_t       MaterialCount;
  mesh_instance MeshInstances[MESH_INSTANCE_MAX_COUNT]; // Filled every frame
  int32_t       MeshInstanceCount;

  // Models
  Render::model* Models[MODEL_MAX_COUNT];
  int32_t        ModelCount;

  // Textures
  int32_t Textures[TEXTURE_MAX_COUNT];
  int32_t TextureCount;

  // Shaders
  uint32_t ShaderPhong;
  uint32_t ShaderSkeletalPhong;
  uint32_t ShaderSkeletalBoneColor;
  uint32_t ShaderColor;
  uint32_t ShaderGizmo;
  uint32_t ShaderQuad;
  uint32_t ShaderTexturedQuad;
  uint32_t ShaderCubemap;
  uint32_t ShaderID;

  // Light
  vec3 LightPosition;
  vec3 LightColor;
};

inline void
AddTexture(render_data* RenderData, int32_t TextureID)
{
  assert(TextureID);
  assert(0 <= RenderData->TextureCount && RenderData->TextureCount < TEXTURE_MAX_COUNT);
  RenderData->Textures[RenderData->TextureCount++] = TextureID;
}

inline material
NewPhongMaterial()
{
  material Material               = {};
  Material.Common.ShaderType      = SHADER_Phong;
  Material.Common.UseBlending     = true;
  Material.Phong.TextureIndex0    = 0;
  Material.Phong.AmbientStrength  = 0.8f;
  Material.Phong.SpecularStrength = 0.6f;
  return Material;
}

inline material
NewColorMaterial()
{
  material Material           = {};
  Material.Common.ShaderType  = SHADER_Color;
  Material.Common.UseBlending = true;
  Material.Color.Color        = vec4{ 1, 1, 0, 1 };
  return Material;
}

inline int32_t
AddMaterial(render_data* RenderData, material Material)
{
  assert(0 <= RenderData->MaterialCount && RenderData->MaterialCount < MATERIAL_MAX_COUNT);
  int32_t LastMaterialIndex = RenderData->MaterialCount;

  RenderData->Materials[RenderData->MaterialCount++] = Material;
  return LastMaterialIndex;
}

inline void
AddMeshInstance(render_data* RenderData, mesh_instance MeshInstance)
{
  assert(0 <= RenderData->MeshInstanceCount &&
         RenderData->MeshInstanceCount < MESH_INSTANCE_MAX_COUNT);
  RenderData->MeshInstances[RenderData->MeshInstanceCount++] = MeshInstance;
}

inline void
AddModel(render_data* RenderData, Render::model* Model)
{
  assert(0 <= RenderData->ModelCount && RenderData->ModelCount < MODEL_MAX_COUNT);
  RenderData->Models[RenderData->ModelCount++] = Model;
}
