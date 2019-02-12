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

#include "offbeat_math.h"
#include "offbeat_random.h"

#define MAX_SQUARE_GRID_LINE_COUNT 50
#define PARTICLE_CEL_DIM 16

enum offbeat_emitter_shape
{
    OFFBEAT_EmitterPoint,
    OFFBEAT_EmitterRing,

    OFFBEAT_EmitterCount,
};

struct offbeat_grid_line
{
    v3 A;
    v3 B;
};

struct offbeat_particle_cel
{
    r32 Density;
    v3 VelocityTimesDensity;
};

struct offbeat_particle
{
    v3 P;
    v3 dP;
    v3 ddP;
    v4 Color;
    v4 dColor;
};

struct offbeat_emitter
{
    v3 Location;
    offbeat_emitter_shape Shape;
    float Radius;
};

struct offbeat_state
{
    r32 t; // NOTE(rytis): Accumulated time.

    v4 SquareGridColor;
    u32 SquareGridLineCount;
    r32 SquareGridStep;
    r32 SquareGridRange;
    offbeat_grid_line SquareGridLines[MAX_SQUARE_GRID_LINE_COUNT];

    b32 UpdateParticles;
    random_series EffectsEntropy;

    u32 NextParticle;
    offbeat_particle Particles[256];

    v3 ParticleCelGridOrigin;
    offbeat_particle_cel ParticleCels[PARTICLE_CEL_DIM][PARTICLE_CEL_DIM][PARTICLE_CEL_DIM];

    b32 DrawParticleCelGrid;
    r32 ParticleCelGridScale;
    v4 ParticleCelGridColor;
    u32 ParticleCelGridLineCount;
    offbeat_grid_line ParticleCelGridLines[3 * (PARTICLE_CEL_DIM + 1) * (PARTICLE_CEL_DIM + 1)];
};

offbeat_state* OffbeatInit(void* Memory, u64 MemorySize);
void OffbeatParticleSystem(offbeat_state* OffbeatState, game_input* Input);
