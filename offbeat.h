#pragma once

#include "common.h"

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

typedef float r32;
typedef double r64;
typedef float f32;
typedef double f64;

typedef vec2 v2;
typedef vec3 v3;
typedef vec4 v4;
typedef mat3 m3;
typedef mat4 m4;

#define OffbeatAssert(Expression) if(!(Expression)) {*(int *)0 = 0;}

#define OffbeatKibibytes(Value) (1024LL * (Value))
#define OffbeatMibibytes(Value) (1024LL * OffbeatKibibytes(Value))
#define OffbeatGibibytes(Value) (1024LL * OffbeatMibibytes(Value))

#define OffbeatArrayCount(Array) sizeof((Array)) / sizeof(Array[0])

#include "offbeat_math.h"
#include "offbeat_random.h"

#define OFFBEAT_MAX_SQUARE_GRID_LINE_COUNT 50

#define OFFBEAT_MAX_PARTICLE_COUNT 512
#define OFFBEAT_MAX_DRAW_COMMAND_COUNT OFFBEAT_MAX_PARTICLE_COUNT

#define OFFBEAT_MAX_INDEX_COUNT 6000
#define OFFBEAT_MAX_VERTEX_COUNT 4000

#define OFFBEAT_MAX_SYSTEM_COUNT 10
#define OFFBEAT_MAX_DRAW_LIST_COUNT OFFBEAT_MAX_SYSTEM_COUNT

struct ob_grid_line
{
    ov3 A;
    ov3 B;
};

struct ob_particle
{
    ov3 P;
    ov3 dP;
    ov3 ddP;
    ov4 Color;
    ov4 dColor;
    f32 Age;
    // TODO(rytis): Change the uniform size to be per-particle and the
    // non-uniform scale per-particle-system.
    f32 Size;
};

enum ob_emission_shape
{
    OFFBEAT_EmissionPoint,
    OFFBEAT_EmissionRing,

    OFFBEAT_EmissionCount,
};

enum ob_emission_velocity_type
{
    OFFBEAT_VelocityCone,
    OFFBEAT_VelocityRandom,

    OFFBEAT_VelocityCount,
};

struct ob_emission
{
    ov3 Location;
    f32 EmissionRate;
    f32 ParticleLifetime;
    ob_emission_shape Shape;
    union
    {
        struct
        {
        } Point;

        struct
        {
            f32 Radius;
        } Ring;
    };
    f32 InitialVelocityScale;
    ob_emission_velocity_type VelocityType;
    union
    {
        struct
        {
            ov3 Direction;
            f32 Height;
            f32 Radius;
            om3 Rotation;
        } Cone;

        struct
        {
        } Random;
    };
};

enum ob_motion_primitive
{
    OFFBEAT_MotionNone,
    OFFBEAT_MotionPoint,

    OFFBEAT_MotionCount,
};

struct ob_motion
{
    ov3 Gravity;
    ob_motion_primitive Primitive;
    union
    {
        struct
        {
            ov3 Position;
            f32 Strength;
        } Point;
    };
};

struct ob_appearance
{
    ov4 Color;
    f32 Size;
    ov3 Scale;
    u32 TextureID;
};

struct ob_particle_system
{
    f32 t; // NOTE(rytis): Time of individual particle system to properly spawn particles.

    ob_emission Emission;
    ob_motion Motion;
    ob_appearance Appearance;

    u32 NextParticle;
    ob_particle Particles[OFFBEAT_MAX_PARTICLE_COUNT];
};

struct ob_camera
{
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

struct ob_draw_list
{
    u32 TextureID;
    u32 ElementCount;

    u32 IndexCount;
    u32 Indices[OFFBEAT_MAX_INDEX_COUNT];

    u32 VertexCount;
    ob_draw_vertex Vertices[OFFBEAT_MAX_VERTEX_COUNT];
};

struct ob_draw_data
{
    u32 DrawListCount;
    ob_draw_list DrawLists[OFFBEAT_MAX_DRAW_LIST_COUNT];
};

struct ob_state
{
    f32 dt;
    f32 t;

    ob_quad_data QuadData;

    ov4 SquareGridColor;
    u32 SquareGridLineCount;
    f32 SquareGridStep;
    f32 SquareGridRange;
    ob_grid_line SquareGridLines[OFFBEAT_MAX_SQUARE_GRID_LINE_COUNT];

    b32 UpdateParticles;
    ob_random_series EffectsEntropy;

    u32 ParticleSystemCount;
    ob_particle_system ParticleSystems[OFFBEAT_MAX_SYSTEM_COUNT];

    ob_draw_data DrawData;
};

void OffbeatSetAllocatorFunctions(void* (*Malloc)(u64), void (*Free)(void*));
ob_state* OffbeatInit(void* Memory, u64 MemorySize);
void OffbeatParticleSystem(ob_state* OffbeatState, game_input* Input, ob_camera Camera);
ob_draw_data* OffbeatGetDrawData(ob_state* OffbeatState);
