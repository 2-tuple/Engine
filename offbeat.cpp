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

ob_state*
OffbeatInit(void* Memory, u64 MemorySize)
{
    OffbeatAssert(MemorySize >= sizeof(ob_state));
    ob_state* OffbeatState = (ob_state*)Memory;
    {
        OffbeatState->t = 0.0f;

        OffbeatState->EffectsEntropy = ObRandomSeed(1234);
        OffbeatState->UpdateParticles = true;

        OffbeatState->SquareGridColor = ov4{0.5f, 0.5f, 0.85f, 1.0f};
        OffbeatState->SquareGridLineCount = 0;
        OffbeatState->SquareGridStep = 2.0f;
        OffbeatState->SquareGridRange = 10.0f;
        f32 Limit = OffbeatState->SquareGridRange + 0.5f * OffbeatState->SquareGridStep;
        for(f32 Coordinate = -OffbeatState->SquareGridRange;
          Coordinate < Limit;
          Coordinate += OffbeatState->SquareGridStep)
        {
          OffbeatState->SquareGridLines[OffbeatState->SquareGridLineCount++] = ob_grid_line{
            ov3{Coordinate, 0.0f, -OffbeatState->SquareGridRange},
            ov3{Coordinate, 0.0f, OffbeatState->SquareGridRange}
          };
          OffbeatState->SquareGridLines[OffbeatState->SquareGridLineCount++] = ob_grid_line{
            ov3{-OffbeatState->SquareGridRange, 0.0f, Coordinate},
            ov3{OffbeatState->SquareGridRange, 0.0f, Coordinate}
          };
        }
        OffbeatAssert(OffbeatState->SquareGridLineCount < OFFBEAT_MAX_SQUARE_GRID_LINE_COUNT);

        ob_emission TestEmission = {};
        TestEmission.Location = ov3{0.0f, 0.0f, 0.0f};
        TestEmission.EmissionRate = 60.0f;
        TestEmission.ParticleLifetime = 1.5f;
#if 1
        TestEmission.Shape.Type = OFFBEAT_EmissionRing;
        TestEmission.Shape.Ring.Radius = 0.5f;
        TestEmission.Shape.Ring.Normal = ov3{0.0f, 1.0f, 0.0f};
        TestEmission.Shape.Ring.Rotation = ObRotationAlign(ov3{0.0f, 0.0f, 1.0f}, TestEmission.Shape.Ring.Normal);
#else
        TestEmission.Shape.Type = OFFBEAT_EmissionPoint;
#endif

        TestEmission.InitialVelocityScale = 1.0f;
#if 1
        TestEmission.Velocity.Type = OFFBEAT_VelocityCone;
        TestEmission.Velocity.Cone.Direction = ov3{0.0f, 1.0f, 0.0f};
        TestEmission.Velocity.Cone.Height = 5.0f;
        TestEmission.Velocity.Cone.Radius = 3.0f;
        TestEmission.Velocity.Cone.Rotation = ObRotationAlign(ov3{0.0f, 0.0f, 1.0f},
                                                     TestEmission.Velocity.Cone.Direction);
#else
        TestEmission.VelocityType = OFFBEAT_VelocityRandom;
#endif

        ob_motion TestMotion = {};
        TestMotion.Gravity = ov3{0.0f, 0.0f, 0.0f};
        TestMotion.Primitive = OFFBEAT_MotionPoint;
        TestMotion.Point.Position = ov3{-3.0f * sinf(0.25f * PI * OffbeatState->t), 1.0f, 1.0f};
        TestMotion.Point.Strength = 10.0f;

        ob_appearance TestAppearance = {};
        TestAppearance.Color = ov4{ObRandomBetween(&OffbeatState->EffectsEntropy, 0.45f, 0.5f),
                                   ObRandomBetween(&OffbeatState->EffectsEntropy, 0.5f, 0.8f),
                                   ObRandomBetween(&OffbeatState->EffectsEntropy, 0.8f, 1.0f),
                                   1.0f};
        TestAppearance.Size = 0.05f;

        ob_particle_system TestSystem = {};
        TestSystem.Emission = TestEmission;
        TestSystem.Motion = TestMotion;
        TestSystem.Appearance = TestAppearance;

        OffbeatState->ParticleSystemCount = 1;
        OffbeatState->ParticleSystems[0] = TestSystem;
    }
    return OffbeatState;
}

static void
OffbeatHandleInput(ob_state* OffbeatState, game_input* Input, ob_camera Camera)
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

    ob_quad_data QuadData = {};
    QuadData.Horizontal = -ObNormalize(Camera.Right);
    QuadData.Vertical = ObNormalize(ObCross(QuadData.Horizontal, -Camera.Forward));
    OffbeatState->QuadData = QuadData;
}

static void
OffbeatDrawGrid(ob_state* OffbeatState)
{
    for(u32 i = 0; i < OffbeatState->SquareGridLineCount; ++i)
    {
        vec3 A = OV3ToVec3(OffbeatState->SquareGridLines[i].A);
        vec3 B = OV3ToVec3(OffbeatState->SquareGridLines[i].B);
        vec4 GridColor = OV4ToVec4(OffbeatState->SquareGridColor);
        Debug::PushLine(A, B, GridColor);
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

static ov3
OffbeatParticleInitialPosition(ob_random_series* Entropy, ob_emission* Emission)
{
    ov3 Result = {};

    switch(Emission->Shape.Type)
    {
        case OFFBEAT_EmissionPoint:
        {
            Result = Emission->Location;
        } break;

        case OFFBEAT_EmissionRing:
        {
            f32 RandomValue = 2.0f * PI * ObRandomUnilateral(Entropy);

            Result.x = Emission->Shape.Ring.Radius * ObSin(RandomValue);
            Result.y = Emission->Shape.Ring.Radius * ObCos(RandomValue);

            Result = Emission->Shape.Ring.Rotation * Result;

            Result += Emission->Location;
        } break;

        default:
        {
        } break;
    }

    return Result;
}

static ov3
OffbeatParticleInitialVelocity(ob_random_series* Entropy, ob_emission* Emission)
{
    ov3 Result = {};

    switch(Emission->Velocity.Type)
    {
        case OFFBEAT_VelocityCone:
        {
            f32 Denom = ObSquareRoot(ObSquare(Emission->Velocity.Cone.Height) + ObSquare(Emission->Velocity.Cone.Radius));
            f32 CosTheta = Emission->Velocity.Cone.Height / Denom;

            f32 Phi = ObRandomBetween(Entropy, 0.0f, 2.0f * PI);
            f32 Z = ObRandomBetween(Entropy, CosTheta, 1.0f);

            // NOTE(rytis): Vector generated around axis (0, 0, 1)
            ov3 RandomVector = {};
            RandomVector.x = ObSquareRoot(1.0f - ObSquare(Z)) * ObCos(Phi);
            RandomVector.y = ObSquareRoot(1.0f - ObSquare(Z)) * ObSin(Phi);
            RandomVector.z = Z;

            Result = Emission->Velocity.Cone.Rotation * RandomVector;
        } break;

        case OFFBEAT_VelocityRandom:
        {
            f32 Theta = ObRandomBetween(Entropy, 0.0f, 2.0f * PI);
            f32 Z = ObRandomBilateral(Entropy);

            // NOTE(rytis): This generates a vector in a unit sphere, so no normalization is required.
            Result.x = ObSquareRoot(1.0f - ObSquare(Z)) * ObCos(Theta);
            Result.y = ObSquareRoot(1.0f - ObSquare(Z)) * ObSin(Theta);
            Result.z = Z;
        } break;

        default:
        {
        } break;
    }
    Result *= Emission->InitialVelocityScale;

    return Result;
}

static void
OffbeatSpawnParticles(ob_particle_system* ParticleSystem, ob_random_series* Entropy, f32 dt)
{
    ob_emission* Emission = &ParticleSystem->Emission;
    ob_appearance* Appearance = &ParticleSystem->Appearance;

    ParticleSystem->t += dt;
    f32 TimePerParticle = 1.0f / Emission->EmissionRate;
    f32 RealParticleSpawn =  ParticleSystem->t * Emission->EmissionRate;
    OffbeatAssert(RealParticleSpawn >= 0.0f);
    u32 ParticleSpawnCount = TruncateF32ToU32(RealParticleSpawn);
    ParticleSystem->t -= ParticleSpawnCount * TimePerParticle;

    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < ParticleSpawnCount; ++ParticleSpawnIndex)
    {
        ob_particle* Particle = ParticleSystem->Particles + ParticleSystem->NextParticle++;
        if(ParticleSystem->NextParticle >= ArrayCount(ParticleSystem->Particles))
        {
            ParticleSystem->NextParticle = 0;
        }

        Particle->P = OffbeatParticleInitialPosition(Entropy, Emission);
        Particle->dP = OffbeatParticleInitialVelocity(Entropy, Emission);
        Particle->Color = Appearance->Color;
        // Particle->Size = Appearance->Size;
        Particle->Age = 0.0f;
        Debug::PushWireframeSphere(OV3ToVec3(Particle->P), 0.02f,
                                   OV4ToVec4(ov4{0.8f, 0.0f, 0.0f, 1.0f}));
    }
}

static ov3
OffbeatUpdateParticleddP(ob_motion* Motion, ob_particle* Particle)
{
    ov3 Result = Motion->Gravity;
    switch(Motion->Primitive)
    {
        case OFFBEAT_MotionPoint:
        {
            f32 Strength = Motion->Point.Strength;
            ov3 Direction = Motion->Point.Position - Particle->P;
            f32 Distance = ObLength(Direction);
            Result += Strength * ObNormalize(Direction) / (Distance * Distance);
        } break;

        case OFFBEAT_MotionNone:
        default:
        {
        } break;
    }

    return Result;
}

static void
OffbeatConstructQuad(ob_draw_list* DrawList, ob_quad_data* QuadData, ob_appearance* Appearance, ov3 ParticlePosition)
{
    OffbeatAssert(DrawList->IndexCount + 6 < OFFBEAT_MAX_INDEX_COUNT);
    OffbeatAssert(DrawList->VertexCount + 4 < OFFBEAT_MAX_VERTEX_COUNT);

    // NOTE(rytis): Vertices (from camera perspective)
    ov3 BottomLeft = ParticlePosition +
                    0.5f * Appearance->Size * (QuadData->Horizontal - QuadData->Vertical);
    ov3 BottomRight = ParticlePosition +
                     0.5f * Appearance->Size * (-QuadData->Horizontal - QuadData->Vertical);
    ov3 TopRight = ParticlePosition +
                  0.5f * Appearance->Size * (-QuadData->Horizontal + QuadData->Vertical);
    ov3 TopLeft = ParticlePosition +
                 0.5f * Appearance->Size * (QuadData->Horizontal + QuadData->Vertical);

    // NOTE(rytis): UVs
    ov2 BottomLeftUV = ov2{0.0f, 0.0f};
    ov2 BottomRightUV = ov2{1.0f, 0.0f};
    ov2 TopRightUV = ov2{1.0f, 1.0f};
    ov2 TopLeftUV = ov2{0.0f, 1.0f};

    u32 VertexIndex = DrawList->VertexCount;
    // NOTE(rytis): Updating draw list vertex array
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{BottomLeft,
                                                                 BottomLeftUV,
                                                                 Appearance->Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{BottomRight,
                                                                 BottomRightUV,
                                                                 Appearance->Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{TopRight,
                                                                 TopRightUV,
                                                                 Appearance->Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{TopLeft,
                                                                 TopLeftUV,
                                                                 Appearance->Color};

    // NOTE(rytis): Updating draw list index array
    // NOTE(rytis): Bottom right triangle
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 1;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 2;
    // NOTE(rytis): Top left triangle
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 2;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 3;

    ++DrawList->ElementCount;
}

static void
OffbeatUpdateAndRenderParticleSystem(ob_draw_list* DrawList, ob_particle_system* ParticleSystem, f32 dt, b32 UpdateFlag, ob_quad_data* QuadData)
{
    ob_motion* Motion = &ParticleSystem->Motion;
    ob_appearance* Appearance = &ParticleSystem->Appearance;

    *DrawList = {};
    DrawList->TextureID = Appearance->TextureID;

    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(ParticleSystem->Particles); ++ParticleIndex)
    {
        ob_particle* Particle = ParticleSystem->Particles + ParticleIndex;

        if(Particle->Age >= 1.0f)
        {
            continue;
        }

        if(!UpdateFlag)
        {
            goto OffbeatRender;
        }

        Particle->Age += dt / ParticleSystem->Emission.ParticleLifetime;

        ov3 ddP = OffbeatUpdateParticleddP(Motion, Particle);

        // NOTE(rytis): Simulate the particle forward in time
        Particle->P += 0.5f * ObSquare(dt) * ddP + dt * Particle->dP;
        Particle->dP += dt * ddP;
        // Particle->Color.a = 1.0f - Particle->Age;

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
        ov4 Color = ObClamp01(Particle->Color);
        OffbeatConstructQuad(DrawList, QuadData, Appearance, Particle->P);
    }
}

void
OffbeatParticleSystem(ob_state* OffbeatState, game_input* Input, ob_camera Camera)
{
    OffbeatHandleInput(OffbeatState, Input, Camera);
    OffbeatDrawGrid(OffbeatState);

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
        ov3 Point = OffbeatState->ParticleSystems[i].Motion.Point.Position = ov3{-3.0f * ObSin(0.25f * PI * OffbeatState->t), 1.0f, 1.0f};
        ov4 PointColor = ov4{1.0f, 0.0f, 0.0f, 1.0f};
        Debug::PushWireframeSphere(OV3ToVec3(Point), 0.05f, OV4ToVec4(PointColor));
    }
}

ob_draw_data*
OffbeatGetDrawData(ob_state* OffbeatState)
{
    return &OffbeatState->DrawData;
}
