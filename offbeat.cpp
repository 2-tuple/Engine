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

#ifdef OFFBEAT_OPENGL
#include "offbeat_opengl.h"
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
static ob_texture (*OffbeatGlobalRGBATextureID)(void*, u32, u32) = OffbeatRGBATextureID;

void
OffbeatSetAllocatorFunctions(void* (*Malloc)(u64), void (*Free)(void*))
{
    OffbeatGlobalAlloc = Malloc;
    OffbeatGlobalFree = Free;
}

void
OffbeatSetTextureFunction(ob_texture (*TextureFunction)(void*, u32, u32))
{
    OffbeatGlobalRGBATextureID = TextureFunction;
}

struct ob_global_data
{
    f32* t;
    ob_texture TextureIDs[OFFBEAT_TextureCount];
    umm ParameterOffsets[OFFBEAT_ParameterCount];
#ifdef OFFBEAT_DEBUG
    u32 CurrentParticleSystem;
    ob_quad_data DebugQuadData;
    ob_draw_data DebugDrawData;
#endif
};

static ob_global_data OffbeatGlobalData;

// NOTE(rytis): Debug stuff.
#ifdef OFFBEAT_DEBUG
#define OffbeatDebugSpawnPoint(Point) OffbeatDebugSpawnPoint_(Point)
#define OffbeatDebugMotionPrimitive(Motion) OffbeatDebugMotionPrimitive_(Motion)

static void
OffbeatDebugSpawnPoint_(ov3 Point)
{
    ob_draw_list* DrawList = &OffbeatGlobalData.DebugDrawData.DrawLists[OffbeatGlobalData.CurrentParticleSystem];
    ob_quad_data* QuadData = &OffbeatGlobalData.DebugQuadData;
    f32 Size = 0.05f;
    ov4 Color = ov4{1.0f, 0.0f, 0.0f, 1.0f};

    ov3 BottomLeft = Point + 0.5f * Size * (-QuadData->Horizontal - QuadData->Vertical);
    ov3 BottomRight = Point + 0.5f * Size * (QuadData->Horizontal - QuadData->Vertical);
    ov3 TopRight = Point + 0.5f * Size * (QuadData->Horizontal + QuadData->Vertical);
    ov3 TopLeft = Point + 0.5f * Size * (-QuadData->Horizontal + QuadData->Vertical);

    // NOTE(rytis): UVs
    ov2 BottomLeftUV = ov2{0.0f, 0.0f};
    ov2 BottomRightUV = ov2{1.0f, 0.0f};
    ov2 TopRightUV = ov2{1.0f, 1.0f};
    ov2 TopLeftUV = ov2{0.0f, 1.0f};

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
OffbeatDebugMotionPrimitive_(ob_motion* Motion)
{
    ob_draw_list* DrawList = &OffbeatGlobalData.DebugDrawData.DrawLists[OffbeatGlobalData.CurrentParticleSystem];
    ov3 Point;
    ov3 Horizontal;
    ov3 Vertical;
    f32 Width;
    f32 Height;
    switch(Motion->Primitive)
    {
        case OFFBEAT_MotionPoint:
        {
            Point = Motion->Point.Position;
            Horizontal = OffbeatGlobalData.DebugQuadData.Horizontal;
            Vertical = OffbeatGlobalData.DebugQuadData.Vertical;
            Width = 0.05f;
            Height = 0.05f;
        } break;

        case OFFBEAT_MotionLine:
        {
            Point = Motion->Line.Position;
            ov3 Normal = ObNormalize(ObCross(OffbeatGlobalData.DebugQuadData.Horizontal, OffbeatGlobalData.DebugQuadData.Vertical));
            Vertical = ObNormalize(Motion->Line.Direction);
            Horizontal = ObNormalize(ObCross(Vertical, Normal));
            Width = 0.03f;
            Height = 100.0f;
        } break;

        default:
        {
            Horizontal = ov3{};
            Vertical = ov3{};
            Width = 0.0f;
            Height = 0.0f;
        } break;
    }
    ov4 Color = ov4{1.0f, 1.0f, 0.0f, 1.0f};

    ov3 BottomLeft = Point + 0.5f * (-(Width * Horizontal) - (Height * Vertical));
    ov3 BottomRight = Point + 0.5f * ((Width * Horizontal) - (Height * Vertical));
    ov3 TopRight = Point + 0.5f * ((Width * Horizontal) + (Height * Vertical));
    ov3 TopLeft = Point + 0.5f * (-(Width * Horizontal) + (Height * Vertical));

    // NOTE(rytis): UVs
    ov2 BottomLeftUV = ov2{0.0f, 0.0f};
    ov2 BottomRightUV = ov2{1.0f, 0.0f};
    ov2 TopRightUV = ov2{1.0f, 1.0f};
    ov2 TopLeftUV = ov2{0.0f, 1.0f};

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

ob_draw_data*
OffbeatGetDebugDrawData()
{
    return &OffbeatGlobalData.DebugDrawData;
}

#else
#define OffbeatDebugSpawnPoint(Point)
#define OffbeatDebugMotionPrimitive(Motion)
#endif

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

void
OffbeatGenerateCrossRGBATexture(void* Memory, u64 MemorySize, u32 Width, u32 Height, u32 Thickness)
{
    OffbeatAssert(MemorySize >= (Width * Height * sizeof(u32)));
    OffbeatAssert((Thickness <= Height) && (Thickness <= Width));

    u32 CenterHeight = Height / 2;
    u32 CenterWidth = Width / 2;
    u32 HalfThickness = Thickness / 2;

    u32 FillStartHeight = CenterHeight - HalfThickness;
    u32 FillStartWidth = CenterWidth - HalfThickness;
    u32 FillEndHeight = FillStartHeight + Thickness;
    u32 FillEndWidth = FillStartWidth + Thickness;

    u32* Pixel = (u32*)Memory;
    for(s32 i = 0; i < Height; ++i)
    {
        for(s32 j = 0; j < Width; ++j)
        {
            if(((FillStartHeight <= i) && (i <= FillEndHeight)) ||
               ((FillStartWidth <= j) && (j <= FillEndWidth)))
            {
                *Pixel++ = 0xFFFFFFFF;
            }
            else
            {
                *Pixel++ = 0;
            }
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

ob_texture_type
OffbeatGetCurrentTextureType(ob_particle_system* ParticleSystem)
{
    for(u32 i = 0; i < OFFBEAT_TextureCount; ++i)
    {
        if(ParticleSystem->Appearance.TextureID == OffbeatGlobalData.TextureIDs[i])
        {
            return (ob_texture_type)i;
        }
    }
    return OFFBEAT_TextureCount;
}

void
OffbeatSetTextureID(ob_particle_system* ParticleSystem, ob_texture_type NewType)
{
    ParticleSystem->Appearance.TextureID = OffbeatGlobalData.TextureIDs[NewType];
}

static f32
EvaluateParameter(ob_parameter Parameter, ob_particle* Particle)
{
    f32 Result;

    switch(Parameter)
    {
        case OFFBEAT_ParameterGlobalTime:
        {
            Result = *OffbeatGlobalData.t;
        } break;

        case OFFBEAT_ParameterAge:
        case OFFBEAT_ParameterRandom:
        case OFFBEAT_ParameterCameraDistance:
        case OFFBEAT_ParameterID:
        {
            Result = *(f32*)((u8*)Particle + OffbeatGlobalData.ParameterOffsets[Parameter]);
        } break;

        default:
        {
            Result = 0.0f;
        } break;
    }

    return Result;
}

#define OffbeatEvaluateExpression_(type) type OffbeatEvaluateExpression(ob_expr_##type* Expr, ob_particle* Particle)\
{\
    f32 Param = EvaluateParameter(Expr->Parameter, Particle);\
    switch(Expr->Function)\
    {\
        case OFFBEAT_FunctionConst:\
        {\
            return Expr->High;\
        } break;\
\
        case OFFBEAT_FunctionLerp:\
        {\
            return ObLerp(Expr->Low, ObClamp01(Param), Expr->High);\
        } break;\
\
        case OFFBEAT_FunctionSin:\
        {\
            f32 Sin01 = 0.5f * (ObSin(2.0f * PI * Param) + 1.0f);\
            return ObLerp(Expr->Low, Sin01, Expr->High);\
        } break;\
\
        case OFFBEAT_FunctionCos:\
        {\
            f32 Cos01 = 0.5f * (ObCos(2.0f * PI * Param) + 1.0f);\
            return ObLerp(Expr->Low, Cos01, Expr->High);\
        } break;\
\
        case OFFBEAT_FunctionFloor:\
        {\
            f32 Floor = ObFloor(Param);\
            return ObLerp(Expr->Low, ObClamp01(Floor), Expr->High);\
        } break;\
\
        case OFFBEAT_FunctionRound:\
        {\
            f32 Round = ObRound(Param);\
            return ObLerp(Expr->Low, ObClamp01(Round), Expr->High);\
        } break;\
\
        case OFFBEAT_FunctionCeil:\
        {\
            f32 Ceil = ObCeil(Param);\
            return ObLerp(Expr->Low, ObClamp01(Ceil), Expr->High);\
        } break;\
\
        default:\
        {\
            return type{};\
        } break;\
    }\
}

static OffbeatEvaluateExpression_(f32);
static OffbeatEvaluateExpression_(ov3);
static OffbeatEvaluateExpression_(ov4);

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

    // NOTE(rytis): Offbeat grid init.
    {
        u32 Width = 1000;
        u32 Height = 1000;
        u32 SquareCount = 50;
        u64 Size = Width * Height * sizeof(u32);

        void* TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateGridRGBATexture(TextureMemory, Size, Width, Height, SquareCount);
        OffbeatState->GridTextureID = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);

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


        OffbeatState->GridVertices[0] = ob_draw_vertex{BottomLeft, BottomLeftUV, Color};
        OffbeatState->GridVertices[1] = ob_draw_vertex{BottomRight, BottomRightUV, Color};
        OffbeatState->GridVertices[2] = ob_draw_vertex{TopRight, TopRightUV, Color};
        OffbeatState->GridVertices[3] = ob_draw_vertex{TopLeft, TopLeftUV, Color};
    }

    // NOTE(rytis): Offbeat texture init.
    {
        u32 Width = 500;
        u32 Height = 500;
        u64 Size = Width * Height * sizeof(u32);

        void* TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateSquareRGBATexture(TextureMemory, Size, Width, Height);
        OffbeatGlobalData.TextureIDs[OFFBEAT_TextureSquare] = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);

        TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateCircleRGBATexture(TextureMemory, Size, Width, Height);
        OffbeatGlobalData.TextureIDs[OFFBEAT_TextureCircle] = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);

        TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateCrossRGBATexture(TextureMemory, Size, Width, Height, Height / 2);
        OffbeatGlobalData.TextureIDs[OFFBEAT_TextureFatCross] = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);

        TextureMemory = OffbeatAllocateMemory(&OffbeatState->MemoryManager, Size);
        OffbeatGenerateCrossRGBATexture(TextureMemory, Size, Width, Height, Height / 25);
        OffbeatGlobalData.TextureIDs[OFFBEAT_TextureSlimCross] = OffbeatGlobalRGBATextureID(TextureMemory, Width, Height);
    }

    // NOTE(rytis): Offbeat parameter offset init.
    {
        OffbeatGlobalData.ParameterOffsets[0] = 0;
        OffbeatGlobalData.ParameterOffsets[1] = OffbeatOffsetOf(ob_particle, Age);
        OffbeatGlobalData.ParameterOffsets[2] = OffbeatOffsetOf(ob_particle, Random);
        OffbeatGlobalData.ParameterOffsets[3] = OffbeatOffsetOf(ob_particle, CameraDistance);
        OffbeatGlobalData.ParameterOffsets[4] = OffbeatOffsetOf(ob_particle, ID);
    }

    // NOTE(rytis): OffbeatState init.
    {
        OffbeatState->t = 0.0f;
        OffbeatGlobalData.t = &OffbeatState->t;

        OffbeatState->RenderProgramID = OffbeatCreateRenderProgram();

        OffbeatState->EffectsEntropy = ObRandomSeed(1234);

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
        TestMotion.Strength.High = 10.0f;

        ob_appearance TestAppearance = {};
        TestAppearance.Color.Low = TestAppearance.Color.High =
            ov4{ObRandomBetween(&OffbeatState->EffectsEntropy, 0.45f, 0.5f),
            ObRandomBetween(&OffbeatState->EffectsEntropy, 0.5f, 0.8f),
            ObRandomBetween(&OffbeatState->EffectsEntropy, 0.8f, 1.0f),
            1.0f};
        TestAppearance.Size.Low = TestAppearance.Size.High = 0.05f;
        TestAppearance.TextureID = OffbeatGlobalData.TextureIDs[OFFBEAT_TextureCircle];

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
OffbeatSpawnParticles(ob_particle* Particles, ob_particle_system* ParticleSystem, ob_random_series* Entropy, f32 dt, u32 ParticleSpawnCount, ov3* CameraPosition, u32* RunningParticleID)
{
    ob_emission* Emission = &ParticleSystem->Emission;

    for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < ParticleSpawnCount; ++ParticleSpawnIndex)
    {
        ob_particle* Particle = Particles + ParticleSpawnIndex;

        Particle->P = OffbeatParticleInitialPosition(Entropy, Emission);
        Particle->dP = OffbeatParticleInitialVelocity(Entropy, Emission);
        Particle->Age = 0.0f;
        Particle->Random = ObRandomUnilateral(Entropy);
        Particle->CameraDistance = ObLength(*CameraPosition - Particle->P);
        Particle->ID = (*RunningParticleID)++;
        OffbeatDebugSpawnPoint(Particle->P);
    }
}

static ov3
OffbeatUpdateParticleddP(ob_motion* Motion, ob_particle* Particle)
{
    f32 LengthSq = ObLengthSq(Particle->dP);
    ov3 Drag = Motion->Drag * LengthSq * ObNormalize(-Particle->dP);
    ov3 Result = Motion->Gravity + Drag;
    f32 Strength = OffbeatEvaluateExpression(&Motion->Strength, Particle);
    ov3 Direction;
    switch(Motion->Primitive)
    {
        case OFFBEAT_MotionPoint:
        {
            Direction = Motion->Point.Position - Particle->P;
        } break;

        case OFFBEAT_MotionLine:
        {
            f32 t = ObInner(Particle->P - Motion->Line.Position, Motion->Line.Direction) /
                    ObInner(Motion->Line.Direction, Motion->Line.Direction);
            Direction = (Motion->Line.Position + t * Motion->Line.Direction) - Particle->P;
        } break;

        case OFFBEAT_MotionNone:
        default:
        {
            Direction = ov3{0.0f, 0.0f, 0.0f};
        } break;
    }
    Result += Strength * ObNormalize(Direction);

    return Result;
}

static void
OffbeatUpdateParticleSystem(ob_particle* Particles, ob_particle_system* ParticleSystem, f32 dt, u32 ParticleSpawnCount, ov3* CameraPosition)
{
    ob_motion* Motion = &ParticleSystem->Motion;

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
        UpdatedParticle->CameraDistance = ObLength(*CameraPosition - UpdatedParticle->P);
        ++UpdatedParticle;
    }

    ParticleSystem->Particles = Particles;
}

static void
OffbeatConstructQuad(ob_draw_list* DrawList, ob_quad_data* QuadData, ob_appearance* Appearance, ob_particle* Particle)
{
    f32 Size = OffbeatEvaluateExpression(&Appearance->Size, Particle);
    // NOTE(rytis): Vertices (facing the camera plane)
    ov3 BottomLeft = Particle->P + 0.5f * Size * (-QuadData->Horizontal - QuadData->Vertical);
    ov3 BottomRight = Particle->P + 0.5f * Size * (QuadData->Horizontal - QuadData->Vertical);
    ov3 TopRight = Particle->P + 0.5f * Size * (QuadData->Horizontal + QuadData->Vertical);
    ov3 TopLeft = Particle->P + 0.5f * Size * (-QuadData->Horizontal + QuadData->Vertical);

    // NOTE(rytis): UVs
    ov2 BottomLeftUV = ov2{0.0f, 0.0f};
    ov2 BottomRightUV = ov2{1.0f, 0.0f};
    ov2 TopRightUV = ov2{1.0f, 1.0f};
    ov2 TopLeftUV = ov2{0.0f, 1.0f};

    ov4 Color = OffbeatEvaluateExpression(&Appearance->Color, Particle);

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
OffbeatRenderParticleSystem(ob_draw_list* DrawList, u32* IndexMemory, ob_draw_vertex* VertexMemory, ob_particle_system* ParticleSystem, ob_quad_data* QuadData)
{
    ob_appearance* Appearance = &ParticleSystem->Appearance;

    *DrawList = {};
    DrawList->Indices = IndexMemory;
    DrawList->Vertices = VertexMemory;
    DrawList->TextureID = Appearance->TextureID;

    for(u32 ParticleIndex = 0; ParticleIndex < ParticleSystem->ParticleCount; ++ParticleIndex)
    {
        ob_particle* Particle = ParticleSystem->Particles + ParticleIndex;

        OffbeatConstructQuad(DrawList, QuadData, Appearance, Particle);
    }
}

static u32
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

    return ParticleSpawnCount;
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
        QuadData.Horizontal = ObNormalize(Camera.Right);
        QuadData.Vertical = ObNormalize(ObCross(QuadData.Horizontal, Camera.Forward));
        OffbeatState->QuadData = QuadData;
        OffbeatState->CameraPosition = Camera.Position;
#ifdef OFFBEAT_DEBUG
        OffbeatGlobalData.DebugQuadData = QuadData;
#endif

        OffbeatUpdateMemoryManager(&OffbeatState->MemoryManager);
    }

    OffbeatState->DrawData.DrawListCount = OffbeatState->ParticleSystemCount;
#ifdef OFFBEAT_DEBUG
    OffbeatGlobalData.DebugDrawData.DrawListCount = OffbeatState->ParticleSystemCount;
#endif
    for(u32 i = 0; i < OffbeatState->ParticleSystemCount; ++i)
    {
        ob_particle_system* ParticleSystem = &OffbeatState->ParticleSystems[i];

        OffbeatUpdateSystemRotations(ParticleSystem);

        ob_history_entry* History =
            (ob_history_entry*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                     (ParticleSystem->HistoryEntryCount + 1) *
                                                     sizeof(ob_history_entry));

        u32 ParticleSpawnCount =
            OffbeatUpdateParticleCount(History, ParticleSystem, OffbeatState->t, OffbeatState->dt);

#ifdef OFFBEAT_DEBUG
        OffbeatGlobalData.CurrentParticleSystem = i;
        OffbeatGlobalData.DebugDrawData.DrawLists[i] = {};
        OffbeatGlobalData.DebugDrawData.DrawLists[i].TextureID = OffbeatGlobalData.TextureIDs[OFFBEAT_TextureSquare];
        OffbeatGlobalData.DebugDrawData.DrawLists[i].Indices =
            (u32*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                        (ParticleSpawnCount + 1) *
                                        sizeof(u32) * 6);
        OffbeatGlobalData.DebugDrawData.DrawLists[i].Vertices =
            (ob_draw_vertex*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                   (ParticleSpawnCount + 1) *
                                                   sizeof(ob_draw_vertex) * 4);
#endif

        ob_particle* Particles =
            (ob_particle*)OffbeatAllocateMemory(&OffbeatState->MemoryManager,
                                                ParticleSystem->ParticleCount *
                                                sizeof(ob_particle));
        OffbeatSpawnParticles(Particles, ParticleSystem, &OffbeatState->EffectsEntropy,
                              OffbeatState->dt, ParticleSpawnCount, &OffbeatState->CameraPosition,
                              &OffbeatState->RunningParticleID);
        OffbeatUpdateParticleSystem(Particles, ParticleSystem, OffbeatState->dt, ParticleSpawnCount,
                                    &OffbeatState->CameraPosition);

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

        OffbeatDebugMotionPrimitive(&ParticleSystem->Motion);
    }

    OffbeatState->CycleCount = __rdtsc() - StartCycleCount;
}

ob_draw_data*
OffbeatGetDrawData(ob_state* OffbeatState)
{
    return &OffbeatState->DrawData;
}
