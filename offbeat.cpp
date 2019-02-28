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

static void
ObZeroMemory(void* Pointer, u64 Size)
{
    u8* Byte = (u8*)Pointer;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

ob_state*
OffbeatInit(void* Memory, u64 MemorySize)
{
    u64 StateSize = sizeof(ob_state);
    OffbeatAssert(MemorySize >= StateSize);
    ObZeroMemory(Memory, MemorySize);

    ob_state* OffbeatState = (ob_state*)Memory;

    // NOTE(rytis): Memory setup.
    {
        // NOTE(rytis): Can waste 1 byte. Not important.
        u64 HalfSize = (MemorySize - StateSize) / 2;

        u8* Marker = (u8*)Memory;

        Marker += StateSize + (StateSize % 2);

        OffbeatState->MemoryManager.LastBuffer = Marker;

        Marker += HalfSize;
        OffbeatState->MemoryManager.LastMaxAddress =
            OffbeatState->MemoryManager.CurrentBuffer = Marker;

        Marker += HalfSize;
        OffbeatState->MemoryManager.CurrentMaxAddress = Marker;

        // NOTE(rytis): Might be unnecessary, since there will be an update.
        OffbeatState->MemoryManager.CurrentAddress = OffbeatState->MemoryManager.CurrentBuffer;

        OffbeatAssert(OffbeatState->MemoryManager.CurrentMaxAddress <= ((u8*)Memory + MemorySize));
    }
    // NOTE(rytis): OffbeatState init.
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
        // TestEmission.Shape.Ring.Rotation = ObRotationAlign(ov3{0.0f, 0.0f, 1.0f}, TestEmission.Shape.Ring.Normal);
#else
        TestEmission.Shape.Type = OFFBEAT_EmissionPoint;
#endif

        TestEmission.InitialVelocityScale = 1.0f;
#if 1
        TestEmission.Velocity.Type = OFFBEAT_VelocityCone;
        TestEmission.Velocity.Cone.Direction = ov3{0.0f, 1.0f, 0.0f};
        TestEmission.Velocity.Cone.Height = 5.0f;
        TestEmission.Velocity.Cone.Radius = 3.0f;
        // TestEmission.Velocity.Cone.Rotation = ObRotationAlign(ov3{0.0f, 0.0f, 1.0f},
        //                                              TestEmission.Velocity.Cone.Direction);
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

        OffbeatState->ParticleSystems[0] = TestSystem;

#if 0
        TestEmission = {};
        TestEmission.Location = ov3{2.0f, 0.0f, -2.0f};
        TestEmission.EmissionRate = 100.0f;
        TestEmission.ParticleLifetime = 2.5f;
        TestEmission.Shape.Type = OFFBEAT_EmissionPoint;

        TestEmission.InitialVelocityScale = 0.3f;
        TestEmission.Velocity.Type = OFFBEAT_VelocityCone;
        TestEmission.Velocity.Cone.Direction = ov3{0.0f, 1.0f, 0.0f};
        TestEmission.Velocity.Cone.Height = 10.0f;
        TestEmission.Velocity.Cone.Radius = 2.0f;
        // TestEmission.Velocity.Cone.Rotation = ObRotationAlign(ov3{0.0f, 0.0f, 1.0f},
                                                     TestEmission.Velocity.Cone.Direction);

        TestMotion = {};
        TestMotion.Gravity = ov3{0.0f, -0.2f, 0.0f};

        TestAppearance = {};
        TestAppearance.Color = ov4{0.2f, 0.8f, 0.4f, 1.0f};
        TestAppearance.Size = 0.01f;

        TestSystem = {};
        TestSystem.Emission = TestEmission;
        TestSystem.Motion = TestMotion;
        TestSystem.Appearance = TestAppearance;

        OffbeatState->ParticleSystemCount = 2;
        OffbeatState->ParticleSystems[1] = TestSystem;
#else
        OffbeatState->ParticleSystemCount = 1;
#endif
    }
    return OffbeatState;
}

ob_state*
OffbeatInit(void* (*Malloc)(u64))
{
    u64 MemorySize = OffbeatMibibytes(100);
    void* Memory = Malloc(MemorySize);
    return OffbeatInit(Memory, MemorySize);
}

ob_state*
OffbeatInit()
{
    return OffbeatInit(GlobalOffbeatAlloc);
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

ob_particle_system*
OffbeatNewParticleSystem(ob_state* OffbeatState)
{
    OffbeatAssert(OffbeatState->ParticleSystemCount + 1 <= OFFBEAT_PARTICLE_SYSTEM_COUNT);
    ob_particle_system* Result = &OffbeatState->ParticleSystems[OffbeatState->ParticleSystemCount++];
    *Result = {};
    return Result;
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
OffbeatSpawnParticles(ob_particle* Particles, ob_particle_system* ParticleSystem, ob_random_series* Entropy, f32 dt)
{
    ob_emission* Emission = &ParticleSystem->Emission;

    u32 Index = ObMin(ParticleSystem->HistoryEndIndex - 1, OffbeatArrayCount(ParticleSystem->History));
    u32 ParticleSpawnCount = ParticleSystem->History[Index].ParticlesEmitted;
    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < ParticleSpawnCount; ++ParticleSpawnIndex)
    {
        ob_particle* Particle = Particles + ParticleSpawnIndex;

        Particle->P = OffbeatParticleInitialPosition(Entropy, Emission);
        Particle->dP = OffbeatParticleInitialVelocity(Entropy, Emission);
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
OffbeatUpdateParticleSystem(ob_particle* Particles, ob_particle_system* ParticleSystem, f32 dt)
{
    ob_motion* Motion = &ParticleSystem->Motion;

    u32 Index = ObMin(ParticleSystem->HistoryEndIndex - 1, OffbeatArrayCount(ParticleSystem->History));
    u32 ParticleSpawnCount = ParticleSystem->History[Index].ParticlesEmitted;
    u32 LastParticleCount = ParticleSystem->ParticleCount - ParticleSpawnCount;

    ob_particle* UpdatedParticle = Particles + ParticleSpawnCount;
    for(u32 ParticleIndex = 0; ParticleIndex < LastParticleCount; ++ParticleIndex)
    {
        ob_particle* LastParticle = ParticleSystem->Particles + ParticleIndex;

        if(LastParticle->Age >= 1.0f)
        {
            continue;
        }

        *UpdatedParticle = *LastParticle;

        UpdatedParticle->Age += dt / ParticleSystem->Emission.ParticleLifetime;

        ov3 ddP = OffbeatUpdateParticleddP(Motion, UpdatedParticle);

        UpdatedParticle->P += 0.5f * ObSquare(dt) * ddP + dt * UpdatedParticle->dP;
        UpdatedParticle->dP += dt * ddP;
        ++UpdatedParticle;
    }

    ParticleSystem->Particles = Particles;
}

static void
OffbeatConstructQuad(ob_draw_list* DrawList, ob_quad_data* QuadData, ob_appearance* Appearance, ov3 ParticlePosition)
{
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
OffbeatRenderParticleSystem(ob_draw_list* DrawList, u32* IndexMemory, ob_draw_vertex* VertexMemory, ob_particle_system* ParticleSystem, ob_quad_data* QuadData)
{
    ob_appearance* Appearance = &ParticleSystem->Appearance;

    *DrawList = {};
    DrawList->TextureID = Appearance->TextureID;
    DrawList->Indices = IndexMemory;
    DrawList->Vertices = VertexMemory;

    for(u32 ParticleIndex = 0; ParticleIndex < ParticleSystem->ParticleCount; ++ParticleIndex)
    {
        ob_particle* Particle = ParticleSystem->Particles + ParticleIndex;

        OffbeatConstructQuad(DrawList, QuadData, Appearance, Particle->P);
    }
}

static void
OffbeatUpdateParticleCount(ob_particle_system* ParticleSystem, f32 t, f32 dt)
{
    ob_emission* Emission = &ParticleSystem->Emission;

    ParticleSystem->t += dt;

    f32 TimePerParticle = 1.0f / Emission->EmissionRate;
    f32 RealParticleSpawn =  ParticleSystem->t * Emission->EmissionRate;
    OffbeatAssert(RealParticleSpawn >= 0.0f);

    u32 ParticleSpawnCount = TruncateF32ToU32(RealParticleSpawn);
    ParticleSystem->t -= ParticleSpawnCount * TimePerParticle;
    ParticleSystem->ParticleCount += ParticleSpawnCount;

    ob_history_entry Entry = {};
    Entry.TimeElapsed = t;
    Entry.ParticlesEmitted = ParticleSpawnCount;
    ParticleSystem->History[ParticleSystem->HistoryEndIndex] = Entry;
    ParticleSystem->HistoryEndIndex = (ParticleSystem->HistoryEndIndex + 1) % OFFBEAT_HISTORY_ENTRY_COUNT;

    f32 MinTimeElapsed = ObClamp(0.0f, t - ParticleSystem->Emission.ParticleLifetime, t);
    u32 i = ParticleSystem->HistoryStartIndex;
    while(ParticleSystem->History[i].TimeElapsed < MinTimeElapsed)
    {
        ParticleSystem->ParticleCount -= ParticleSystem->History[i].ParticlesEmitted;
        i = (i + 1) % OFFBEAT_HISTORY_ENTRY_COUNT;
    }
    ParticleSystem->HistoryStartIndex = i;
}

static void
OffbeatUpdateMemoryManager(ob_memory_manager* MemoryManager)
{
    u8* Temp = MemoryManager->CurrentBuffer;
    u8* TempMax = MemoryManager->CurrentMaxAddress;

    MemoryManager->CurrentBuffer = MemoryManager->LastBuffer;
    MemoryManager->CurrentMaxAddress = MemoryManager->LastMaxAddress;

    MemoryManager->LastBuffer = Temp;
    MemoryManager->LastMaxAddress = TempMax;

    MemoryManager->CurrentAddress = MemoryManager->CurrentBuffer;
}

static void*
OffbeatAllocateMemory(ob_memory_manager* MemoryManager, u64 Size)
{
    OffbeatAssert(MemoryManager->CurrentAddress + Size < MemoryManager->CurrentMaxAddress);
    void* Result = MemoryManager->CurrentAddress;
    MemoryManager->CurrentAddress += Size;
    return Result;
}

static void
OffbeatUpdateSystemRotations(ob_particle_system* ParticleSystem)
{
    ob_emission* Emission = &ParticleSystem->Emission;
    ov3 Z = ov3{0.0f, 0.0f, 1.0f};

    switch(Emission->Shape.Type)
    {
        case OFFBEAT_EmissionRing:
        {
            Emission->Shape.Ring.Rotation = ObRotationAlign(Z, Emission->Shape.Ring.Normal);
        } break;
    }

    switch(Emission->Velocity.Type)
    {
        case OFFBEAT_VelocityCone:
        {
            Emission->Velocity.Cone.Rotation = ObRotationAlign(Z, Emission->Velocity.Cone.Direction);
        } break;
    }
}

void
OffbeatUpdate(ob_state* OffbeatState, ob_camera Camera, f32 dt)
{
    {
        OffbeatState->dt = dt;
        OffbeatState->t += OffbeatState->dt;

        ob_quad_data QuadData = {};
        QuadData.Horizontal = -ObNormalize(Camera.Right);
        QuadData.Vertical = ObNormalize(ObCross(QuadData.Horizontal, -Camera.Forward));
        OffbeatState->QuadData = QuadData;

        OffbeatUpdateMemoryManager(&OffbeatState->MemoryManager);
    }

    OffbeatDrawGrid(OffbeatState);

    OffbeatState->DrawData.DrawListCount = OffbeatState->ParticleSystemCount;
    for(u32 i = 0; i < OffbeatState->ParticleSystemCount; ++i)
    {
        ob_particle_system* ParticleSystem = &OffbeatState->ParticleSystems[i];

        OffbeatUpdateSystemRotations(ParticleSystem);

        OffbeatUpdateParticleCount(ParticleSystem, OffbeatState->t, OffbeatState->dt);

        ob_particle* Particles =
            (ob_particle*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                ParticleSystem->ParticleCount *
                                                sizeof(ob_particle));
        OffbeatSpawnParticles(Particles, ParticleSystem, &OffbeatState->EffectsEntropy,
                              OffbeatState->dt);
        OffbeatUpdateParticleSystem(Particles, ParticleSystem, OffbeatState->dt);

        u32* IndexMemory =
            (u32*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                        ParticleSystem->ParticleCount *
                                        sizeof(u32) * 6);
        ob_draw_vertex* VertexMemory =
            (ob_draw_vertex*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                   ParticleSystem->ParticleCount *
                                                   sizeof(ob_draw_vertex) * 4);
        OffbeatRenderParticleSystem(&OffbeatState->DrawData.DrawLists[i], IndexMemory, VertexMemory,
                                    ParticleSystem, &OffbeatState->QuadData);

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
