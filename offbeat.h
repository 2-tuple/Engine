#pragma once

#include <stdint.h>
#include <GL/glew.h>

typedef int8_t s8;
typedef int8_t s08;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;

typedef uint8_t u8;
typedef uint8_t u08;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef uintptr_t umm;

/* TODO(rytis):
 * Change over to SOA for faster processing.
 *
 * PROBABLY NOT NECESSARY:
 * Add memory alignment (16 byte, preferably changeable with macro or function parameter)
 * in OffbeatInit and memory manager.
 *
 * Add selectors (plane, sphere, cube ???). Allow testing selectors against a ray.
 *
 * Should try to find the weird framerate bug. Calculation speed does not seem to be impacted
 * but you can SEE the lower framerate.
 */

#if defined(__WIN32__) || defined(_WIN32) || defined(__WIN64__) || defined(_WIN64) || defined(WIN32)
#define OffbeatAssert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#include <assert.h>
#define OffbeatAssert(Expression) assert(Expression)
#endif

#define OffbeatKibibytes(Value) (1024LL * (Value))
#define OffbeatMibibytes(Value) (1024LL * OffbeatKibibytes(Value))
#define OffbeatGibibytes(Value) (1024LL * OffbeatMibibytes(Value))

#define OffbeatArrayCount(Array) sizeof((Array)) / sizeof(Array[0])

#define OffbeatOffsetOf(type, Member) (umm)&(((type*)0)->Member)

#include "offbeat_config.h"

#include "offbeat_math.h"
#include "offbeat_random.h"

#ifndef OFFBEAT_API
#define OFFBEAT_API
#endif

#ifdef OFFBEAT_OPENGL
typedef GLuint ob_texture;
typedef GLuint ob_program;
#else
typedef u32 ob_texture;
typedef u32 ob_program;
#endif

#define OFFBEAT_PARTICLE_SYSTEM_COUNT 512
#define OFFBEAT_DRAW_LIST_COUNT OFFBEAT_PARTICLE_SYSTEM_COUNT

struct ob_file_data
{
    u64 Size;
    void* Data;
};

struct ob_particle
{
    ov3 P;
    f32 Age;
    ov3 dP;
    f32 dAge;
    f32 ID;
    f32 Random;
    f32 Padding[2];
};

enum ob_function
{
    OFFBEAT_FunctionConst = 0,
    OFFBEAT_FunctionLerp = 1,
    OFFBEAT_FunctionMaxLerp = 2,
    OFFBEAT_FunctionTriangle = 3,
    OFFBEAT_FunctionTwoTriangles = 4,
    OFFBEAT_FunctionFourTriangles = 5,
    OFFBEAT_FunctionStep = 6,
    OFFBEAT_FunctionPeriodic = 7,
    OFFBEAT_FunctionPeriodicTime = 8,
    OFFBEAT_FunctionPeriodicSquare = 9,
    OFFBEAT_FunctionPeriodicSquareTime = 10,

    OFFBEAT_FunctionCount,
};

enum ob_parameter
{
    OFFBEAT_ParameterAge = 0,
    OFFBEAT_ParameterVelocity = 1,
    OFFBEAT_ParameterID = 2,
    OFFBEAT_ParameterRandom = 3,
    OFFBEAT_ParameterCollisionCount = 4,
    OFFBEAT_ParameterCameraDistance = 5,

    OFFBEAT_ParameterCount,
};

struct ob_expr
{
    ob_function Function;
    ob_parameter Parameter;
    f32 Float;
    u32 Uint;
    ov4 Low;
    ov4 High;
};

enum ob_emission_shape
{
    OFFBEAT_EmissionPoint = 0,
    OFFBEAT_EmissionRing = 1,
    OFFBEAT_EmissionDisk = 2,
    OFFBEAT_EmissionSquare = 3,
    OFFBEAT_EmissionSphere = 4,
    OFFBEAT_EmissionSphereVolume = 5,
    OFFBEAT_EmissionCubeVolume = 6,

    OFFBEAT_EmissionCount,
};

enum ob_emission_velocity
{
    OFFBEAT_VelocityRandom = 0,
    OFFBEAT_VelocityCone = 1,

    OFFBEAT_VelocityCount,
};

struct ob_emission
{
    ob_expr Location;
    ob_expr EmissionRate;
    ob_expr ParticleLifetime;

    ob_emission_shape Shape;
    ob_expr EmissionRadius;
    ob_expr EmissionNormal;
    om3 EmissionRotation;

    ob_expr InitialVelocityScale;

    ob_emission_velocity VelocityType;
    ob_expr ConeDirection;
    ob_expr ConeHeight;
    ob_expr ConeRadius;
    om3 ConeRotation;
};

enum ob_motion_primitive
{
    OFFBEAT_MotionNone = 0,
    OFFBEAT_MotionPoint = 1,
    OFFBEAT_MotionLine = 2,
    OFFBEAT_MotionSphere = 3,

    OFFBEAT_MotionCount,
};

struct ob_motion
{
    ob_expr Gravity;
    ob_expr Drag;
    ob_expr Strength;
    b32 Collision;

    ob_motion_primitive Primitive;
    ob_expr Position;
    ob_expr LineDirection;
    ob_expr SphereRadius;
};

struct ob_appearance
{
    ob_expr Color;
    ob_expr Size;
    // ov3 Scale;
    u32 TextureIndex;
};

struct ob_history_entry
{
    f32 MaxLifetime;
    u32 ParticlesEmitted;
};

struct ob_particle_system
{
    f32 t;
    f32 tSpawn;

    b32 UseGPU;

    ob_emission Emission;
    ob_motion Motion;
    ob_appearance Appearance;

    u32 ParticleSpawnCount;
    u32 ParticleCount;
    ob_particle* Particles;

    u32 MaxParticleCount;
    u32 HistoryEntryCount;
    ob_history_entry* History;

#ifdef OFFBEAT_OPENGL_COMPUTE
    GLuint ParticleSSBO;
    GLuint OldParticleSSBO;
#endif
};

struct ob_packed_particle_system
{
    u32 Header[4];
    b32 UseGPU;
    ob_emission Emission;
    ob_motion Motion;
    ob_appearance Appearance;
};

// NOTE(rytis): Aligned vvv
struct ob_global_data_aligned
{
    f32 t; f32 dt; u32 RunningParticleID; u32 ParticleSpawnCount;
    u32 OldParticleCount; u32 RandomTableIndex; ov2 P0;
    ov3 Horizontal; f32 P1;
    ov3 Vertical; f32 P2;
    ov3 CameraPosition; f32 P3;
    u64 DepthMap; u64 NormalMap;
    om4 View;
    om4 Projection;
    om4 InverseView;
    om4 InverseProjection;
};

struct ob_emission_uniform_aligned
{
    ob_expr Location;
    ob_expr EmissionRate;
    ob_expr ParticleLifetime;

    ob_emission_shape Shape; ov3 P0;
    ob_expr EmissionRadius;
    ob_expr EmissionNormal;
    om3x4 EmissionRotation;

    ob_expr InitialVelocityScale;
    ob_emission_velocity VelocityType; ov3 P1;
    ob_expr ConeHeight;
    ob_expr ConeRadius;
    ob_expr ConeDirection;
    om3x4 ConeRotation;
};

struct ob_motion_uniform_aligned
{
    ob_expr Gravity;
    ob_expr Drag;
    ob_expr Strength;
    b32 Collision;

    ob_motion_primitive Primitive; ov2 P0;
    ob_expr Position;
    ob_expr LineDirection;
    ob_expr SphereRadius;
};

struct ob_appearance_uniform_aligned
{
    ob_expr Color;
    ob_expr Size;
    // ov3 Scale;
    u32 TextureIndex;
};

struct ob_draw_vertex_aligned
{
    ov3 Position; u32 TextureIndex;
    ov2 UV; ov2 P0;
    ov4 Color;
};
// NOTE(rytis): Aligned ^^^

struct ob_quad_data
{
    ov3 Horizontal;
    ov3 Vertical;
};

struct ob_draw_vertex
{
    ov3 Position; u32 TextureIndex;
    ov2 UV;
    ov4 Color;
};

struct ob_draw_list
{
    b32 UseGPU;
    u32 ElementCount;

    u32 IndexCount;
    u32* Indices;

    u32 VertexCount;
    ob_draw_vertex* Vertices;

#ifdef OFFBEAT_OPENGL_COMPUTE
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
#endif
};

struct ob_draw_data
{
    u32 DrawListCount;
    ob_draw_list DrawLists[OFFBEAT_DRAW_LIST_COUNT];
};

struct ob_draw_list_debug
{
    ob_texture TextureID;
    u32 ElementCount;

    u32 IndexCount;
    u32* Indices;

    u32 VertexCount;
    ob_draw_vertex* Vertices;
};

struct ob_draw_data_debug
{
    u32 DrawListCount;
    ob_draw_list_debug DrawLists[OFFBEAT_DRAW_LIST_COUNT];
};

struct ob_memory_manager
{
    u8* CurrentMaxAddress;
    u8* LastMaxAddress;

    u8* CurrentAddress;

    u8* CurrentBuffer;
    u8* LastBuffer;
};

struct ob_state
{
    f32 dt;
    f32 t;

    u32 RunningParticleID;

    ob_program SpawnProgramID;
    ob_program UpdateProgramID;
    ob_program StatelessEvaluationProgramID;
    ob_program RenderProgramID;

    u64 TotalParticleCount;

    ob_quad_data QuadData;
    ov3 CameraPosition;
    om4 ViewMatrix;
    om4 ProjectionMatrix;
    u64 DepthMapHandle;
    u64 NormalMapHandle;
    b32 GeometryTexturesLoaded;

    ob_texture GridTextureID;
    u32 GridIndices[12];
    ob_draw_vertex GridVertices[4];

    b32 TexturesLoaded;
    u32 AdditionalTextureCount; // NOTE(rytis): 2 additional slots for grid and debug textures.
    u32 TextureCount;
    ob_texture Textures[OFFBEAT_PARTICLE_SYSTEM_COUNT + 2];
    u64 TextureHandles[OFFBEAT_PARTICLE_SYSTEM_COUNT + 2];
#ifdef OFFBEAT_DEBUG
    b32 DrawDebugData;
    u32 DebugCurrentParticleSystem;
    ob_draw_data_debug DebugDrawData;
#endif

    ob_random_series EffectsEntropy;

    u32 CurrentParticleSystem;
    u32 ParticleSystemCount;
    ob_particle_system ParticleSystems[OFFBEAT_PARTICLE_SYSTEM_COUNT];

    ob_draw_data DrawData;

    ob_memory_manager MemoryManager;
};

// NOTE(rytis): Optional callbacks.
OFFBEAT_API void OffbeatSetAllocatorFunctions(void* (*Malloc)(u64), void (*Free)(void*));
OFFBEAT_API void OffbeatSetTextureFunction(ob_texture (*TextureFunction)(void*, u32, u32));

// NOTE(rytis): Init.
OFFBEAT_API ob_state* OffbeatSetupMemory(void* Memory, u64 MemorySize);
OFFBEAT_API void OffbeatInitGeometryTextures(ob_texture ViewSpaceDepthMap, ob_texture ViewSpaceNormalMap);
OFFBEAT_API void OffbeatAddTexture(ob_texture Texture);
OFFBEAT_API void OffbeatGenerateTextureArray();
OFFBEAT_API void OffbeatInit();

// NOTE(rytis): Particle system manipulation.
OFFBEAT_API ob_particle_system* OffbeatNewParticleSystem(u32* Index = 0);
OFFBEAT_API void OffbeatRemoveParticleSystem(u32 Index);
OFFBEAT_API void OffbeatAddParticleSystem(ob_particle_system* NewParticleSystem);
OFFBEAT_API void OffbeatRemoveCurrentParticleSystem();
OFFBEAT_API void OffbeatRemoveAllParticleSystems();
OFFBEAT_API ob_particle_system* OffbeatGetCurrentParticleSystem();
OFFBEAT_API ob_particle_system* OffbeatPreviousParticleSystem();
OFFBEAT_API ob_particle_system* OffbeatNextParticleSystem();
OFFBEAT_API void OffbeatToggleGPU(ob_particle_system* ParticleSystem);

// NOTE(rytis): Calculation.
OFFBEAT_API void OffbeatUpdateCamera(f32 Position[3], f32 Forward[3], f32 Right[3]);
OFFBEAT_API void OffbeatUpdateViewMatrix(f32 RowMajorMatrix[16]);
OFFBEAT_API void OffbeatUpdateProjectionMatrix(f32 RowMajorMatrix[16]);
OFFBEAT_API void OffbeatUpdateParticles(f32 dt);

// NOTE(rytis): Render data.
OFFBEAT_API ob_draw_data* OffbeatGetDrawData();
OFFBEAT_API ob_draw_data_debug* OffbeatGetDebugDrawData();
OFFBEAT_API void OffbeatRenderParticles();

// NOTE(rytis): Particle system pack/unpack as bytes.
OFFBEAT_API ob_file_data OffbeatPackParticleSystemBytes(u32 ParticleSystemIndex);
OFFBEAT_API ob_file_data OffbeatPackCurrentParticleSystemBytes();
OFFBEAT_API b32 OffbeatUnpackParticleSystem(ob_particle_system* Result, void* PackedBytes);

// NOTE(rytis): Particle system pack/unpack as struct.
OFFBEAT_API ob_packed_particle_system OffbeatPackParticleSystemStruct(u32 ParticleSystemIndex);
OFFBEAT_API ob_packed_particle_system OffbeatPackCurrentParticleSystemStruct();
OFFBEAT_API b32 OffbeatUnpackParticleSystem(ob_particle_system* Result, ob_packed_particle_system* PackedStruct);
