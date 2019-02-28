#pragma once

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

/* TODO(rytis):
 *
 * Change rotation calculations so that rotation matrix would be calculated either every frame
 * or on every direction vector change.
 * UPDATE: For now we do it every frame before any other calculations for the particle system.
 * Maybe could thing of something better?
 */

#ifndef OFFBEAT_API
#define OFFBEAT_API
#endif

#define OffbeatAssert(Expression) if(!(Expression)) {*(int *)0 = 0;}

#define OffbeatKibibytes(Value) (1024LL * (Value))
#define OffbeatMibibytes(Value) (1024LL * OffbeatKibibytes(Value))
#define OffbeatGibibytes(Value) (1024LL * OffbeatMibibytes(Value))

#define OffbeatArrayCount(Array) sizeof((Array)) / sizeof(Array[0])

#include "offbeat_math.h"
#include "offbeat_random.h"

#define OFFBEAT_MAX_SQUARE_GRID_LINE_COUNT 50

#define OFFBEAT_HISTORY_ENTRY_COUNT 1000

#define OFFBEAT_PARTICLE_SYSTEM_COUNT 512
#define OFFBEAT_DRAW_LIST_COUNT OFFBEAT_PARTICLE_SYSTEM_COUNT

struct ob_grid_line
{
    ov3 A;
    ov3 B;
};

struct ob_particle
{
    ov3 P;
    ov3 dP;
    f32 Age;
};

enum ob_emission_shape_type
{
    OFFBEAT_EmissionPoint,
    OFFBEAT_EmissionRing,

    OFFBEAT_EmissionCount,
};

struct ob_emission_shape
{
    ob_emission_shape_type Type;

    union
    {
        struct
        {
        } Point;

        struct
        {
            f32 Radius;
            ov3 Normal;
            om3 Rotation;
        } Ring;
    };
};

enum ob_emission_velocity_type
{
    OFFBEAT_VelocityCone,
    OFFBEAT_VelocityRandom,

    OFFBEAT_VelocityCount,
};

struct ob_emission_velocity
{
    ob_emission_velocity_type Type;
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

struct ob_emission
{
    ov3 Location;
    f32 EmissionRate;
    f32 ParticleLifetime;
    ob_emission_shape Shape;

    f32 InitialVelocityScale;
    ob_emission_velocity Velocity;
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
    // ov3 Scale;
    u32 TextureID;
};

struct ob_history_entry
{
    f32 TimeElapsed;
    u32 ParticlesEmitted;
};

struct ob_particle_system
{
    f32 t; // NOTE(rytis): Time of individual particle system to properly spawn particles.

    ob_emission Emission;
    ob_motion Motion;
    ob_appearance Appearance;

    u32 ParticleCount;
    ob_particle* Particles;

    u32 HistoryStartIndex;
    u32 HistoryEndIndex;
    ob_history_entry History[OFFBEAT_HISTORY_ENTRY_COUNT];
};

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

struct ob_draw_list
{
    u32 TextureID;
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

    ob_quad_data QuadData;

    ov4 SquareGridColor;
    u32 SquareGridLineCount;
    f32 SquareGridStep;
    f32 SquareGridRange;
    ob_grid_line SquareGridLines[OFFBEAT_MAX_SQUARE_GRID_LINE_COUNT];

    b32 UpdateParticles;
    ob_random_series EffectsEntropy;

    u32 CurrentParticleSystem;
    u32 ParticleSystemCount;
    ob_particle_system ParticleSystems[OFFBEAT_PARTICLE_SYSTEM_COUNT];

    ob_draw_data DrawData;

    ob_memory_manager MemoryManager;
};

// NOTE(rytis): Init
OFFBEAT_API void OffbeatSetAllocatorFunctions(void* (*Malloc)(u64), void (*Free)(void*));
OFFBEAT_API ob_state* OffbeatInit(void* Memory, u64 MemorySize);
OFFBEAT_API ob_state* OffbeatInit(void* (*Malloc)(u64));
OFFBEAT_API ob_state* OffbeatInit();

// NOTE(rytis): Particle system manipulation
OFFBEAT_API ob_particle_system* OffbeatAddParticleSystem(ob_state* OffbeatState);

// NOTE(rytis): Calculation
OFFBEAT_API void OffbeatUpdate(ob_state* OffbeatState, ob_camera Camera, f32 dt);

// NOTE(rytis): Render data
OFFBEAT_API ob_draw_data* OffbeatGetDrawData(ob_state* OffbeatState);
