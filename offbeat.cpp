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

static void* (*OffbeatGlobalAlloc)(u64) = _Malloc;
static void (*OffbeatGlobalFree)(void*) = _Free;

void
OffbeatSetAllocatorFunctions(void* (*Malloc)(u64), void (*Free)(void*))
{
    OffbeatGlobalAlloc = Malloc;
    OffbeatGlobalFree = Free;
}

#ifdef OFFBEAT_OPENGL
ob_texture OffbeatRGBATextureID(void* TextureData, u32 Width, u32 Height)
{
    ob_texture LastBoundTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&LastBoundTexture);

    ob_texture TextureID;
    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, TextureData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glBindTexture(GL_TEXTURE_2D, LastBoundTexture);
    return TextureID;
}
#endif

static ob_texture (*OffbeatGlobalRGBATextureID)(void*, u32, u32) = OffbeatRGBATextureID;

void
OffbeatSetTextureFunction(ob_texture (*TextureFunction)(void*, u32, u32))
{
    OffbeatGlobalRGBATextureID = TextureFunction;
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
ObZeroMemory(void* Pointer, u64 Size)
{
    u8* Byte = (u8*)Pointer;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

void
OffbeatGenerateSquareRGBATexture(void* Memory, u64 MemorySize, u32 Width, u32 Height)
{
    OffbeatAssert(MemorySize >= (Width * Height * sizeof(u32)));

    u32* Pixel = (u32*)Memory;
    for(u32 i = 0; i < Height; ++i)
    {
        for(u32 j = 0; j < Width; ++j)
        {
            *Pixel++ = 0xFFFFFFFF;
        }
    }
}

void
OffbeatGenerateCircleRGBATexture(void* Memory, u64 MemorySize, u32 Width, u32 Height)
{
    OffbeatAssert(MemorySize >= (Width * Height * sizeof(u32)));

    s32 CenterHeight = Height / 2;
    s32 CenterWidth = Width / 2;

    u32* Pixel = (u32*)Memory;
    for(s32 i = 0; i < Height; ++i)
    {
        for(s32 j = 0; j < Width; ++j)
        {
            // u8 Alpha = ObSin(i * PI / Height) * ObSin(j * PI / Width);
            f32 LengthSq = ObSquare((float)(i - CenterHeight) / (float)CenterHeight) +
                           ObSquare((float)(j - CenterWidth) / (float)CenterWidth);
            f32 Scale = 1.0f - ObClamp(0.0f, ObSquareRoot(LengthSq), 1.0f);
            u32 Alpha = TruncateF32ToU32(0xFF * Scale);
            // NOTE(rytis): ABGR in registers, RGBA in memory.
            *Pixel++ = 0x00FFFFFF | (Alpha << 24);
        }
    }
}

ob_texture_type
OffbeatGetCurrentTextureType(ob_particle_system* ParticleSystem)
{
    for(u32 i = 0; i < OFFBEAT_TextureCount; ++i)
    {
        if(ParticleSystem->Appearance.TextureID == OffbeatGlobalTextureIDs[i])
        {
            return (ob_texture_type)i;
        }
    }
    return OFFBEAT_TextureCount;
}

void
OffbeatSetTextureID(ob_particle_system* ParticleSystem, ob_texture_type NewType)
{
    ParticleSystem->Appearance.TextureID = OffbeatGlobalTextureIDs[NewType];
}

// TODO(rytis): Fix this parameter mess.
static f32
EvaluateParameter(ob_parameter Parameter, ob_particle* Particle, f32* t, f32* tSystem)
{
    f32 Result;

    switch(Parameter)
    {
        case OFFBEAT_ParameterGlobalTime:
        {
            Result = *t;
        } break;

        case OFFBEAT_ParameterSystemTime:
        {
            Result = *tSystem;
        } break;

        case OFFBEAT_ParameterAge:
        case OFFBEAT_ParameterRandom:
        case OFFBEAT_ParameterCameraDistance:
        case OFFBEAT_ParameterID:
        {
            Result = *(f32*)((u8*)Particle + OffbeatGlobalOffsets[Parameter]);
        } break;

        default:
        {
            Result = 0.0f;
        } break;
    }

    return Result;
}

static f32
EvaluateExpression(ob_expr_f32* Expr, ob_particle* Particle, f32* t, f32* tSystem)
{
    f32 Param = EvaluateParameter(Expr->Parameter, Particle, t, tSystem);
    switch(Expr->Function)
    {
        case OFFBEAT_FunctionConst:
        {
            return Expr->High;
        } break;

        case OFFBEAT_FunctionLerp:
        {
            return ObLerp(Expr->Low, ObClamp01(Param), Expr->High);
        } break;

        case OFFBEAT_FunctionSin:
        {
            f32 Sin01 = 0.5f * (ObSin(2.0f * PI * Param) + 1.0f);
            return ObLerp(Expr->Low, Sin01, Expr->High);
        } break;

        case OFFBEAT_FunctionCos:
        {
            f32 Cos01 = 0.5f * (ObCos(2.0f * PI * Param) + 1.0f);
            return ObLerp(Expr->Low, Cos01, Expr->High);
        } break;

        case OFFBEAT_FunctionFloor:
        {
            f32 Floor = ObFloor(Param);
            return ObLerp(Expr->Low, ObClamp01(Floor), Expr->High);
        } break;

        case OFFBEAT_FunctionRound:
        {
            f32 Round = ObRound(Param);
            return ObLerp(Expr->Low, ObClamp01(Round), Expr->High);
        } break;

        case OFFBEAT_FunctionCeil:
        {
            f32 Ceil = ObCeil(Param);
            return ObLerp(Expr->Low, ObClamp01(Ceil), Expr->High);
        } break;

        default:
        {
            return 0.0f;
        } break;
    }
}

static ov3
EvaluateExpression(ob_expr_ov3* Expr, ob_particle* Particle, f32* t, f32* tSystem)
{
    f32 Param = EvaluateParameter(Expr->Parameter, Particle, t, tSystem);
    switch(Expr->Function)
    {
        case OFFBEAT_FunctionConst:
        {
            return Expr->High;
        } break;

        case OFFBEAT_FunctionLerp:
        {
            return ObLerp(Expr->Low, ObClamp01(Param), Expr->High);
        } break;

        case OFFBEAT_FunctionSin:
        {
            f32 Sin01 = 0.5f * (ObSin(2.0f * PI * Param) + 1.0f);
            return ObLerp(Expr->Low, Sin01, Expr->High);
        } break;

        case OFFBEAT_FunctionCos:
        {
            f32 Cos01 = 0.5f * (ObCos(2.0f * PI * Param) + 1.0f);
            return ObLerp(Expr->Low, Cos01, Expr->High);
        } break;

        case OFFBEAT_FunctionFloor:
        {
            f32 Floor = ObFloor(Param);
            return ObLerp(Expr->Low, ObClamp01(Floor), Expr->High);
        } break;

        case OFFBEAT_FunctionRound:
        {
            f32 Round = ObRound(Param);
            return ObLerp(Expr->Low, ObClamp01(Round), Expr->High);
        } break;

        case OFFBEAT_FunctionCeil:
        {
            f32 Ceil = ObCeil(Param);
            return ObLerp(Expr->Low, ObClamp01(Ceil), Expr->High);
        } break;

        default:
        {
            return ov3{};
        } break;
    }
}

static ov4
EvaluateExpression(ob_expr_ov4* Expr, ob_particle* Particle, f32* t, f32* tSystem)
{
    f32 Param = EvaluateParameter(Expr->Parameter, Particle, t, tSystem);
    switch(Expr->Function)
    {
        case OFFBEAT_FunctionConst:
        {
            return Expr->High;
        } break;

        case OFFBEAT_FunctionLerp:
        {
            return ObLerp(Expr->Low, ObClamp01(Param), Expr->High);
        } break;

        case OFFBEAT_FunctionSin:
        {
            f32 Sin01 = 0.5f * (ObSin(2.0f * PI * Param) + 1.0f);
            return ObLerp(Expr->Low, Sin01, Expr->High);
        } break;

        case OFFBEAT_FunctionCos:
        {
            f32 Cos01 = 0.5f * (ObCos(2.0f * PI * Param) + 1.0f);
            return ObLerp(Expr->Low, Cos01, Expr->High);
        } break;

        case OFFBEAT_FunctionFloor:
        {
            f32 Floor = ObFloor(Param);
            return ObLerp(Expr->Low, ObClamp01(Floor), Expr->High);
        } break;

        case OFFBEAT_FunctionRound:
        {
            f32 Round = ObRound(Param);
            return ObLerp(Expr->Low, ObClamp01(Round), Expr->High);
        } break;

        case OFFBEAT_FunctionCeil:
        {
            f32 Ceil = ObCeil(Param);
            return ObLerp(Expr->Low, ObClamp01(Ceil), Expr->High);
        } break;

        default:
        {
            return ov4{};
        } break;
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

        // NOTE(rytis): Might be unnecessary, since there will be an initial memory update.
        OffbeatState->MemoryManager.CurrentAddress = OffbeatState->MemoryManager.CurrentBuffer;

        OffbeatAssert(OffbeatState->MemoryManager.CurrentMaxAddress <= ((u8*)Memory + MemorySize));
    }

    // NOTE(rytis): Offbeat texture init.
    {
        u32 Width = 500;
        u32 Height = 500;
        u64 Size = Width * Height * sizeof(u32);

        void* TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateSquareRGBATexture(TextureMemory, Size, Width, Height);
        OffbeatGlobalTextureIDs[OFFBEAT_TextureSquare] = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);

        TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateCircleRGBATexture(TextureMemory, Size, Width, Height);
        OffbeatGlobalTextureIDs[OFFBEAT_TextureCircle] = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);
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

        TestEmission.Shape.Type = OFFBEAT_EmissionRing;
        TestEmission.Shape.Ring.Radius = 0.5f;
        TestEmission.Shape.Ring.Normal = ov3{0.0f, 1.0f, 0.0f};

        TestEmission.InitialVelocityScale = 1.0f;
        TestEmission.Velocity.Type = OFFBEAT_VelocityCone;
        TestEmission.Velocity.Cone.Direction = ov3{0.0f, 1.0f, 0.0f};
        TestEmission.Velocity.Cone.Height = 5.0f;
        TestEmission.Velocity.Cone.Radius = 3.0f;

        ob_motion TestMotion = {};
        TestMotion.Gravity = ov3{0.0f, 0.0f, 0.0f};
        TestMotion.Primitive = OFFBEAT_MotionPoint;
        TestMotion.Point.Position = ov3{-3.0f * sinf(0.25f * PI * OffbeatState->t), 1.0f, 1.0f};
        TestMotion.Point.Strength.High = 10.0f;

        ob_appearance TestAppearance = {};
        TestAppearance.Color.Low = TestAppearance.Color.High =
            ov4{ObRandomBetween(&OffbeatState->EffectsEntropy, 0.45f, 0.5f),
            ObRandomBetween(&OffbeatState->EffectsEntropy, 0.5f, 0.8f),
            ObRandomBetween(&OffbeatState->EffectsEntropy, 0.8f, 1.0f),
            1.0f};
        TestAppearance.Size.Low = TestAppearance.Size.High = 0.05f;
        TestAppearance.TextureID = OffbeatGlobalTextureIDs[OFFBEAT_TextureCircle];

        ob_particle_system TestSystem = {};
        TestSystem.Emission = TestEmission;
        TestSystem.Motion = TestMotion;
        TestSystem.Appearance = TestAppearance;

        OffbeatState->ParticleSystems[0] = TestSystem;
        OffbeatState->ParticleSystemCount = 1;
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
    return OffbeatInit(OffbeatGlobalAlloc);
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

u32
OffbeatNewParticleSystem(ob_state* OffbeatState, ob_particle_system** NewParticleSystem)
{
    OffbeatAssert(OffbeatState->ParticleSystemCount + 1 <= OFFBEAT_PARTICLE_SYSTEM_COUNT);
    u32 Result = OffbeatState->ParticleSystemCount;
    *NewParticleSystem = &OffbeatState->ParticleSystems[OffbeatState->ParticleSystemCount++];
    OffbeatState->ParticleSystems[Result] = {};
    return Result;
}

ob_particle_system*
OffbeatNewParticleSystem(ob_state* OffbeatState)
{
    OffbeatAssert(OffbeatState->ParticleSystemCount + 1 <= OFFBEAT_PARTICLE_SYSTEM_COUNT);
    ob_particle_system* Result = &OffbeatState->ParticleSystems[OffbeatState->ParticleSystemCount++];
    *Result = {};
    return Result;
}

void
OffbeatRemoveParticleSystem(ob_state* OffbeatState, u32 Index)
{
    for(u32 i = Index; i < OffbeatState->ParticleSystemCount - 1; ++i)
    {
        OffbeatState->ParticleSystems[i] = OffbeatState->ParticleSystems[i + 1];
    }
    --OffbeatState->ParticleSystemCount;
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
            f32 CosTheta = Denom > 0.0f ? Emission->Velocity.Cone.Height / Denom : 0.0f;

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
OffbeatSpawnParticles(ob_particle* Particles, ob_particle_system* ParticleSystem, ob_random_series* Entropy, f32 dt, ov3* CameraPosition, u32* RunningParticleID)
{
    ob_emission* Emission = &ParticleSystem->Emission;

    u32 Index = ParticleSystem->HistoryEntryCount - 1;
    u32 ParticleSpawnCount = ParticleSystem->History[Index].ParticlesEmitted;
    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < ParticleSpawnCount; ++ParticleSpawnIndex)
    {
        ob_particle* Particle = Particles + ParticleSpawnIndex;

        Particle->P = OffbeatParticleInitialPosition(Entropy, Emission);
        Particle->dP = OffbeatParticleInitialVelocity(Entropy, Emission);
        Particle->Age = 0.0f;
        Particle->Random = ObRandomUnilateral(Entropy);
        Particle->CameraDistance = ObLength(*CameraPosition - Particle->P);
        Particle->ID = (*RunningParticleID)++;
        Debug::PushWireframeSphere(OV3ToVec3(Particle->P), 0.02f,
                                   OV4ToVec4(ov4{0.8f, 0.0f, 0.0f, 1.0f}));
    }
}

static ov3
OffbeatUpdateParticleddP(ob_motion* Motion, ob_particle* Particle, f32* t, f32* tSystem)
{
    f32 LengthSq = ObLengthSq(Particle->dP);
    ov3 Drag = Motion->Drag * LengthSq * ObNormalize(-Particle->dP);
    ov3 Result = Motion->Gravity + Drag;
    switch(Motion->Primitive)
    {
        case OFFBEAT_MotionPoint:
        {
            f32 Strength = EvaluateExpression(&Motion->Point.Strength, Particle, t, tSystem);
            ov3 Direction = Motion->Point.Position - Particle->P;
            Result += Strength * ObNormalize(Direction);
        } break;

        case OFFBEAT_MotionNone:
        default:
        {
        } break;
    }

    return Result;
}

static void
OffbeatUpdateParticleSystem(ob_particle* Particles, ob_particle_system* ParticleSystem, f32 dt, ov3* CameraPosition)
{
    ob_motion* Motion = &ParticleSystem->Motion;

    u32 Index = ParticleSystem->HistoryEntryCount - 1;
    u32 ParticleSpawnCount = ParticleSystem->History[Index].ParticlesEmitted;
    // TODO(rytis): Remove this.
    f32 t = ParticleSystem->History[Index].TimeElapsed;
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

        // TODO(rytis): Find a better solution for expression evaluation with parameters from
        // higher scope. So far it's very sloppy.
        ov3 ddP = OffbeatUpdateParticleddP(Motion, UpdatedParticle, &t, &ParticleSystem->t);

        UpdatedParticle->P += 0.5f * ObSquare(dt) * ddP + dt * UpdatedParticle->dP;
        UpdatedParticle->dP += dt * ddP;
        UpdatedParticle->CameraDistance = ObLength(*CameraPosition - UpdatedParticle->P);
        ++UpdatedParticle;
    }

    ParticleSystem->Particles = Particles;
}

static void
OffbeatConstructQuad(ob_draw_list* DrawList, ob_quad_data* QuadData, ob_appearance* Appearance, ob_particle* Particle, f32* tSystem, f32* t)
{
    f32 Size = EvaluateExpression(&Appearance->Size, Particle, tSystem, t);
    // NOTE(rytis): Vertices (facing the camera plane)
    ov3 BottomLeft = Particle->P + 0.5f * Size * (QuadData->Horizontal - QuadData->Vertical);
    ov3 BottomRight = Particle->P + 0.5f * Size * (-QuadData->Horizontal - QuadData->Vertical);
    ov3 TopRight = Particle->P + 0.5f * Size * (-QuadData->Horizontal + QuadData->Vertical);
    ov3 TopLeft = Particle->P + 0.5f * Size * (QuadData->Horizontal + QuadData->Vertical);

    // NOTE(rytis): UVs
    ov2 BottomLeftUV = ov2{0.0f, 0.0f};
    ov2 BottomRightUV = ov2{1.0f, 0.0f};
    ov2 TopRightUV = ov2{1.0f, 1.0f};
    ov2 TopLeftUV = ov2{0.0f, 1.0f};

    ov4 Color = EvaluateExpression(&Appearance->Color, Particle, tSystem, t);

    u32 VertexIndex = DrawList->VertexCount;
    // NOTE(rytis): Updating draw list vertex array
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{BottomLeft, BottomLeftUV, Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{BottomRight, BottomRightUV, Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{TopRight, TopRightUV, Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{TopLeft, TopLeftUV, Color};

    // NOTE(rytis): Updating draw list index array
    // NOTE(rytis): CCW bottom right triangle
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 1;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 2;
    // NOTE(rytis): CCW top left triangle
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 2;
    DrawList->Indices[DrawList->IndexCount++] = VertexIndex + 3;

    ++DrawList->ElementCount;
}

static void
OffbeatRenderParticleSystem(ob_draw_list* DrawList, u32* IndexMemory, ob_draw_vertex* VertexMemory, ob_particle_system* ParticleSystem, ob_quad_data* QuadData, f32* t)
{
    ob_appearance* Appearance = &ParticleSystem->Appearance;

    *DrawList = {};
    DrawList->TextureID = Appearance->TextureID;
    DrawList->Indices = IndexMemory;
    DrawList->Vertices = VertexMemory;

    for(u32 ParticleIndex = 0; ParticleIndex < ParticleSystem->ParticleCount; ++ParticleIndex)
    {
        ob_particle* Particle = ParticleSystem->Particles + ParticleIndex;

        OffbeatConstructQuad(DrawList, QuadData, Appearance, Particle, &ParticleSystem->t, t);
    }
}

static void
OffbeatUpdateParticleCount(ob_history_entry* History, ob_particle_system* ParticleSystem, f32 t, f32 dt)
{
    ob_emission* Emission = &ParticleSystem->Emission;

    ParticleSystem->t += dt;
    ParticleSystem->tSpawn += dt;

    f32 TimePerParticle = Emission->EmissionRate > 0.0f ? 1.0f / Emission->EmissionRate : 0.0f;
    f32 RealParticleSpawn =  ParticleSystem->tSpawn * Emission->EmissionRate;
    OffbeatAssert(RealParticleSpawn >= 0.0f);

    u32 ParticleSpawnCount = TruncateF32ToU32(RealParticleSpawn);
    ParticleSystem->tSpawn -= ParticleSpawnCount * TimePerParticle;
    ParticleSystem->ParticleCount += ParticleSpawnCount;

    f32 MinTimeElapsed = ObClamp(0.0f, t - ParticleSystem->Emission.ParticleLifetime, t);
    u32 NewHistoryEntryCount = 0;
    for(u32 HistoryIndex = 0; HistoryIndex < ParticleSystem->HistoryEntryCount; ++HistoryIndex)
    {
        ob_history_entry* HistoryEntry = ParticleSystem->History + HistoryIndex;
        if(HistoryEntry->TimeElapsed < MinTimeElapsed)
        {
            ParticleSystem->ParticleCount -= HistoryEntry->ParticlesEmitted;
        }
        else
        {
            History[NewHistoryEntryCount++] = *HistoryEntry;
        }
    }

    History[NewHistoryEntryCount].TimeElapsed = t;
    History[NewHistoryEntryCount++].ParticlesEmitted = ParticleSpawnCount;

    ParticleSystem->HistoryEntryCount = NewHistoryEntryCount;
    ParticleSystem->History = History;
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
    u64 StartCycleCount = __rdtsc();
    {
        OffbeatState->dt = dt;
        OffbeatState->t += OffbeatState->dt;

        ob_quad_data QuadData = {};
        QuadData.Horizontal = -ObNormalize(Camera.Right);
        QuadData.Vertical = ObNormalize(ObCross(QuadData.Horizontal, -Camera.Forward));
        OffbeatState->QuadData = QuadData;
        OffbeatState->CameraPosition = Camera.Position;

        OffbeatUpdateMemoryManager(&OffbeatState->MemoryManager);
    }

    OffbeatDrawGrid(OffbeatState);

    ov4 PointColor = ov4{1.0f, 1.0f, 0.0f, 1.0f};
    OffbeatState->DrawData.DrawListCount = OffbeatState->ParticleSystemCount;
    for(u32 i = 0; i < OffbeatState->ParticleSystemCount; ++i)
    {
        ob_particle_system* ParticleSystem = &OffbeatState->ParticleSystems[i];

        OffbeatUpdateSystemRotations(ParticleSystem);

        ob_history_entry* History =
            (ob_history_entry*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                     (ParticleSystem->HistoryEntryCount + 1) *
                                                     sizeof(ob_history_entry));

        OffbeatUpdateParticleCount(History, ParticleSystem, OffbeatState->t, OffbeatState->dt);

        ob_particle* Particles =
            (ob_particle*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                ParticleSystem->ParticleCount *
                                                sizeof(ob_particle));
        OffbeatSpawnParticles(Particles, ParticleSystem, &OffbeatState->EffectsEntropy,
                              OffbeatState->dt, &OffbeatState->CameraPosition,
                              &OffbeatState->RunningParticleID);
        OffbeatUpdateParticleSystem(Particles, ParticleSystem, OffbeatState->dt, &OffbeatState->CameraPosition);

        u32* IndexMemory =
            (u32*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                        ParticleSystem->ParticleCount *
                                        sizeof(u32) * 6);
        ob_draw_vertex* VertexMemory =
            (ob_draw_vertex*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                   ParticleSystem->ParticleCount *
                                                   sizeof(ob_draw_vertex) * 4);
        OffbeatRenderParticleSystem(&OffbeatState->DrawData.DrawLists[i], IndexMemory, VertexMemory,
                                    ParticleSystem, &OffbeatState->QuadData, &OffbeatState->t);

        // NOTE(rytis): Motion primitive position render
        // ov3 Point = ParticleSystem->Motion.Point.Position = ov3{-3.0f * ObSin(0.25f * PI * OffbeatState->t), 1.0f, 1.0f};
        if(ParticleSystem->Motion.Primitive == OFFBEAT_MotionPoint)
        {
            Debug::PushWireframeSphere(OV3ToVec3(ParticleSystem->Motion.Point.Position), 0.05f, OV4ToVec4(PointColor));
        }
    }

    OffbeatState->CycleCount = __rdtsc() - StartCycleCount;
}

ob_draw_data*
OffbeatGetDrawData(ob_state* OffbeatState)
{
    return &OffbeatState->DrawData;
}
