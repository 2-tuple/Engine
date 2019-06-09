// TODO(rytis): Remove this.
#include "offbeat.h"

// Windows perf counter header
#if defined(__WIN32__) || defined(_WIN32) || defined(__WIN64__) || defined(_WIN64) || defined(WIN32)
#include <intrin.h>
// Linux perf counter header
#elif defined(__linux__) || defined(LINUX)
#include <x86intrin.h>
#define __rdtsc _rdtsc
#else
#error "ERROR: No intrinsics library available!"
#endif

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

extern ob_state* OffbeatState;

#ifdef OFFBEAT_OPENGL
#include "offbeat_opengl.h"
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

static void
OffbeatGenerateGridRGBATexture(void* Memory, u64 MemorySize, u32 Width, u32 Height, u32 SquareCount)
{
    OffbeatAssert(MemorySize >= (Width * Height * sizeof(u32)));

    u32 Light = 0xFF888888;
    u32 Dark = 0xFF777777;

    u32 XSquares = ((f32)Width + 0.5f) / SquareCount;
    u32 YSquares = ((f32)Height + 0.5f) / SquareCount;

    u32* Pixel = (u32*)Memory;
    for(u32 i = 0; i < Height; ++i)
    {
        b32 YLight = ((i / YSquares) % 2) == 0;
        for(u32 j = 0; j < Width; ++j)
        {
            b32 XLight = ((j / XSquares) % 2) == 0;
            if(YLight ^ XLight)
            {
                *Pixel++ = Light;
            }
            else
            {
                *Pixel++ = Dark;
            }
        }
    }
}

void
OffbeatAddTexture(u32 Texture)
{
    OffbeatAssert((OffbeatState->TextureCount + 1) < OFFBEAT_PARTICLE_SYSTEM_COUNT);
    OffbeatAssert(!OffbeatState->TexturesLoaded);
    OffbeatState->Textures[OffbeatState->TextureCount++] = Texture;
}

ob_file_data
OffbeatPackParticleSystemBytes(u32 ParticleSystemIndex)
{
    OffbeatAssert(ParticleSystemIndex < OFFBEAT_PARTICLE_SYSTEM_COUNT);
    ob_file_data Result = {};

    ob_particle_system* ParticleSystem = &OffbeatState->ParticleSystems[ParticleSystemIndex];
    Result.Size = sizeof(u32) * 5 + sizeof(ob_emission) + sizeof(ob_motion) + sizeof(ob_appearance);
    Result.Data = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Result.Size);

    u32* U32Memory = (u32*)Result.Data;
    *U32Memory++ = 0x0F;
    *U32Memory++ = 0xFB;
    *U32Memory++ = 0xEA;
    *U32Memory++ = 0x70;
    *U32Memory++ = (u32)ParticleSystem->UseGPU;

    ob_emission* EmissionMemory = (ob_emission*)U32Memory;
    *EmissionMemory++ = ParticleSystem->Emission;

    ob_motion* MotionMemory = (ob_motion*)EmissionMemory;
    *MotionMemory++ = ParticleSystem->Motion;

    ob_appearance* AppearanceMemory = (ob_appearance*)MotionMemory;
    *AppearanceMemory++ = ParticleSystem->Appearance;

    OffbeatAssert((((u8*)Result.Data) + Result.Size) == ((u8*)AppearanceMemory));
    return Result;
}

ob_file_data
OffbeatPackCurrentParticleSystemBytes()
{
    return OffbeatPackParticleSystemBytes(OffbeatState->CurrentParticleSystem);
}

b32
OffbeatUnpackParticleSystem(ob_particle_system* Result, void* PackedBytes)
{
    u32* U32Memory = (u32*)PackedBytes;
    if((U32Memory[0] != 0x0F) &&
       (U32Memory[1] != 0xFB) &&
       (U32Memory[2] != 0xEA) &&
       (U32Memory[3] != 0x70))
    {
        return false;
    }

    Result->UseGPU = (b32)U32Memory[4];

    U32Memory += 5;

    ob_emission* EmissionMemory = (ob_emission*)U32Memory;
    Result->Emission = *EmissionMemory;
    ++EmissionMemory;

    ob_motion* MotionMemory = (ob_motion*)EmissionMemory;
    Result->Motion = *MotionMemory;
    ++MotionMemory;

    ob_appearance* AppearanceMemory = (ob_appearance*)MotionMemory;
    Result->Appearance = *AppearanceMemory;
    ++AppearanceMemory;

    // TODO(rytis): Avoid this somehow???
#ifdef OFFBEAT_OPENGL_COMPUTE
    glGenBuffers(1, &Result->ParticleSSBO);
    glGenBuffers(1, &Result->OldParticleSSBO);
#endif

    return true;
}

ob_packed_particle_system
OffbeatPackParticleSystemStruct(u32 ParticleSystemIndex)
{
    OffbeatAssert(ParticleSystemIndex < OFFBEAT_PARTICLE_SYSTEM_COUNT);
    ob_packed_particle_system Result = {};

    ob_particle_system* ParticleSystem = &OffbeatState->ParticleSystems[ParticleSystemIndex];

    Result.Header[0] = 0x0F;
    Result.Header[1] = 0xFB;
    Result.Header[2] = 0xEA;
    Result.Header[3] = 0x70;
    Result.UseGPU = ParticleSystem->UseGPU;
    Result.Emission = ParticleSystem->Emission;
    Result.Motion = ParticleSystem->Motion;
    Result.Appearance = ParticleSystem->Appearance;

    return Result;
}

ob_packed_particle_system
OffbeatPackCurrentParticleSystemStruct()
{
    return OffbeatPackParticleSystemStruct(OffbeatState->CurrentParticleSystem);
}

b32
OffbeatUnpackParticleSystem(ob_particle_system* Result, ob_packed_particle_system* PackedStruct)
{
    if((PackedStruct->Header[0] != 0x0F) &&
       (PackedStruct->Header[1] != 0xFB) &&
       (PackedStruct->Header[2] != 0xEA) &&
       (PackedStruct->Header[3] != 0x70))
    {
        return false;
    }

    Result->UseGPU = PackedStruct->UseGPU;
    Result->Emission = PackedStruct->Emission;
    Result->Motion = PackedStruct->Motion;
    Result->Appearance = PackedStruct->Appearance;

    // TODO(rytis): Avoid this somehow???
#ifdef OFFBEAT_OPENGL_COMPUTE
    glGenBuffers(1, &Result->ParticleSSBO);
    glGenBuffers(1, &Result->OldParticleSSBO);
#endif

    return true;
}

static f32
EvaluateParameter(ob_parameter Parameter, ob_particle* Particle)
{
    f32 Result = 0.0f;

    if(!Particle)
    {
        return Result;
    }

    switch(Parameter)
    {
        case OFFBEAT_ParameterAge:
        {
            Result = Particle->Age;
        } break;

        case OFFBEAT_ParameterVelocity:
        {
            Result = ObLength(Particle->dP);
        } break;

        case OFFBEAT_ParameterID:
        {
            Result = Particle->ID;
        } break;

        case OFFBEAT_ParameterRandom:
        {
            Result = Particle->Random;
        } break;

        // NOTE(rytis): No collision count when using CPU.
        case OFFBEAT_ParameterCollisionCount:
        {
            Result = 0;
        } break;

        case OFFBEAT_ParameterCameraDistance:
        {
            Result = ObLength(Particle->P - OffbeatState->CameraPosition);
        } break;

        default:
        {
        } break;
    }

    return Result;
}

static ov4
OffbeatEvaluateExpression(ob_expr* Expr, ob_particle* Particle = 0)
{
    if(Expr->Function == OFFBEAT_FunctionPeriodicTime)
    {
        return ObLerp(Expr->Low,
                      0.5f * (ObSin(2.0f * PI * Expr->Float * OffbeatState->t) + 1.0f),
                      Expr->High);
    }
    else if(Expr->Function == OFFBEAT_FunctionPeriodicSquareTime)
    {
        if(((s32)(Expr->Float * OffbeatState->t)) % 2)
        {
            return Expr->High;
        }
        else
        {
            return Expr->Low;
        }
    }

    f32 Param = EvaluateParameter(Expr->Parameter, Particle);
    switch(Expr->Function)
    {
        case OFFBEAT_FunctionConst:
        {
            return Expr->High;
        } break;

        case OFFBEAT_FunctionLerp:
        {
        } break;

        case OFFBEAT_FunctionMaxLerp:
        {
            Param = ObClamp01(Param / Expr->Float);
        } break;

        case OFFBEAT_FunctionTriangle:
        {
            Param = ObClamp01(Param);
            Param = 1.0f - ObAbsoluteValue(2.0f * Param - 1.0f);
        } break;

        case OFFBEAT_FunctionTwoTriangles:
        {
            Param = ObClamp01(Param);
            Param = 1.0f - ObAbsoluteValue(2.0f * Param - 1.0f);
            Param = 1.0f - ObAbsoluteValue(2.0f * Param - 1.0f);
        } break;

        case OFFBEAT_FunctionFourTriangles:
        {
            Param = ObClamp01(Param);
            Param = 1.0f - ObAbsoluteValue(2.0f * Param - 1.0f);
            Param = 1.0f - ObAbsoluteValue(2.0f * Param - 1.0f);
            Param = 1.0f - ObAbsoluteValue(2.0f * Param - 1.0f);
        } break;

        case OFFBEAT_FunctionStep:
        {
            f32 MaxValue = Expr->Float;
            u32 StepCount = Expr->Uint ? Expr->Uint : 1;
            f32 StepValue = (f32)StepCount / MaxValue;
            Param = ObClamp01(((f32)((u32)(Param * StepValue)) / StepValue) / MaxValue);
        } break;

        case OFFBEAT_FunctionPeriodic:
        {
            return ObLerp(Expr->Low,
                          0.5f * (ObSin(2.0f * PI * Expr->Float * Param) + 1.0f),
                          Expr->High);
        } break;

        case OFFBEAT_FunctionPeriodicSquare:
        {
            if(((s32)(Expr->Float * Param)) % 2)
            {
                return Expr->High;
            }
            else
            {
                return Expr->Low;
            }
        } break;

        default:
        {
            return ov4{};
        } break;
    }
    return ObLerp(Expr->Low, Param, Expr->High);
}

// NOTE(rytis): Debug stuff.
#ifdef OFFBEAT_DEBUG
#define OffbeatDebugSpawnPoint(Point) OffbeatDebugSpawnPoint_(Point)
#define OffbeatDebugMotionPrimitive(Motion) OffbeatDebugMotionPrimitive_(Motion)

static void
OffbeatAddDebugQuad(ob_draw_list_debug* DrawList, ov3 Point, ov3 Horizontal, ov3 Vertical, f32 Width, f32 Height, ov4 Color)
{
    ov3 BottomLeft = Point + 0.5f * (-(Width * Horizontal) - (Height * Vertical));
    ov3 BottomRight = Point + 0.5f * ((Width * Horizontal) - (Height * Vertical));
    ov3 TopRight = Point + 0.5f * ((Width * Horizontal) + (Height * Vertical));
    ov3 TopLeft = Point + 0.5f * (-(Width * Horizontal) + (Height * Vertical));

    // NOTE(rytis): UVs
#if 0
    ov2 BottomLeftUV = ov2{0.0f, 0.0f};
    ov2 BottomRightUV = ov2{1.0f, 0.0f};
    ov2 TopRightUV = ov2{1.0f, 1.0f};
    ov2 TopLeftUV = ov2{0.0f, 1.0f};
#else
    // NOTE(rytis): Flipped
    ov2 BottomLeftUV = ov2{0.0f, 1.0f};
    ov2 BottomRightUV = ov2{1.0f, 1.0f};
    ov2 TopRightUV = ov2{1.0f, 0.0f};
    ov2 TopLeftUV = ov2{0.0f, 0.0f};
#endif

    u32 VertexIndex = DrawList->VertexCount;
    // NOTE(rytis): Updating draw list vertex array
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{BottomLeft,
                                                                 OffbeatState->TextureCount + 1,
                                                                 BottomLeftUV, Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{BottomRight,
                                                                 OffbeatState->TextureCount + 1,
                                                                 BottomRightUV, Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{TopRight,
                                                                 OffbeatState->TextureCount + 1,
                                                                 TopRightUV, Color};
    DrawList->Vertices[DrawList->VertexCount++] = ob_draw_vertex{TopLeft,
                                                                 OffbeatState->TextureCount + 1,
                                                                 TopLeftUV, Color};

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
OffbeatDebugSpawnPoint_(ov3 Point)
{
    ob_draw_list_debug* DrawList = &OffbeatState->DebugDrawData.DrawLists[OffbeatState->DebugCurrentParticleSystem];
    ob_quad_data* QuadData = &OffbeatState->QuadData;
    f32 Size = 0.05f;
    ov4 Color = ov4{1.0f, 0.0f, 0.0f, 1.0f};
    OffbeatAddDebugQuad(DrawList, Point, QuadData->Horizontal, QuadData->Vertical, Size, Size, Color);
}

static void
OffbeatDebugMotionPrimitive_(ob_motion* Motion)
{
    ob_draw_list_debug* DrawList = &OffbeatState->DebugDrawData.DrawLists[OffbeatState->DebugCurrentParticleSystem];
    ov3 Point;
    ov3 Horizontal;
    ov3 Vertical;
    f32 Width;
    f32 Height;
    ov4 Color = ov4{1.0f, 1.0f, 0.0f, 1.0f};

    switch(Motion->Primitive)
    {
        case OFFBEAT_MotionPoint:
        {
            Point = OffbeatEvaluateExpression(&Motion->Position).xyz;
            Horizontal = OffbeatState->QuadData.Horizontal;
            Vertical = OffbeatState->QuadData.Vertical;
            Width = 0.05f;
            Height = 0.05f;
            OffbeatAddDebugQuad(DrawList, Point, Horizontal, Vertical, Width, Height, Color);
        } break;

        case OFFBEAT_MotionLine:
        {
            Point = OffbeatEvaluateExpression(&Motion->Position).xyz;
            ov3 Normal = ObNormalize(ObCross(OffbeatState->QuadData.Horizontal, OffbeatState->QuadData.Vertical));
            Vertical = ObNormalize(OffbeatEvaluateExpression(&Motion->LineDirection).xyz);
            Horizontal = ObNormalize(ObCross(Vertical, Normal));
            Width = 0.03f;
            Height = 100.0f;
            OffbeatAddDebugQuad(DrawList, Point, Horizontal, Vertical, Width, Height, Color);
        } break;

        case OFFBEAT_MotionSphere:
        {
            Point = OffbeatEvaluateExpression(&Motion->Position).xyz;
            f32 Radius = OffbeatEvaluateExpression(&Motion->SphereRadius).x;
            Horizontal = OffbeatState->QuadData.Horizontal;
            Vertical = OffbeatState->QuadData.Vertical;
            Width = 0.05f;
            Height = 0.05f;
            OffbeatAddDebugQuad(DrawList, Point, Horizontal, Vertical, Width, Height, Color);
            OffbeatAddDebugQuad(DrawList, Point + Vertical * Radius, Horizontal, Vertical,
                                Width, Height, Color);
            OffbeatAddDebugQuad(DrawList, Point - Vertical * Radius, Horizontal, Vertical,
                                Width, Height, Color);
            OffbeatAddDebugQuad(DrawList, Point + Horizontal * Radius, Horizontal, Vertical,
                                Width, Height, Color);
            OffbeatAddDebugQuad(DrawList, Point - Horizontal * Radius, Horizontal, Vertical,
                                Width, Height, Color);
        } break;

        default:
        {
            Horizontal = ov3{};
            Vertical = ov3{};
            Width = 0.0f;
            Height = 0.0f;
        } break;
    }
}

ob_draw_data_debug*
OffbeatGetDebugDrawData()
{
    return &OffbeatState->DebugDrawData;
}

#else
#define OffbeatDebugSpawnPoint(Point)
#define OffbeatDebugMotionPrimitive(Motion)
#endif

ob_state*
OffbeatSetupMemory(void* Memory, u64 MemorySize)
{
    u64 StateSize = sizeof(ob_state);
    OffbeatAssert(MemorySize >= StateSize);
    ObZeroMemory(Memory, MemorySize);

    OffbeatState = (ob_state*)Memory;

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

    return OffbeatState;
}

// TODO(rytis): Remove the tests.
#include "offbeat_tests.h"

void
OffbeatInit()
{
    // OffbeatRunTests();
    // NOTE(rytis): Additional texture generation.
    {
        u32 Width = 1000;
        u32 Height = 1000;
        u32 SquareCount = 50;
        u64 Size = Width * Height * sizeof(u32);

        void* TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateGridRGBATexture(TextureMemory, Size, Width, Height, SquareCount);
        OffbeatState->Textures[OffbeatState->TextureCount] = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);

        TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateSquareRGBATexture(TextureMemory, Size, Width, Height);
        OffbeatState->Textures[OffbeatState->TextureCount + 1] = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);

        OffbeatState->AdditionalTextureCount = 2;
    }

    // NOTE(rytis): Offbeat bindless texture init.
    OffbeatInitBindlessTextures();

    // NOTE(rytis): Offbeat grid init.
    {
        ov3 Z = ov3{0.0f, 0.0f, 1.0f};
        om3 Rotation = ObRotationAlign(Z, ov3{0.0f, 1.0f, 0.0f});

        f32 Scale = 10.0f;

        ov3 BottomLeft = Rotation * (Scale * ov3{-1.0f, -1.0f, 0.0f});
        ov3 BottomRight = Rotation * (Scale * ov3{1.0f, -1.0f, 0.0f});
        ov3 TopRight = Rotation * (Scale * ov3{1.0f, 1.0f, 0.0f});
        ov3 TopLeft = Rotation * (Scale * ov3{-1.0f, 1.0f, 0.0f});

        ov2 BottomLeftUV = ov2{0.0f, 0.0f};
        ov2 BottomRightUV = ov2{1.0f, 0.0f};
        ov2 TopRightUV = ov2{1.0f, 1.0f};
        ov2 TopLeftUV = ov2{0.0f, 1.0f};

        ov4 Color = ov4{1.0f, 1.0f, 1.0f, 1.0f};

        // NOTE(rytis): Top side.
        OffbeatState->GridIndices[0] = 0;
        OffbeatState->GridIndices[1] = 1;
        OffbeatState->GridIndices[2] = 2;
        OffbeatState->GridIndices[3] = 0;
        OffbeatState->GridIndices[4] = 2;
        OffbeatState->GridIndices[5] = 3;

        // NOTE(rytis): Bottom side.
        OffbeatState->GridIndices[6] = 0;
        OffbeatState->GridIndices[7] = 2;
        OffbeatState->GridIndices[8] = 1;
        OffbeatState->GridIndices[9] = 0;
        OffbeatState->GridIndices[10] = 3;
        OffbeatState->GridIndices[11] = 2;

        OffbeatState->GridVertices[0] = ob_draw_vertex{BottomLeft, OffbeatState->TextureCount,
                                                       BottomLeftUV, Color};
        OffbeatState->GridVertices[1] = ob_draw_vertex{BottomRight, OffbeatState->TextureCount,
                                                       BottomRightUV, Color};
        OffbeatState->GridVertices[2] = ob_draw_vertex{TopRight, OffbeatState->TextureCount,
                                                       TopRightUV, Color};
        OffbeatState->GridVertices[3] = ob_draw_vertex{TopLeft, OffbeatState->TextureCount,
                                                       TopLeftUV, Color};
    }

    // NOTE(rytis): OffbeatState init.
    {
        OffbeatState->t = 0.0f;

#if !(defined(__linux__) || defined(LINUX))
        OffbeatState->RenderProgramID = OffbeatCreateRenderProgram();
#endif
        OffbeatState->EffectsEntropy = ObRandomSeed(1234);
    }

#ifdef OFFBEAT_OPENGL_COMPUTE
    OffbeatCreateComputePrograms(&OffbeatState->SpawnProgramID, &OffbeatState->UpdateProgramID,
                                 &OffbeatState->StatelessEvaluationProgramID);
    OffbeatComputeInit();
#endif
}

static void
OffbeatInitParticleSystem(ob_particle_system* ParticleSystem)
{
    *ParticleSystem = {};
    ParticleSystem->Emission.EmissionRate.High.x = 60.0f;
    ParticleSystem->Emission.InitialVelocityScale.High.x = 1.0f;
    ParticleSystem->Emission.ParticleLifetime.High.x = 1.0f;
    ParticleSystem->Emission.VelocityType = OFFBEAT_VelocityCone;
    ParticleSystem->Emission.ConeHeight.High.x = 20.0f;
    ParticleSystem->Emission.ConeRadius.High.x = 10.0f;
    ParticleSystem->Emission.ConeDirection.High.xyz = ov3{0.0f, 1.0f, 0.0f};
    ParticleSystem->Appearance.Color.High = ov4{1.0f, 1.0f, 1.0f, 1.0f};
    ParticleSystem->Appearance.Size.High.x = 0.05f;

#ifdef OFFBEAT_OPENGL_COMPUTE
    glGenBuffers(1, &ParticleSystem->ParticleSSBO);
    glGenBuffers(1, &ParticleSystem->OldParticleSSBO);
#endif
}

static void
OffbeatInitDrawList(ob_draw_list* DrawList)
{
    *DrawList = {};

#ifdef OFFBEAT_OPENGL_COMPUTE
    glGenVertexArrays(1, &DrawList->VAO);
    glGenBuffers(1, &DrawList->VBO);
    glGenBuffers(1, &DrawList->EBO);

    glBindVertexArray(DrawList->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, DrawList->VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, DrawList->EBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex_aligned), (GLvoid*)(OffbeatOffsetOf(ob_draw_vertex_aligned, Position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex_aligned), (GLvoid*)(OffbeatOffsetOf(ob_draw_vertex_aligned, TextureIndex)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex_aligned), (GLvoid*)(OffbeatOffsetOf(ob_draw_vertex_aligned, UV)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex_aligned), (GLvoid*)(OffbeatOffsetOf(ob_draw_vertex_aligned, Color)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

#ifdef OFFBEAT_OPENGL_COMPUTE
static void
OffbeatCleanupParticleSystem(ob_particle_system* ParticleSystem)
{
    glDeleteBuffers(1, &ParticleSystem->ParticleSSBO);
    glDeleteBuffers(1, &ParticleSystem->OldParticleSSBO);
}

static void
OffbeatCleanupDrawList(ob_draw_list* DrawList)
{
    glDeleteBuffers(1, &DrawList->VBO);
    glDeleteBuffers(1, &DrawList->EBO);
    glDeleteVertexArrays(1, &DrawList->VAO);
}
#else
#define OffbeatCleanupParticleSystem(ParticleSystem)
#define OffbeatCleanupDrawList(DrawList)
#endif

ob_particle_system*
OffbeatNewParticleSystem(u32* Index)
{
    OffbeatAssert(OffbeatState->ParticleSystemCount + 1 <= OFFBEAT_PARTICLE_SYSTEM_COUNT);
    if(Index)
    {
        *Index = OffbeatState->ParticleSystemCount;
    }
    OffbeatState->CurrentParticleSystem = OffbeatState->ParticleSystemCount;
    ob_particle_system* Result = &OffbeatState->ParticleSystems[OffbeatState->ParticleSystemCount++];
    OffbeatInitParticleSystem(Result);
    ob_draw_list* NewDrawList = &OffbeatState->DrawData.DrawLists[OffbeatState->DrawData.DrawListCount++];
    OffbeatInitDrawList(NewDrawList);
    return Result;
}

void
OffbeatAddParticleSystem(ob_particle_system* NewParticleSystem)
{
    OffbeatAssert(OffbeatState->ParticleSystemCount + 1 <= OFFBEAT_PARTICLE_SYSTEM_COUNT);
    OffbeatState->CurrentParticleSystem = OffbeatState->ParticleSystemCount;
    OffbeatState->ParticleSystems[OffbeatState->ParticleSystemCount] = {};
    OffbeatState->ParticleSystems[OffbeatState->ParticleSystemCount++] = *NewParticleSystem;

    ob_draw_list NewDrawList;
    OffbeatInitDrawList(&NewDrawList);
    OffbeatState->DrawData.DrawLists[OffbeatState->DrawData.DrawListCount++] = NewDrawList;
}

void
OffbeatRemoveParticleSystem(u32 Index)
{
    if(OffbeatState->ParticleSystemCount == 0)
    {
        OffbeatState->CurrentParticleSystem = 0;
        return;
    }
    OffbeatCleanupParticleSystem(&OffbeatState->ParticleSystems[Index]);
    OffbeatCleanupDrawList(&OffbeatState->DrawData.DrawLists[Index]);
    for(u32 i = Index; i < OffbeatState->ParticleSystemCount - 1; ++i)
    {
        OffbeatState->ParticleSystems[i] = OffbeatState->ParticleSystems[i + 1];
        OffbeatState->DrawData.DrawLists[i] = OffbeatState->DrawData.DrawLists[i + 1];
    }
    --OffbeatState->ParticleSystemCount;
    --OffbeatState->DrawData.DrawListCount;
    if(OffbeatState->CurrentParticleSystem >= OffbeatState->ParticleSystemCount)
    {
        if(OffbeatState->ParticleSystemCount == 0)
        {
            OffbeatState->CurrentParticleSystem = 0;
        }
        else
        {
            --OffbeatState->CurrentParticleSystem;
        }
    }
}

void
OffbeatRemoveCurrentParticleSystem()
{
    OffbeatRemoveParticleSystem(OffbeatState->CurrentParticleSystem);
}

void
OffbeatRemoveAllParticleSystems()
{
    for(u32 i = 0; i < OffbeatState->ParticleSystemCount; ++i)
    {
        OffbeatCleanupParticleSystem(&OffbeatState->ParticleSystems[i]);
        OffbeatCleanupDrawList(&OffbeatState->DrawData.DrawLists[i]);
    }
    OffbeatState->ParticleSystemCount = 0;
    OffbeatState->DrawData.DrawListCount = 0;
    OffbeatState->CurrentParticleSystem = 0;
}

ob_particle_system*
OffbeatGetCurrentParticleSystem()
{
    // TODO(rytis): Return 0 if there are no particle systems?
    return &OffbeatState->ParticleSystems[OffbeatState->CurrentParticleSystem];
}

ob_particle_system*
OffbeatPreviousParticleSystem()
{
    if(OffbeatState->CurrentParticleSystem > 0)
    {
        --OffbeatState->CurrentParticleSystem;
    }
    return OffbeatGetCurrentParticleSystem();
}

ob_particle_system*
OffbeatNextParticleSystem()
{
    if((OffbeatState->CurrentParticleSystem + 1) < OffbeatState->ParticleSystemCount)
    {
        ++OffbeatState->CurrentParticleSystem;
    }
    return OffbeatGetCurrentParticleSystem();
}

void
OffbeatToggleGPU(ob_particle_system* ParticleSystem)
{
    ParticleSystem->UseGPU = !ParticleSystem->UseGPU;
    ParticleSystem->ParticleCount = 0;
}

static void
OffbeatUpdateSystemRotationsAndNormalize(ob_particle_system* ParticleSystem)
{
    ob_emission* Emission = &ParticleSystem->Emission;
    ob_motion* Motion = &ParticleSystem->Motion;
    ov3 Z = ov3{0.0f, 0.0f, 1.0f};

    switch(Emission->Shape)
    {
        case OFFBEAT_EmissionRing:
        case OFFBEAT_EmissionDisk:
        case OFFBEAT_EmissionSquare:
        case OFFBEAT_EmissionCubeVolume:
        {
            Emission->EmissionNormal.Low.xyz = ObNOZ(Emission->EmissionNormal.Low.xyz);
            Emission->EmissionNormal.High.xyz = ObNOZ(Emission->EmissionNormal.High.xyz);
            ov3 Normal = OffbeatEvaluateExpression(&Emission->EmissionNormal).xyz;
            Emission->EmissionRotation = ObRotationAlign(Z, Normal);
        } break;
    }

    switch(Emission->VelocityType)
    {
        case OFFBEAT_VelocityCone:
        {
            Emission->ConeDirection.Low.xyz = ObNOZ(Emission->ConeDirection.Low.xyz);
            Emission->ConeDirection.High.xyz = ObNOZ(Emission->ConeDirection.High.xyz);
            ov3 ConeDirection = OffbeatEvaluateExpression(&Emission->ConeDirection).xyz;
            Emission->ConeRotation = ObRotationAlign(Z, ConeDirection);
        } break;
    }

    switch(Motion->Primitive)
    {
        case OFFBEAT_MotionLine:
        {
            Motion->LineDirection.Low.xyz = ObNOZ(Motion->LineDirection.Low.xyz);
            Motion->LineDirection.High.xyz = ObNOZ(Motion->LineDirection.High.xyz);
        } break;
    }
}

static void
OffbeatUpdateParticleCount(ob_history_entry* History, ob_particle_system* ParticleSystem, f32 t, f32 dt)
{
    ob_emission* Emission = &ParticleSystem->Emission;

    ParticleSystem->t += dt;
    ParticleSystem->tSpawn += dt;

    f32 EmissionRate = OffbeatEvaluateExpression(&Emission->EmissionRate).x;
    f32 TimePerParticle = EmissionRate > 0.0f ? 1.0f / EmissionRate : 0.0f;
    f32 RealParticleSpawn =  ParticleSystem->tSpawn * EmissionRate;
    OffbeatAssert(RealParticleSpawn >= 0.0f);

    u32 ParticleSpawnCount = TruncateF32ToU32(RealParticleSpawn);
    ParticleSystem->tSpawn -= ParticleSpawnCount * TimePerParticle;
    ParticleSystem->ParticleCount += ParticleSpawnCount;
    ParticleSystem->MaxParticleCount += ParticleSpawnCount;

    u32 NewHistoryEntryCount = 0;
    for(u32 HistoryIndex = 0; HistoryIndex < ParticleSystem->HistoryEntryCount; ++HistoryIndex)
    {
        ob_history_entry* HistoryEntry = ParticleSystem->History + HistoryIndex;
        HistoryEntry->MaxLifetime -= dt;
        if(HistoryEntry->MaxLifetime <= 0.0f)
        {
            ParticleSystem->MaxParticleCount -= HistoryEntry->ParticlesEmitted;
        }
        else
        {
            History[NewHistoryEntryCount++] = *HistoryEntry;
        }
    }

    f32 NewMaxLifetime = ObMax(ParticleSystem->Emission.ParticleLifetime.Low.x, ParticleSystem->Emission.ParticleLifetime.High.x) + dt;
    History[NewHistoryEntryCount].MaxLifetime = NewMaxLifetime;
    History[NewHistoryEntryCount].ParticlesEmitted = ParticleSpawnCount;
    ++NewHistoryEntryCount;

    ParticleSystem->ParticleSpawnCount = ParticleSpawnCount;
    ParticleSystem->HistoryEntryCount = NewHistoryEntryCount;
    ParticleSystem->History = History;
}

static ov3
OffbeatParticleInitialPosition(ob_random_series* Entropy, ob_emission* Emission, ob_particle* Particle)
{
    ov3 Result = {};

    switch(Emission->Shape)
    {
        case OFFBEAT_EmissionPoint:
        {
            Result = OffbeatEvaluateExpression(&Emission->Location, Particle).xyz;
        } break;

        case OFFBEAT_EmissionRing:
        {
            f32 Radius = OffbeatEvaluateExpression(&Emission->EmissionRadius, Particle).x;
            f32 RandomValue = 2.0f * PI * ObRandomUnilateral(Entropy);

            Result.x = Radius * ObSin(RandomValue);
            Result.y = Radius * ObCos(RandomValue);
            Result = Emission->EmissionRotation * Result;
            Result += OffbeatEvaluateExpression(&Emission->Location, Particle).xyz;
        } break;

        case OFFBEAT_EmissionDisk:
        {
            f32 Radius = OffbeatEvaluateExpression(&Emission->EmissionRadius, Particle).x;
            f32 RandomValue = 2.0f * PI * ObRandomUnilateral(Entropy);
            f32 Length = ObRandomUnilateral(Entropy);

            Result.x = Radius * Length * ObSin(RandomValue);
            Result.y = Radius * Length * ObCos(RandomValue);
            Result = Emission->EmissionRotation * Result;
            Result += OffbeatEvaluateExpression(&Emission->Location, Particle).xyz;
        } break;

        case OFFBEAT_EmissionSquare:
        {
            f32 Radius = OffbeatEvaluateExpression(&Emission->EmissionRadius, Particle).x;

            Result.x = Radius * ObRandomBilateral(Entropy);
            Result.y = Radius * ObRandomBilateral(Entropy);
            Result = Emission->EmissionRotation * Result;
            Result += OffbeatEvaluateExpression(&Emission->Location, Particle).xyz;
        } break;

        case OFFBEAT_EmissionSphere:
        {
            f32 Radius = OffbeatEvaluateExpression(&Emission->EmissionRadius, Particle).x;
            f32 Theta = ObRandomBetween(Entropy, 0.0f, 2.0f * PI);
            f32 Z = ObRandomBilateral(Entropy);

            Result.x = ObSquareRoot(1.0f - ObSquare(Z)) * ObCos(Theta);
            Result.y = ObSquareRoot(1.0f - ObSquare(Z)) * ObSin(Theta);
            Result.z = Z;
            Result *= Radius;
            Result += OffbeatEvaluateExpression(&Emission->Location, Particle).xyz;
        } break;

        case OFFBEAT_EmissionSphereVolume:
        {
            f32 Radius = OffbeatEvaluateExpression(&Emission->EmissionRadius, Particle).x;
            f32 Theta = ObRandomBetween(Entropy, 0.0f, 2.0f * PI);
            f32 Z = ObRandomBilateral(Entropy);
            f32 Length = ObRandomUnilateral(Entropy);

            Result.x = ObSquareRoot(1.0f - ObSquare(Z)) * ObCos(Theta);
            Result.y = ObSquareRoot(1.0f - ObSquare(Z)) * ObSin(Theta);
            Result.z = Z;
            Result *= Radius * Length;
            Result += OffbeatEvaluateExpression(&Emission->Location, Particle).xyz;
        } break;

        case OFFBEAT_EmissionCubeVolume:
        {
            f32 Radius = OffbeatEvaluateExpression(&Emission->EmissionRadius, Particle).x;
            Result.x = ObRandomBilateral(Entropy);
            Result.y = ObRandomBilateral(Entropy);
            Result.z = ObRandomBilateral(Entropy);
            Result *= Radius;
            Result = Emission->EmissionRotation * Result;
            Result += OffbeatEvaluateExpression(&Emission->Location, Particle).xyz;
        } break;

        default:
        {
        } break;
    }

    return Result;
}

static ov3
OffbeatParticleInitialVelocity(ob_random_series* Entropy, ob_emission* Emission, ob_particle* Particle)
{
    ov3 Result = {};

    switch(Emission->VelocityType)
    {
        case OFFBEAT_VelocityRandom:
        {
            f32 Theta = ObRandomBetween(Entropy, 0.0f, 2.0f * PI);
            f32 Z = ObRandomBilateral(Entropy);

            // NOTE(rytis): This generates a vector in a unit sphere, so no normalization is required.
            Result.x = ObSquareRoot(1.0f - ObSquare(Z)) * ObCos(Theta);
            Result.y = ObSquareRoot(1.0f - ObSquare(Z)) * ObSin(Theta);
            Result.z = Z;
        } break;

        case OFFBEAT_VelocityCone:
        {
            f32 ConeHeight = OffbeatEvaluateExpression(&Emission->ConeHeight, Particle).x;
            f32 ConeRadius = OffbeatEvaluateExpression(&Emission->ConeRadius, Particle).x;
            f32 Denom = ObSquareRoot(ObSquare(ConeHeight) + ObSquare(ConeRadius));
            f32 CosTheta = Denom > 0.0f ? ConeHeight / Denom : 0.0f;

            f32 Phi = ObRandomBetween(Entropy, 0.0f, 2.0f * PI);
            f32 Z = ObRandomBetween(Entropy, CosTheta, 1.0f);

            // NOTE(rytis): Vector generated around axis (0, 0, 1)
            ov3 RandomVector = {};
            RandomVector.x = ObSquareRoot(1.0f - ObSquare(Z)) * ObCos(Phi);
            RandomVector.y = ObSquareRoot(1.0f - ObSquare(Z)) * ObSin(Phi);
            RandomVector.z = Z;

            Result = Emission->ConeRotation * RandomVector;
        } break;

        default:
        {
        } break;
    }
    Result *= OffbeatEvaluateExpression(&Emission->InitialVelocityScale, Particle).x;

    return Result;
}

static void
OffbeatSpawnParticles(ob_particle* Particles, ob_particle_system* ParticleSystem, ob_random_series* Entropy, f32 dt, ov3* CameraPosition, u32* RunningParticleID)
{
    ob_emission* Emission = &ParticleSystem->Emission;

    for(u32 ParticleIndex = 0; ParticleIndex < ParticleSystem->ParticleSpawnCount; ++ParticleIndex)
    {
        ob_particle* Particle = Particles + ParticleIndex;

        Particle->Random = ObRandomUnilateral(Entropy);
        Particle->ID = (f32)(*RunningParticleID)++;

        Particle->P = OffbeatParticleInitialPosition(Entropy, Emission, Particle);
        Particle->dP = OffbeatParticleInitialVelocity(Entropy, Emission, Particle);

        Particle->Age = 0.0f;
        Particle->dAge = 1.0f / OffbeatEvaluateExpression(&Emission->ParticleLifetime, Particle).x;

        OffbeatDebugSpawnPoint(Particle->P);
    }
}

static ov3
OffbeatUpdateParticleddP(ob_motion* Motion, ob_particle* Particle)
{
    f32 LengthSq = ObLengthSq(Particle->dP);
    ov3 Drag = OffbeatEvaluateExpression(&Motion->Drag, Particle).x *
               LengthSq * ObNormalize(-Particle->dP);
    ov3 Result = OffbeatEvaluateExpression(&Motion->Gravity, Particle).xyz + Drag;
    f32 Strength = OffbeatEvaluateExpression(&Motion->Strength, Particle).x;
    ov3 Direction;
    switch(Motion->Primitive)
    {
        case OFFBEAT_MotionPoint:
        {
            Direction = OffbeatEvaluateExpression(&Motion->Position, Particle).xyz - Particle->P;
        } break;

        case OFFBEAT_MotionLine:
        {
            ov3 Position = OffbeatEvaluateExpression(&Motion->Position, Particle).xyz;
            ov3 LineDirection = OffbeatEvaluateExpression(&Motion->LineDirection, Particle).xyz;
            f32 t = ObInner(Particle->P - Position, LineDirection) /
                    ObLengthSq(LineDirection);
            Direction = (Position + t * LineDirection) - Particle->P;
        } break;

        case OFFBEAT_MotionSphere:
        {
            ov3 Position = OffbeatEvaluateExpression(&Motion->Position, Particle).xyz;
            f32 SphereRadius = OffbeatEvaluateExpression(&Motion->SphereRadius, Particle).x;
            Direction = Position + ObNOZ(Particle->P - Position) * SphereRadius - Particle->P;
        } break;

        case OFFBEAT_MotionNone:
        default:
        {
            Direction = ov3{0.0f, 0.0f, 0.0f};
        } break;
    }
    Result += Strength * ObNOZ(Direction);

    return Result;
}

static void
OffbeatUpdateParticleSystem(ob_particle* Particles, ob_particle_system* ParticleSystem, f32 dt, ov3* CameraPosition)
{
    ob_motion* Motion = &ParticleSystem->Motion;

    u32 OldParticleCount = ParticleSystem->ParticleCount - ParticleSystem->ParticleSpawnCount;

    ob_particle* UpdatedParticle = Particles + ParticleSystem->ParticleSpawnCount;
    for(u32 ParticleIndex = 0; ParticleIndex < OldParticleCount; ++ParticleIndex)
    {
        ob_particle* OldParticle = ParticleSystem->Particles + ParticleIndex;

        OldParticle->Age += dt * OldParticle->dAge;
        if(OldParticle->Age >= 1.0f)
        {
            --ParticleSystem->ParticleCount;
            continue;
        }

        *UpdatedParticle = *OldParticle;

        ov3 ddP = OffbeatUpdateParticleddP(Motion, UpdatedParticle);

        UpdatedParticle->P += 0.5f * ObSquare(dt) * ddP + dt * UpdatedParticle->dP;
        UpdatedParticle->dP += dt * ddP;
        ++UpdatedParticle;
    }

    OffbeatAssert(ParticleSystem->ParticleCount <= ParticleSystem->MaxParticleCount);

    ParticleSystem->Particles = Particles;
}

static void
OffbeatConstructQuad(ob_draw_list* DrawList, ob_quad_data* QuadData, ob_appearance* Appearance, ob_particle* Particle)
{
    f32 Size = OffbeatEvaluateExpression(&Appearance->Size, Particle).x;
    // NOTE(rytis): Vertices (facing the camera plane)
    ov3 BottomLeft = Particle->P + 0.5f * Size * (-QuadData->Horizontal - QuadData->Vertical);
    ov3 BottomRight = Particle->P + 0.5f * Size * (QuadData->Horizontal - QuadData->Vertical);
    ov3 TopRight = Particle->P + 0.5f * Size * (QuadData->Horizontal + QuadData->Vertical);
    ov3 TopLeft = Particle->P + 0.5f * Size * (-QuadData->Horizontal + QuadData->Vertical);

    // NOTE(rytis): UVs
#if 0
    ov2 BottomLeftUV = ov2{0.0f, 0.0f};
    ov2 BottomRightUV = ov2{1.0f, 0.0f};
    ov2 TopRightUV = ov2{1.0f, 1.0f};
    ov2 TopLeftUV = ov2{0.0f, 1.0f};
#else
    // NOTE(rytis): Flipped
    ov2 BottomLeftUV = ov2{0.0f, 1.0f};
    ov2 BottomRightUV = ov2{1.0f, 1.0f};
    ov2 TopRightUV = ov2{1.0f, 0.0f};
    ov2 TopLeftUV = ov2{0.0f, 0.0f};
#endif

    ov4 Color = OffbeatEvaluateExpression(&Appearance->Color, Particle);

    u32 VertexIndex0 = DrawList->VertexCount;
    u32 VertexIndex1 = DrawList->VertexCount + 1;
    u32 VertexIndex2 = DrawList->VertexCount + 2;
    u32 VertexIndex3 = DrawList->VertexCount + 3;
    // NOTE(rytis): Updating draw list vertex array
    DrawList->Vertices[VertexIndex0] = ob_draw_vertex{BottomLeft, Appearance->TextureIndex,
                                                      BottomLeftUV, Color};
    DrawList->Vertices[VertexIndex1] = ob_draw_vertex{BottomRight, Appearance->TextureIndex,
                                                      BottomRightUV, Color};
    DrawList->Vertices[VertexIndex2] = ob_draw_vertex{TopRight, Appearance->TextureIndex,
                                                      TopRightUV, Color};
    DrawList->Vertices[VertexIndex3] = ob_draw_vertex{TopLeft, Appearance->TextureIndex,
                                                      TopLeftUV, Color};
    DrawList->VertexCount += 4;

    // NOTE(rytis): Updating draw list index array
    u32 Index0 = DrawList->IndexCount;
    u32 Index1 = DrawList->IndexCount + 1;
    u32 Index2 = DrawList->IndexCount + 2;
    u32 Index3 = DrawList->IndexCount + 3;
    u32 Index4 = DrawList->IndexCount + 4;
    u32 Index5 = DrawList->IndexCount + 5;
    // NOTE(rytis): CCW bottom right triangle
    DrawList->Indices[Index0] = VertexIndex0;
    DrawList->Indices[Index1] = VertexIndex1;
    DrawList->Indices[Index2] = VertexIndex2;
    // NOTE(rytis): CCW top left triangle
    DrawList->Indices[Index3] = VertexIndex0;
    DrawList->Indices[Index4] = VertexIndex2;
    DrawList->Indices[Index5] = VertexIndex3;
    DrawList->IndexCount += 6;

    ++DrawList->ElementCount;
}

static void
OffbeatRenderParticleSystem(ob_draw_list* DrawList, u32* IndexMemory, ob_draw_vertex* VertexMemory, ob_particle_system* ParticleSystem, ob_quad_data* QuadData)
{
    ob_appearance* Appearance = &ParticleSystem->Appearance;

    DrawList->ElementCount = 0;
    DrawList->IndexCount = 0;
    DrawList->Indices = IndexMemory;
    DrawList->VertexCount = 0;
    DrawList->Vertices = VertexMemory;

    for(u32 ParticleIndex = 0; ParticleIndex < ParticleSystem->ParticleCount; ++ParticleIndex)
    {
        ob_particle* Particle = ParticleSystem->Particles + ParticleIndex;

        OffbeatConstructQuad(DrawList, QuadData, Appearance, Particle);
    }
}

void
OffbeatUpdateCamera(f32 Position[3], f32 Forward[3], f32 Right[3])
{
    ov3 P = ov3{Position[0], Position[1], Position[2]};
    ov3 R = ov3{Right[0], Right[1], Right[2]};
    ov3 F = ov3{Forward[0], Forward[1], Forward[2]};

    ob_quad_data QuadData = {};
    QuadData.Horizontal = ObNormalize(R);
    QuadData.Vertical = ObNormalize(ObCross(QuadData.Horizontal, F));

    OffbeatState->QuadData = QuadData;
    OffbeatState->CameraPosition = P;
}

void
OffbeatUpdateViewMatrix(f32 RowMajorMatrix[16])
{
    om4 Matrix = {};
    Matrix.E[0][0] = RowMajorMatrix[0];
    Matrix.E[0][1] = RowMajorMatrix[1];
    Matrix.E[0][2] = RowMajorMatrix[2];
    Matrix.E[0][3] = RowMajorMatrix[3];
    Matrix.E[1][0] = RowMajorMatrix[4];
    Matrix.E[1][1] = RowMajorMatrix[5];
    Matrix.E[1][2] = RowMajorMatrix[6];
    Matrix.E[1][3] = RowMajorMatrix[7];
    Matrix.E[2][0] = RowMajorMatrix[8];
    Matrix.E[2][1] = RowMajorMatrix[9];
    Matrix.E[2][2] = RowMajorMatrix[10];
    Matrix.E[2][3] = RowMajorMatrix[11];
    Matrix.E[3][0] = RowMajorMatrix[12];
    Matrix.E[3][1] = RowMajorMatrix[13];
    Matrix.E[3][2] = RowMajorMatrix[14];
    Matrix.E[3][3] = RowMajorMatrix[15];
    OffbeatState->ViewMatrix = Matrix;
}


void
OffbeatUpdateProjectionMatrix(f32 RowMajorMatrix[16])
{
    om4 Matrix = {};
    Matrix.E[0][0] = RowMajorMatrix[0];
    Matrix.E[0][1] = RowMajorMatrix[1];
    Matrix.E[0][2] = RowMajorMatrix[2];
    Matrix.E[0][3] = RowMajorMatrix[3];
    Matrix.E[1][0] = RowMajorMatrix[4];
    Matrix.E[1][1] = RowMajorMatrix[5];
    Matrix.E[1][2] = RowMajorMatrix[6];
    Matrix.E[1][3] = RowMajorMatrix[7];
    Matrix.E[2][0] = RowMajorMatrix[8];
    Matrix.E[2][1] = RowMajorMatrix[9];
    Matrix.E[2][2] = RowMajorMatrix[10];
    Matrix.E[2][3] = RowMajorMatrix[11];
    Matrix.E[3][0] = RowMajorMatrix[12];
    Matrix.E[3][1] = RowMajorMatrix[13];
    Matrix.E[3][2] = RowMajorMatrix[14];
    Matrix.E[3][3] = RowMajorMatrix[15];
    OffbeatState->ProjectionMatrix = Matrix;
}

void
OffbeatInitGeometryTextures(ob_texture ViewSpaceDepthMap, ob_texture ViewSpaceNormalMap)
{
    OffbeatAssert(glGetTextureHandleARB != 0);
    if(OffbeatState->GeometryTexturesLoaded)
    {
        return;
    }

    OffbeatState->DepthMapHandle = glGetTextureHandleARB(ViewSpaceDepthMap);
    glMakeTextureHandleResidentARB(OffbeatState->DepthMapHandle);

    OffbeatState->NormalMapHandle = glGetTextureHandleARB(ViewSpaceNormalMap);
    glMakeTextureHandleResidentARB(OffbeatState->NormalMapHandle);

    OffbeatState->GeometryTexturesLoaded = true;
}

void
OffbeatUpdateParticles(f32 dt)
{
    OffbeatUpdateMemoryManager(&OffbeatState->MemoryManager);

    OffbeatState->dt = dt;
    OffbeatState->t += OffbeatState->dt;

    OffbeatState->TotalParticleCount = 0;
#ifdef OFFBEAT_DEBUG
    OffbeatState->DebugDrawData.DrawListCount = OffbeatState->ParticleSystemCount;
#endif
    for(u32 i = 0; i < OffbeatState->ParticleSystemCount; ++i)
    {
        OffbeatState->DebugCurrentParticleSystem = i;
        ob_particle_system* ParticleSystem = &OffbeatState->ParticleSystems[i];
        ob_draw_list* DrawList = &OffbeatState->DrawData.DrawLists[i];
        DrawList->UseGPU = ParticleSystem->UseGPU;

        OffbeatUpdateSystemRotationsAndNormalize(ParticleSystem);

        ob_history_entry* History =
            (ob_history_entry*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                     (ParticleSystem->HistoryEntryCount + 1) *
                                                     sizeof(ob_history_entry));

        OffbeatUpdateParticleCount(History, ParticleSystem, OffbeatState->t, OffbeatState->dt);

#ifdef OFFBEAT_DEBUG
        OffbeatState->DebugDrawData.DrawLists[i] = {};
        OffbeatState->DebugDrawData.DrawLists[i].Indices =
            (u32*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                        (ParticleSystem->ParticleSpawnCount + 10) *
                                        sizeof(u32) * 6);
        OffbeatState->DebugDrawData.DrawLists[i].Vertices =
            (ob_draw_vertex*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                   (ParticleSystem->ParticleSpawnCount + 10) *
                                                   sizeof(ob_draw_vertex) * 4);
#endif

#ifdef OFFBEAT_OPENGL_COMPUTE
        if(ParticleSystem->UseGPU)
        {
            // TODO(rytis): Sorting is NECESSARY for GPU! Preferable for CPU, too.
            OffbeatComputeSwapBuffers(ParticleSystem);
            OffbeatComputeSpawnParticles(ParticleSystem);
            OffbeatComputeUpdateParticles(ParticleSystem);
            OffbeatComputeStatelessEvaluation(DrawList, ParticleSystem);
        }
        else
#endif
        {
            ob_particle* Particles =
                (ob_particle*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                    ParticleSystem->MaxParticleCount *
                                                    sizeof(ob_particle));
            OffbeatSpawnParticles(Particles, ParticleSystem, &OffbeatState->EffectsEntropy,
                                  OffbeatState->dt, &OffbeatState->CameraPosition,
                                  &OffbeatState->RunningParticleID);
            OffbeatUpdateParticleSystem(Particles, ParticleSystem, OffbeatState->dt,
                                        &OffbeatState->CameraPosition);

            u32* IndexMemory =
                (u32*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                            ParticleSystem->ParticleCount *
                                            sizeof(u32) * 6);
            ob_draw_vertex* VertexMemory =
                (ob_draw_vertex*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                       ParticleSystem->ParticleCount *
                                                       sizeof(ob_draw_vertex) * 4);
            OffbeatRenderParticleSystem(DrawList, IndexMemory, VertexMemory, ParticleSystem,
                                        &OffbeatState->QuadData);
        }

        OffbeatDebugMotionPrimitive(&ParticleSystem->Motion);
        OffbeatState->TotalParticleCount += ParticleSystem->ParticleCount;
    }
}

ob_draw_data*
OffbeatGetDrawData()
{
    return &OffbeatState->DrawData;
}
