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

offbeat_state*
OffbeatInit(void* Memory, u64 MemorySize)
{
    OffbeatAssert(MemorySize >= sizeof(offbeat_state));
    offbeat_state* OffbeatState = (offbeat_state*)Memory;
    {
        OffbeatState->t = 0.0f;

        OffbeatState->EffectsEntropy = RandomSeed(1234);
        OffbeatState->UpdateParticles = true;

        OffbeatState->SquareGridColor = v4{0.5f, 0.5f, 0.85f, 1.0f};
        OffbeatState->SquareGridLineCount = 0;
        OffbeatState->SquareGridStep = 2.0f;
        OffbeatState->SquareGridRange = 10.0f;
        f32 Limit = OffbeatState->SquareGridRange + 0.5f * OffbeatState->SquareGridStep;
        for(f32 Coordinate = -OffbeatState->SquareGridRange;
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
        OffbeatAssert(OffbeatState->SquareGridLineCount < OFFBEAT_MAX_SQUARE_GRID_LINE_COUNT);

        offbeat_emission TestEmission = {};
        TestEmission.Location = v3{0.0f, 0.0f, 0.0f};
        TestEmission.EmissionRate = 60.0f;
        TestEmission.ParticleLifetime = 1.5f;
        TestEmission.Shape = OFFBEAT_EmissionRing;
        TestEmission.Ring.Radius = 0.5f;

        TestEmission.InitialVelocityScale = 1.0f;
        TestEmission.VelocityType = OFFBEAT_VelocityCone;
        TestEmission.Cone.Direction = v3{0.0f, 1.0f, 0.0f};
        TestEmission.Cone.Height = 5.0f;
        TestEmission.Cone.EndRadius = 3.0f;

        offbeat_motion TestMotion = {};
        TestMotion.Gravity = v3{0.0f, 0.0f, 0.0f};
        TestMotion.Primitive = OFFBEAT_MotionPoint;
        TestMotion.Point.Position = v3{-3.0f * sinf(0.25f * PI * OffbeatState->t), 1.0f, 1.0f};
        TestMotion.Point.Strength = 10.0f;

        offbeat_appearance TestAppearance = {};
        TestAppearance.Color = v4{RandomBetween(&OffbeatState->EffectsEntropy, 0.45f, 0.5f),
                                  RandomBetween(&OffbeatState->EffectsEntropy, 0.5f, 0.8f),
                                  RandomBetween(&OffbeatState->EffectsEntropy, 0.8f, 1.0f),
                                  1.0f};
        TestAppearance.Size = 0.05f;

        offbeat_particle_system TestSystem = {};
        // TestSystem.t = 0.0f;
        TestSystem.Emission = TestEmission;
        TestSystem.Motion = TestMotion;
        TestSystem.Appearance = TestAppearance;
        // TestSystem.NextParticle = 0;

        OffbeatState->ParticleSystemCount = 1;
        OffbeatState->ParticleSystems[0] = TestSystem;
    }
    return OffbeatState;
}

static void
OffbeatHandleInput(offbeat_state* OffbeatState, game_input* Input, offbeat_camera Camera)
{
    if(OffbeatState->UpdateParticles)
    {
        OffbeatState->dt = Input->dt;
        OffbeatState->t += OffbeatState->dt;
    }

    if(Input->q.EndedDown && Input->q.Changed)
    {
        OffbeatState->UpdateParticles = !OffbeatState->UpdateParticles;
    }

    offbeat_quad_data QuadData = {};
    QuadData.Horizontal = -Math::Normalized(Camera.Right);
    QuadData.Vertical = Math::Normalized(Math::Cross(QuadData.Horizontal, -Camera.Forward));
    OffbeatState->QuadData = QuadData;
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
TruncateF32ToS32(f32 F32)
{
    return (s32)F32;
}

inline u32
TruncateF32ToU32(f32 F32)
{
    return (u32)F32;
}

static v3
OffbeatParticleInitialPosition(random_series* Entropy, offbeat_emission* Emission)
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
            // TODO(rytis): This currently uses Y axis as the up direction. In future this
            // should be changed to either be independent from a specific coordinate system
            // or use a clearly defined internal coordinate system.
            f32 RandomValue = 2.0f * PI * RandomUnilateral(Entropy);
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

static v3
OffbeatParticleInitialVelocity(random_series* Entropy, offbeat_emission* Emission)
{
    v3 Result = {};

    switch(Emission->VelocityType)
    {
        case OFFBEAT_VelocityCone:
        {
            v3 RandomVector = v3{RandomBilateral(Entropy),
                                 RandomBilateral(Entropy),
                                 RandomBilateral(Entropy)};
            v3 AxisVector = Math::Normalized(Emission->Cone.Direction) * Emission->Cone.Height;
            f32 Length = Emission->Cone.EndRadius * RandomUnilateral(Entropy);
            v3 PerpVector = Math::Normalized(Math::Cross(AxisVector, Math::Normalized(RandomVector))) * Length;
            Result = Math::Normalized(AxisVector + PerpVector);
        } break;

        case OFFBEAT_VelocityRandom:
        {
            Result.X = RandomBilateral(Entropy);
            Result.Y = RandomBilateral(Entropy);
            Result.Z = RandomBilateral(Entropy);
            Result = Math::Normalized(Result);
        } break;

        default:
        {
        } break;
    }
    Result *= Emission->InitialVelocityScale;

    return Result;
}

static void
OffbeatSpawnParticles(offbeat_particle_system* ParticleSystem, random_series* Entropy, f32 dt)
{
    offbeat_emission* Emission = &ParticleSystem->Emission;
    offbeat_appearance* Appearance = &ParticleSystem->Appearance;

    ParticleSystem->t += dt;
    f32 TimePerParticle = 1.0f / Emission->EmissionRate;
    f32 RealParticleSpawn =  ParticleSystem->t * Emission->EmissionRate;
    OffbeatAssert(RealParticleSpawn >= 0.0f);
    u32 ParticleSpawnCount = TruncateF32ToU32(RealParticleSpawn);
    ParticleSystem->t -= ParticleSpawnCount * TimePerParticle;

    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < ParticleSpawnCount; ++ParticleSpawnIndex)
    {
        offbeat_particle* Particle = ParticleSystem->Particles + ParticleSystem->NextParticle++;
        if(ParticleSystem->NextParticle >= ArrayCount(ParticleSystem->Particles))
        {
            ParticleSystem->NextParticle = 0;
        }

        Particle->P = OffbeatParticleInitialPosition(Entropy, Emission);
        Particle->dP = OffbeatParticleInitialVelocity(Entropy, Emission);
        Particle->Color = Appearance->Color;
        Particle->Age = 0.0f;
        Debug::PushWireframeSphere(Particle->P, 0.02f, v4{0.8f, 0.0f, 0.0f, 1.0f});
    }
}

static v3
OffbeatUpdateParticleddP(offbeat_motion* Motion, offbeat_particle* Particle)
{
    v3 Result = Motion->Gravity;
    switch(Motion->Primitive)
    {
        case OFFBEAT_MotionPoint:
        {
            f32 Strength = Motion->Point.Strength;
            v3 Direction = Motion->Point.Position - Particle->P;
            f32 Distance = Math::Length(Direction);
            Result += Strength * Math::Normalized(Direction) / (Distance * Distance);
        } break;

        case OFFBEAT_MotionNone:
        default:
        {
        } break;
    }

    return Result;
}

static void
OffbeatConstructQuad(offbeat_draw_list* DrawList, offbeat_quad_data* QuadData, offbeat_appearance* Appearance, v3 ParticlePosition)
{
    OffbeatAssert(DrawList->IndexCount + 6 < OFFBEAT_MAX_INDEX_COUNT);
    OffbeatAssert(DrawList->VertexCount + 4 < OFFBEAT_MAX_VERTEX_COUNT);

    // NOTE(rytis): Vertices (from camera perspective)
    v3 TopLeft = ParticlePosition +
                 0.5f * Appearance->Size * (QuadData->Horizontal + QuadData->Vertical);
    v3 TopRight = ParticlePosition +
                  0.5f * Appearance->Size * (-QuadData->Horizontal + QuadData->Vertical);
    v3 BottomLeft = ParticlePosition +
                    0.5f * Appearance->Size * (QuadData->Horizontal - QuadData->Vertical);
    v3 BottomRight = ParticlePosition +
                     0.5f * Appearance->Size * (-QuadData->Horizontal - QuadData->Vertical);

    // NOTE(rytis): UVs
    v2 TopLeftUV = v2{0.0f, 1.0f};
    v2 TopRightUV = v2{1.0f, 1.0f};
    v2 BottomLeftUV = v2{0.0f, 0.0f};
    v2 BottomRightUV = v2{1.0f, 0.0f};

    u32 VertexIndex = DrawList->VertexCount;
    // NOTE(rytis): Updating draw list vertex array
    DrawList->Vertices[DrawList->VertexCount++] = offbeat_draw_vertex{TopLeft,
                                                                      TopLeftUV,
                                                                      Appearance->Color};
    DrawList->Vertices[DrawList->VertexCount++] = offbeat_draw_vertex{TopRight,
                                                                      TopRightUV,
                                                                      Appearance->Color};
    DrawList->Vertices[DrawList->VertexCount++] = offbeat_draw_vertex{BottomLeft,
                                                                      BottomLeftUV,
                                                                      Appearance->Color};
    DrawList->Vertices[DrawList->VertexCount++] = offbeat_draw_vertex{BottomRight,
                                                                      BottomRightUV,
                                                                      Appearance->Color};

    // NOTE(rytis): Updating draw list index array
    // NOTE(rytis): Top left triangle
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 2;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 1;
    // NOTE(rytis): Bottom right triangle
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 1;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 2;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 3;

    ++DrawList->ElementCount;
}

static void
OffbeatUpdateAndRenderParticleSystem(offbeat_draw_list* DrawList, offbeat_particle_system* ParticleSystem, f32 dt, b32 UpdateFlag, offbeat_quad_data* QuadData)
{
    offbeat_motion* Motion = &ParticleSystem->Motion;
    offbeat_appearance* Appearance = &ParticleSystem->Appearance;

    *DrawList = {};
    DrawList->TextureID = Appearance->TextureID;

    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(ParticleSystem->Particles); ++ParticleIndex)
    {
        offbeat_particle* Particle = ParticleSystem->Particles + ParticleIndex;

        if(Particle->Age >= 1.0f)
        {
            continue;
        }

        if(!UpdateFlag)
        {
            goto OffbeatRender;
        }

        Particle->Age += dt / ParticleSystem->Emission.ParticleLifetime;

        v3 ddP = OffbeatUpdateParticleddP(Motion, Particle);

        // NOTE(rytis): Simulate the particle forward in time
        Particle->P += 0.5f * Square(dt) * ddP + dt * Particle->dP;
        Particle->dP += dt * ddP;
        Particle->Color.A = 1.0f - Particle->Age;

#if 0
        if(Particle->P.Y < 0.0f)
        {
            f32 CoefficientOfRestitution = 0.3f;
            f32 CoefficientOfFriction = 0.7f;
            Particle->P.Y = -Particle->P.Y;
            Particle->dP.Y = -CoefficientOfRestitution * Particle->dP.Y;
            Particle->dP.X = CoefficientOfFriction * Particle->dP.X;
            Particle->dP.Z = CoefficientOfFriction * Particle->dP.Z;
            Particle->Color += v4{-0.1f, 0.0f, 0.0f, 0.0f};
        }
#endif

        // NOTE(rytis): Particle draw
OffbeatRender:
        v4 Color;
        Color.R = Clamp01(Particle->Color.R);
        Color.G = Clamp01(Particle->Color.G);
        Color.B = Clamp01(Particle->Color.B);
        Color.A = Clamp01(Particle->Color.A);

        OffbeatConstructQuad(DrawList, QuadData, Appearance, Particle->P);
    }
}

void
OffbeatParticleSystem(offbeat_state* OffbeatState, game_input* Input, offbeat_camera Camera)
{
    OffbeatHandleInput(OffbeatState, Input, Camera);
    OffbeatDrawGrid(OffbeatState);

    glEnable(GL_BLEND);

    OffbeatState->DrawData.DrawListCount = OffbeatState->ParticleSystemCount;
    for(u32 i = 0; i < OffbeatState->ParticleSystemCount; ++i)
    {
        if(OffbeatState->UpdateParticles)
        {
            // NOTE(rytis): Particle spawn / emission
            OffbeatSpawnParticles(&OffbeatState->ParticleSystems[i], &OffbeatState->EffectsEntropy, OffbeatState->dt);
        }

        // NOTE(rytis): Particle system update and render
        OffbeatUpdateAndRenderParticleSystem(&OffbeatState->DrawData.DrawLists[i], &OffbeatState->ParticleSystems[i], OffbeatState->dt, OffbeatState->UpdateParticles, &OffbeatState->QuadData);

        // NOTE(rytis): Motion primitive position render
        v3 Point = OffbeatState->ParticleSystems[i].Motion.Point.Position = v3{-3.0f * sinf(0.25f * PI * OffbeatState->t), 1.0f, 1.0f};
        v4 PointColor = v4{1.0f, 0.0f, 0.0f, 1.0f};
        Debug::PushWireframeSphere(Point, 0.05f, PointColor);
    }

    glDisable(GL_BLEND);
}

offbeat_draw_data*
OffbeatGetDrawData(offbeat_state* OffbeatState)
{
    return &OffbeatState->DrawData;
}
