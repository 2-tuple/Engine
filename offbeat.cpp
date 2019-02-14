#include "debug_drawing.h"

#include "offbeat.h"

#ifndef OFFBEAT_USE_DEFAULT_ALLOCATOR
static void* _Malloc(u64 Size) { OffbeatAssert(!"No custom allocator set!"); return 0; }
static void _Free(void* Pointer) { OffbeatAssert(!"No custom allocator set!"); }
#else
#include <stdlib.h>
static void* _Malloc(u64 Size) { return malloc(Size); }
static void _Free(void* Pointer) { free(Pointer); }
#endif

static void* (*GlobalOffbeatAlloc)(u64) = _Malloc;
static void (*GlobalOffbeatFree)(void*) = _Free;

void
OffbeatSetAllocatorFunctions(void* (*Malloc)(u64), void (*Free)(void*))
{
    GlobalOffbeatAlloc = Malloc;
    GlobalOffbeatFree = Free;
}

#if 1
offbeat_state*
OffbeatInit(void* Memory, u64 MemorySize)
{
    OffbeatAssert(MemorySize >= sizeof(offbeat_state));
    offbeat_state* OffbeatState = (offbeat_state*)Memory;
    {
        OffbeatState->t = 0.0f;
        OffbeatState->tSpawn = 0.0f;

        OffbeatState->EffectsEntropy = RandomSeed(1234);
        OffbeatState->UpdateParticles = true;

        OffbeatState->SquareGridColor = v4{0.5f, 0.5f, 0.85f, 1.0f};
        OffbeatState->SquareGridLineCount = 0;
        OffbeatState->SquareGridStep = 2.0f;
        OffbeatState->SquareGridRange = 10.0f;
        r32 Limit = OffbeatState->SquareGridRange + 0.5f * OffbeatState->SquareGridStep;
        for(r32 Coordinate = -OffbeatState->SquareGridRange;
          Coordinate < Limit;
          Coordinate += OffbeatState->SquareGridStep)
        {
          OffbeatState->SquareGridLines[OffbeatState->SquareGridLineCount++] = offbeat_grid_line{
            v3{Coordinate, 0.0f, -OffbeatState->SquareGridRange},
            v3{Coordinate, 0.0f, OffbeatState->SquareGridRange}
          };
          OffbeatState->SquareGridLines[OffbeatState->SquareGridLineCount++] = offbeat_grid_line{
            v3{-OffbeatState->SquareGridRange, 0.0f, Coordinate},
            v3{OffbeatState->SquareGridRange, 0.0f, Coordinate}
          };
        }
        OffbeatAssert(OffbeatState->SquareGridLineCount < MAX_SQUARE_GRID_LINE_COUNT);

        offbeat_emission TestEmission = {};
        TestEmission.Location = v3{0.0f, 0.0f, 0.0f};
        TestEmission.EmissionRate = 60.0f;
        TestEmission.ParticleLifetime = 1.5f;
        TestEmission.Shape = OFFBEAT_EmissionRing;
        TestEmission.Ring.Radius = 1.0f;

        offbeat_particle_system TestSystem = {};
        TestSystem.Emission = TestEmission;
        TestSystem.NextParticle = 0;

        OffbeatState->ParticleSystemCount = 1;
        OffbeatState->ParticleSystems[0] = TestSystem;
    }
    return OffbeatState;
}
#else
offbeat_state*
OffbeatInit(void* Memory, u64 MemorySize)
{
    OffbeatAssert(MemorySize >= sizeof(offbeat_state));
    offbeat_state* OffbeatState = (offbeat_state*)Memory;
    {
        OffbeatState->t = 0.0f;

        OffbeatState->EffectsEntropy = RandomSeed(1234);
        OffbeatState->UpdateParticles = true;
        OffbeatState->DrawParticleCelGrid = false;

        OffbeatState->SquareGridColor = v4{0.5f, 0.5f, 0.85f, 1.0f};
        OffbeatState->SquareGridLineCount = 0;
        OffbeatState->SquareGridStep = 2.0f;
        OffbeatState->SquareGridRange = 10.0f;
        r32 Limit = OffbeatState->SquareGridRange + 0.5f * OffbeatState->SquareGridStep;
        for(r32 Coordinate = -OffbeatState->SquareGridRange;
          Coordinate < Limit;
          Coordinate += OffbeatState->SquareGridStep)
        {
          OffbeatState->SquareGridLines[OffbeatState->SquareGridLineCount++] = offbeat_grid_line{
            v3{Coordinate, 0.0f, -OffbeatState->SquareGridRange},
            v3{Coordinate, 0.0f, OffbeatState->SquareGridRange}
          };
          OffbeatState->SquareGridLines[OffbeatState->SquareGridLineCount++] = offbeat_grid_line{
            v3{-OffbeatState->SquareGridRange, 0.0f, Coordinate},
            v3{OffbeatState->SquareGridRange, 0.0f, Coordinate}
          };
        }
        OffbeatAssert(OffbeatState->SquareGridLineCount < MAX_SQUARE_GRID_LINE_COUNT);

        OffbeatState->ParticleCelGridScale = 0.25f;
        OffbeatState->ParticleCelGridOrigin =
          v3{-0.5f * PARTICLE_CEL_DIM * OffbeatState->ParticleCelGridScale,
               0.0f,
               -0.5f * PARTICLE_CEL_DIM * OffbeatState->ParticleCelGridScale};
        OffbeatState->ParticleCelGridColor = v4{0.85f, 0.3f, 0.3f, 0.1f};
        OffbeatState->ParticleCelGridLineCount = 0;
        v3 GridOrigin = OffbeatState->ParticleCelGridOrigin;
        for(u32 i = 0; i < PARTICLE_CEL_DIM + 1; ++i)
        {
          r32 Plane = i * OffbeatState->ParticleCelGridScale;
          for(u32 j = 0; j < PARTICLE_CEL_DIM + 1; ++j)
          {
              r32 PlaneLine = j * OffbeatState->ParticleCelGridScale;
              r32 LineLength = PARTICLE_CEL_DIM * OffbeatState->ParticleCelGridScale;
              // X
              OffbeatState->ParticleCelGridLines[OffbeatState->ParticleCelGridLineCount++] = offbeat_grid_line{
                  v3{GridOrigin.X, GridOrigin.Y + Plane, GridOrigin.Z + PlaneLine},
                  v3{GridOrigin.X + LineLength, GridOrigin.Y + Plane, GridOrigin.Z + PlaneLine}
              };
              // Z
              OffbeatState->ParticleCelGridLines[OffbeatState->ParticleCelGridLineCount++] = offbeat_grid_line{
                  v3{GridOrigin.X + PlaneLine, GridOrigin.Y + Plane, GridOrigin.Z},
                  v3{GridOrigin.X + PlaneLine, GridOrigin.Y + Plane, GridOrigin.Z + LineLength}
              };
              // Y
              OffbeatState->ParticleCelGridLines[OffbeatState->ParticleCelGridLineCount++] = offbeat_grid_line{
                  v3{GridOrigin.X + Plane, GridOrigin.Y, GridOrigin.Z + PlaneLine},
                  v3{GridOrigin.X + Plane, GridOrigin.Y + LineLength, GridOrigin.Z + PlaneLine}
              };
          }
        }
    }
    return OffbeatState;
}
#endif

static void
OffbeatHandleInput(offbeat_state* OffbeatState, game_input* Input)
{
    if(OffbeatState->UpdateParticles)
    {
        OffbeatState->dt = Input->dt;
        OffbeatState->t += OffbeatState->dt;
        OffbeatState->tSpawn += OffbeatState->dt;
    }

    if(Input->q.EndedDown && Input->q.Changed)
    {
        OffbeatState->UpdateParticles = !OffbeatState->UpdateParticles;
    }

    if(Input->e.EndedDown && Input->e.Changed)
    {
        OffbeatState->DrawParticleCelGrid = !OffbeatState->DrawParticleCelGrid;
    }
}

static void
OffbeatDrawGrid(offbeat_state* OffbeatState)
{
    for(u32 i = 0; i < OffbeatState->SquareGridLineCount; ++i)
    {
        Debug::PushLine(OffbeatState->SquareGridLines[i].A,
                        OffbeatState->SquareGridLines[i].B,
                        OffbeatState->SquareGridColor);
    }
}

static void
ZeroMemory(void* Pointer, u64 Size)
{
    u8* Byte = (u8*)Pointer;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

inline s32
TruncateR32ToS32(r32 R32)
{
    return (s32)R32;
}

inline u32
TruncateR32ToU32(r32 R32)
{
    return (u32)R32;
}

static void
OffbeatDrawParticleCels(offbeat_state* OffbeatState)
{
    for(u32 i = 0; i < OffbeatState->ParticleCelGridLineCount; ++i)
    {
        Debug::PushLine(OffbeatState->ParticleCelGridLines[i].A,
                        OffbeatState->ParticleCelGridLines[i].B,
                        OffbeatState->ParticleCelGridColor);
    }
}

static v3
OffbeatParticlePosition(random_series* EffectsEntropy, offbeat_emission* Emission)
{
    v3 Result = {};

    switch(Emission->Shape)
    {
        case OFFBEAT_EmissionPoint:
        {
            Result = Emission->Location;
        } break;

        case OFFBEAT_EmissionRing:
        {
            r32 RandomValue = 2.0f * PI * RandomUnilateral(EffectsEntropy);
            Result = Emission->Location;
            Result.X += Emission->Ring.Radius * sinf(RandomValue);
            Result.Z += Emission->Ring.Radius * cosf(RandomValue);
        } break;

        default:
        {
        } break;
    }

    return Result;
}

static void
OffbeatSpawnParticles(offbeat_particle_system* ParticleSystem, random_series* Entropy, r32* tSpawn)
{
    offbeat_emission* Emission = &ParticleSystem->Emission;

    r32 TimePerParticle = 1.0f / Emission->EmissionRate;
    r32 RealParticleSpawn =  (*tSpawn) * Emission->EmissionRate;
    OffbeatAssert(RealParticleSpawn >= 0.0f);
    u32 ParticleSpawnCount = TruncateR32ToU32(RealParticleSpawn);
    *tSpawn -= ParticleSpawnCount * TimePerParticle;
    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < ParticleSpawnCount; ++ParticleSpawnIndex)
    {
        offbeat_particle* Particle = ParticleSystem->Particles + ParticleSystem->NextParticle++;
        if(ParticleSystem->NextParticle >= ArrayCount(ParticleSystem->Particles))
        {
            ParticleSystem->NextParticle = 0;
        }

        Particle->P = OffbeatParticlePosition(Entropy, Emission);
        Debug::PushWireframeSphere(Particle->P, 0.02f, v4{0.8f, 0.0f, 0.0f, 1.0f});
        Particle->dP = v3{RandomBetween(Entropy, -0.01f, 0.01f),
                          7.0f * RandomBetween(Entropy, 0.7f, 1.0f),
                          RandomBetween(Entropy, -0.01f, 0.01f)};
        Particle->ddP = v3{0.0f, -9.8f, 0.0f};
        Particle->Color = v4{RandomBetween(Entropy, 0.45f, 0.5f),
                             RandomBetween(Entropy, 0.5f, 0.8f),
                             RandomBetween(Entropy, 0.8f, 1.0f),
                             1.0f};
        Particle->dColor = v4{0.0f, 0.0f, 0.0f, -0.2f};
        Particle->Age = 0.0f;
    }
}

/*
 * NOTE(rytis): OK, so, I did this particle simulation the way Casey Muratori did on
 * the Handmade Hero, but (at least so far) he focused more on the physics side of
 * the simulation (meaning, the created particle "fountain" is using more accurate
 * physics calculations, thus being a more realistic simulation).
 * I don't know if the density calculations are needed for this project. Destiny
 * particle system, which is a big inspiration for this project, uses attractors and
 * repulsors (which, I believe, are based on Newtonian gravity calculations) and
 * I don't know if I need anything fancier than that.
 */
void
OffbeatParticleSystem(offbeat_state* OffbeatState, game_input* Input)
{
    OffbeatHandleInput(OffbeatState, Input);
    OffbeatDrawGrid(OffbeatState);

    glEnable(GL_BLEND);

    v3 GridOrigin = OffbeatState->ParticleCelGridOrigin;
    r32 InvGridScale = 1.0f / OffbeatState->ParticleCelGridScale;

    if(OffbeatState->UpdateParticles)
    {
        for(u32 i = 0; i < OffbeatState->ParticleSystemCount; ++i)
        {
            OffbeatSpawnParticles(&OffbeatState->ParticleSystems[i], &OffbeatState->EffectsEntropy, &OffbeatState->tSpawn);
        }

        ZeroMemory(OffbeatState->ParticleCels, PARTICLE_CEL_DIM * PARTICLE_CEL_DIM * PARTICLE_CEL_DIM * sizeof(offbeat_particle_cel));

        for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(OffbeatState->ParticleSystems[0].Particles); ++ParticleIndex)
        {
            offbeat_particle* Particle = OffbeatState->ParticleSystems[0].Particles + ParticleIndex;

            v3 P = InvGridScale * (Particle->P - GridOrigin);

            s32 X = TruncateR32ToS32(P.X);
            s32 Y = TruncateR32ToS32(P.Y);
            s32 Z = TruncateR32ToS32(P.Z);

            if(X < 0) {X = 0;}
            if(X > (PARTICLE_CEL_DIM - 1)) {X = (PARTICLE_CEL_DIM - 1);}
            if(Y < 0) {Y = 0;}
            if(Y > (PARTICLE_CEL_DIM - 1)) {Y = (PARTICLE_CEL_DIM - 1);}
            if(Z < 0) {Z = 0;}
            if(Z > (PARTICLE_CEL_DIM - 1)) {Z = (PARTICLE_CEL_DIM - 1);}

            offbeat_particle_cel* Cel = &OffbeatState->ParticleCels[Z][Y][X];
            r32 Density = Particle->Color.A;
            Cel->Density += Density;
            Cel->VelocityTimesDensity += Density * Particle->dP;
        }
    }

    if(OffbeatState->DrawParticleCelGrid)
    {
        OffbeatDrawParticleCels(OffbeatState);
    }

    v3 Attractor = v3{-3.0f * sinf(0.25f * PI * OffbeatState->t), 1.0f, 1.0f};
    v4 AttractorColor = v4{1.0f, 0.0f, 0.0f, 1.0f};

    Debug::PushWireframeSphere(Attractor, 0.05f, AttractorColor);

    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(OffbeatState->ParticleSystems[0].Particles); ++ParticleIndex)
    {
        offbeat_particle* Particle = OffbeatState->ParticleSystems[0].Particles + ParticleIndex;

        if(Particle->Age >= 1.0f)
        {
            continue;
        }

        if(OffbeatState->UpdateParticles)
        {
            Particle->Age += OffbeatState->dt / OffbeatState->ParticleSystems[0].Emission.ParticleLifetime;

            v3 P = InvGridScale * (Particle->P - GridOrigin);

            s32 X = TruncateR32ToS32(P.X);
            s32 Y = TruncateR32ToS32(P.Y);
            s32 Z = TruncateR32ToS32(P.Z);

            if(X < 1) {X = 0;}
            if(X > (PARTICLE_CEL_DIM - 2)) {X = (PARTICLE_CEL_DIM - 2);}
            if(Y < 1) {Y = 0;}
            if(Y > (PARTICLE_CEL_DIM - 2)) {Y = (PARTICLE_CEL_DIM - 2);}
            if(Z < 1) {Z = 0;}
            if(Z > (PARTICLE_CEL_DIM - 2)) {Z = (PARTICLE_CEL_DIM - 2);}

            offbeat_particle_cel* CelCenter = &OffbeatState->ParticleCels[Z][Y][X];
            offbeat_particle_cel* CelUp = &OffbeatState->ParticleCels[Z][Y + 1][X];
            offbeat_particle_cel* CelDown = &OffbeatState->ParticleCels[Z][Y - 1][X];
            offbeat_particle_cel* CelFront = &OffbeatState->ParticleCels[Z - 1][Y][X];
            offbeat_particle_cel* CelBack = &OffbeatState->ParticleCels[Z + 1][Y][X];
            offbeat_particle_cel* CelLeft = &OffbeatState->ParticleCels[Z][Y][X - 1];
            offbeat_particle_cel* CelRight = &OffbeatState->ParticleCels[Z][Y][X + 1];

            v3 Dispersion = {};
            r32 Dc = 1.0f;
            Dispersion += Dc * (CelCenter->Density - CelUp->Density) * v3{0.0f, 1.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelDown->Density) * v3{0.0f, -1.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelFront->Density) * v3{0.0f, 0.0f, -1.0f};
            Dispersion += Dc * (CelCenter->Density - CelBack->Density) * v3{0.0f, 0.0f, 1.0f};
            Dispersion += Dc * (CelCenter->Density - CelLeft->Density) * v3{-1.0f, 0.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelRight->Density) * v3{1.0f, 0.0f, 0.0f};

            r32 Strength = 10.0f;
            v3 Direction = Attractor - Particle->P;
            r32 Distance = Math::Length(Direction);
            v3 ddPAttraction = Strength * Math::Normalized(Direction) / (Distance * Distance);

            v3 ddP = Particle->ddP + Dispersion + ddPAttraction;

            // NOTE(rytis): Simulate the particle forward in time
            Particle->P += 0.5f * Square(OffbeatState->dt) * ddP + OffbeatState->dt * Particle->dP;
            Particle->dP += OffbeatState->dt * ddP;
            Particle->Color += Particle->dColor * OffbeatState->dt;
            Particle->Color.A = 1.0f - Particle->Age;

            if(Particle->P.Y < 0.0f)
            {
                r32 CoefficientOfRestitution = 0.3f;
                r32 CoefficientOfFriction = 0.7f;
                Particle->P.Y = -Particle->P.Y;
                Particle->dP.Y = -CoefficientOfRestitution * Particle->dP.Y;
                Particle->dP.X = CoefficientOfFriction * Particle->dP.X;
                Particle->dP.Z = CoefficientOfFriction * Particle->dP.Z;
                Particle->Color += v4{-0.1f, 0.0f, 0.0f, 0.0f};
            }
        }

        v4 Color;
        Color.R = Clamp01(Particle->Color.R);
        Color.G = Clamp01(Particle->Color.G);
        Color.B = Clamp01(Particle->Color.B);
        Color.A = Clamp01(Particle->Color.A);

        if(Color.A > 0.9f)
        {
            Color.A = 0.9f * Clamp01MapToRange(1.0f, Color.A, 0.9f);
        }

        // NOTE(rytis): Render the particle (as a debug output for now)
        Debug::PushWireframeSphere(Particle->P, 0.01f, Color);
    }
    glDisable(GL_BLEND);
}
