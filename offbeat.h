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
 *
 * Change rotation calculations so that rotation matrix would be calculated either every frame
 * or on every direction vector change.
 * UPDATE: For now we do it every frame before any other calculations for the particle system.
 * Maybe could thing of something better?
 *
 * Finalize texture list. Provide means to add custom textures. If I want to do that, though,
 * a global texture ID table might not be the proper solution.
 *
 * IN PROGRESS: Expand the GUI to allow customization of particle emission, motion or appearance.
 *
 * Figure out a way to properly implement expressions, including the ability to use some
 * per-particle values (age, distance to camera, etc.). Add distance to camera.
 * UPDATE: For now, expression evaluation seems to be decent enough, although it's not very flexible.
 *
 * Add memory alignment (16 byte, preferably changeable with macro or function parameter)
 * in OffbeatInit and memory manager.
 *
 * Add more emission, initial velocity and motion primitive shapes.
 *
 * Add selectors (plane, sphere, cube ???). Allow testing selectors against a ray.
 *
 * Add option to enable / disable GPU dispatch for selected particle system in runtime.
 */

//#define OffbeatAssert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#include <assert.h>
#define OffbeatAssert(Expression) assert(Expression)

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
    void* Data;
    u64 Size;
};

enum ob_texture_type
{
    OFFBEAT_TextureSquare,
    OFFBEAT_TextureCircle,
    OFFBEAT_TextureFatCross,
    OFFBEAT_TextureSlimCross,
    // OFFBEAT_TextureRing,

    OFFBEAT_TextureCount,
};

struct ob_particle
{
    ov3 P;
    f32 Age;
    ov3 dP;
    f32 ID;
    f32 Random;
    f32 CameraDistance;
    f32 Padding[2];
};

enum ob_function
{
    OFFBEAT_FunctionConst = 0,
    OFFBEAT_FunctionLerp = 1,
    OFFBEAT_FunctionSmoothstep = 2,
    OFFBEAT_FunctionSquared = 3,
    OFFBEAT_FunctionCubed = 4,
    OFFBEAT_FunctionFourth = 5,
    OFFBEAT_FunctionTriangle = 6,
    OFFBEAT_FunctionTwoTriangles = 7,
    OFFBEAT_FunctionFourTriangles = 8,
    OFFBEAT_FunctionPeriodic = 9,

    OFFBEAT_FunctionCount,
};

enum ob_parameter
{
    OFFBEAT_ParameterAge = 0,
    OFFBEAT_ParameterRandom = 1,
    OFFBEAT_ParameterCameraDistance = 2,
    OFFBEAT_ParameterID = 3,

    OFFBEAT_ParameterCount,
};

struct ob_expr
{
    ob_function Function;
    ob_parameter Parameter;
    f32 Frequency;
    f32 Padding;
    ov4 Low;
    ov4 High;
};

enum ob_emission_shape
{
    OFFBEAT_EmissionPoint = 0,
    OFFBEAT_EmissionRing = 1,

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
    ov3 Location;
    f32 EmissionRate;
    ob_expr ParticleLifetime;

    ob_emission_shape Shape;
    f32 RingRadius;
    ov3 RingNormal;
    om3 RingRotation;

    f32 InitialVelocityScale;

    ob_emission_velocity VelocityType;
    ov3 ConeDirection;
    f32 ConeHeight;
    f32 ConeRadius;
    om3 ConeRotation;
};

enum ob_motion_primitive
{
    OFFBEAT_MotionNone = 0,
    OFFBEAT_MotionPoint = 1,
    OFFBEAT_MotionLine = 2,

    OFFBEAT_MotionCount,
};

struct ob_motion
{
    ov3 Gravity;
    f32 Drag;
    ob_expr Strength;

    ob_motion_primitive Primitive;
    ov3 Position;
    ov3 LineDirection;
};

struct ob_appearance
{
    ob_expr Color;
    ob_expr Size;
    // ov3 Scale;
    ob_texture_type Texture;
};

struct ob_history_entry
{
    f32 TimeElapsed;
    u32 ParticlesEmitted;
};

struct ob_particle_system
{
    f32 t;
    f32 tSpawn;

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

// NOTE(rytis): Aligned vvv
struct ob_emission_uniform_aligned
{
    ov3 Location; f32 EmissionRate;
    ob_expr ParticleLifetime;

    ob_emission_shape Shape; f32 RingRadius; ov2 P0;
    ov3 RingNormal; f32 P1;
    om3x4 RingRotation;

    f32 InitialVelocityScale; ob_emission_velocity VelocityType; f32 ConeHeight; f32 ConeRadius;
    ov3 ConeDirection; f32 P2;
    om3x4 ConeRotation;
};

struct ob_motion_uniform_aligned
{
    ov3 Gravity; f32 Drag;
    ob_expr Strength;

    ob_motion_primitive Primitive; ov3 P0;
    ov3 Position; f32 P1;
    ov3 LineDirection; f32 P2;
};

struct ob_appearance_uniform_aligned
{
    ob_expr Color;
    ob_expr Size;
    // ov3 Scale;
};

struct ob_draw_vertex_aligned
{
    ov3 Position; f32 P0;
    ov2 UV; ov2 P1;
    ov4 Color;
};
// NOTE(rytis): Aligned ^^^

struct ob_camera
{
    ov3 Position;
    ov3 Forward;
    ov3 Right;
};

struct ob_quad_data
{
    ov3 Horizontal;
    ov3 Vertical;
};

struct ob_draw_vertex
{
    ov3 Position;
    ov2 UV;
    ov4 Color;
};

#ifdef OFFBEAT_OPENGL_COMPUTE
struct ob_draw_list
{
    ob_texture TextureID;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    u32 IndexCount;
};

struct ob_draw_data
{
    u32 DrawListCount;
    ob_draw_list DrawLists[OFFBEAT_DRAW_LIST_COUNT];
};
#else
struct ob_draw_list
{
    ob_texture TextureID;
    u32 ElementCount;

    u32 IndexCount;
    u32* Indices;

    u32 VertexCount;
    ob_draw_vertex* Vertices;
};

struct ob_draw_data
{
    u32 DrawListCount;
    ob_draw_list DrawLists[OFFBEAT_DRAW_LIST_COUNT];
};
#endif

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
    u64 FrameCount;
    u64 CycleSum;
    u64 ParticleSum;
    f32 MSSum;

    ob_quad_data QuadData;
    ov3 CameraPosition;

    ob_texture GridTextureID;
    u32 GridIndices[12];
    ob_draw_vertex GridVertices[4];

    ob_random_series EffectsEntropy;

    // TODO(rytis): Not sure if CurrentParticleSystem is even needed. For now, it seems to be ok?
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
OFFBEAT_API ob_state* OffbeatInit(void* Memory, u64 MemorySize);
OFFBEAT_API ob_state* OffbeatInit(void* (*Malloc)(u64));
OFFBEAT_API ob_state* OffbeatInit();

// NOTE(rytis): Particle system manipulation.
OFFBEAT_API ob_particle_system* OffbeatNewParticleSystem(ob_state* OffbeatState);
OFFBEAT_API u32 OffbeatNewParticleSystem(ob_state* OffbeatState, ob_particle_system** NewParticleSystem);
OFFBEAT_API void OffbeatRemoveParticleSystem(ob_state* OffbeatState, u32 Index);
OFFBEAT_API void OffbeatAddParticleSystem(ob_state* OffbeatState, ob_particle_system* NewParticleSystem);
OFFBEAT_API void OffbeatRemoveCurrentParticleSystem(ob_state* OffbeatState);
OFFBEAT_API ob_particle_system* OffbeatGetCurrentParticleSystem(ob_state* OffbeatState);
OFFBEAT_API ob_particle_system* OffbeatPreviousParticleSystem(ob_state* OffbeatState);
OFFBEAT_API ob_particle_system* OffbeatNextParticleSystem(ob_state* OffbeatState);
OFFBEAT_API ob_texture_type OffbeatGetCurrentTextureType(ob_particle_system* ParticleSystem);
OFFBEAT_API ob_texture OffbeatGetTextureID(ob_particle_system* ParticleSystem);
OFFBEAT_API void OffbeatSetTextureID(ob_particle_system* ParticleSystem, ob_texture_type NewType);

// NOTE(rytis): Calculation.
OFFBEAT_API void OffbeatUpdate(ob_state* OffbeatState, ob_camera Camera, f32 dt);

// NOTE(rytis): Render data.
#ifndef OFFBEAT_OPENGL_COMPUTE
OFFBEAT_API ob_draw_data* OffbeatGetDrawData(ob_state* OffbeatState);
#endif
OFFBEAT_API ob_draw_data_debug* OffbeatGetDebugDrawData();
OFFBEAT_API void OffbeatRenderParticles(ob_state* OffbeatState, float* ViewMatrix, float* ProjectionMatrix);

// NOTE(rytis): Particle system pack/unpack.
OFFBEAT_API ob_file_data OffbeatPackParticleSystem(ob_state* OffbeatState, u32 ParticleSystemIndex);
OFFBEAT_API b32 OffbeatUnpackParticleSystem(ob_particle_system* Result, void* PackedMemory);
