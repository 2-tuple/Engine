#pragma once

#include "offbeat.h"
#include "ui.h"
#include "file_io.h"

#define OFFBEAT_FRAME_COUNT 60

static char* g_EmissionShapeStrings[OFFBEAT_EmissionCount] = {
    "Point",
    "Ring",
    "Disk",
    "Square",
    "Sphere",
    "Sphere Volume",
    "Cube Volume",
};

static char* g_EmissionVelocityStrings[OFFBEAT_VelocityCount] = {
    "Random",
    "Cone",
};

static char* g_MotionPrimitiveStrings[OFFBEAT_MotionCount] = {
    "None",
    "Point",
    "Line",
    "Sphere",
};

static char* g_FunctionStrings[OFFBEAT_FunctionCount] = {
    "Const",
    "Lerp",
    "Max Lerp",
    "Triangle",
    "Two Triangles",
    "Four Triangles",
    "Step",
    "Periodic",
    "Periodic Time",
    "Periodic Square",
    "Periodic Square Time",
};

static char* g_ParameterStrings[OFFBEAT_ParameterCount] = {
    "Age",
    "Velocity",
    "ID",
    "Random",
    "CollisionCount",
    "CameraDistance",
};

static uint32_t g_TextureCount;
static char g_TextureStrings[OFFBEAT_PARTICLE_SYSTEM_COUNT][50];

static uint32_t g_CurrentFrame;
static uint32_t g_TotalParticleCounts[OFFBEAT_FRAME_COUNT];
static int64_t g_CycleCounts[OFFBEAT_FRAME_COUNT];
static float g_MilisecondCounts[OFFBEAT_FRAME_COUNT];

char*
OffbeatGUITextureString(void* Data, int Index)
{
    return g_TextureStrings[Index];
}

char*
OffbeatGUIPathArrayToString(void* Data, int Index)
{
    path* Paths = (path*)Data;
    return Paths[Index].Name;
}

void
OffbeatGUIAddTexture(Resource::resource_manager* Resources, char* Name, char* Path)
{
    strcpy(g_TextureStrings[g_TextureCount++], Name);

    rid TextureRID;
    if(!Resources->GetTexturePathRID(&TextureRID, Path))
    {
        TextureRID = Resources->RegisterTexture(Path);
    }
    OffbeatAddTexture(Resources->GetTexture(TextureRID));
}

void
OffbeatGUIAddFrameInfo(uint32_t TotalParticleCount, int64_t CycleCount, float MilisecondCount)
{
    g_TotalParticleCounts[g_CurrentFrame] = TotalParticleCount;
    g_CycleCounts[g_CurrentFrame] = CycleCount;
    g_MilisecondCounts[g_CurrentFrame] = MilisecondCount;
    g_CurrentFrame = (g_CurrentFrame + 1) % OFFBEAT_FRAME_COUNT;
}

void
OffbeatExportCurrentParticleSystem(char* Path)
{
    ob_file_data FileData = OffbeatPackCurrentParticleSystemBytes();
    Platform::WriteEntireFile(Path, FileData.Size, FileData.Data);
}

void
OffbeatImportParticleSystem(game_state* GameState, char* Path)
{
    GameState->TemporaryMemStack->NullifyClear();
    debug_read_file_result ReadResult = Platform::ReadEntireFile(GameState->TemporaryMemStack, Path);
    OffbeatAssert(ReadResult.Contents);

    ob_particle_system NewParticleSystem = {};
    // TODO(rytis): Check if unpack succeeded.
    OffbeatUnpackParticleSystem(&NewParticleSystem, ReadResult.Contents);
    OffbeatAddParticleSystem(&NewParticleSystem);
}

static void
OffbeatDragF32(const char* Label, ov4* Value, float MinValue, float MaxValue, float ScreenDelta)
{
    UI::DragFloat(Label, Value->E, MinValue, MaxValue, ScreenDelta);
}

static void
OffbeatDragOV3(const char* Label, ov4* Value, float MinValue, float MaxValue, float ScreenDelta)
{
    UI::DragFloat3(Label, Value->E, MinValue, MaxValue, ScreenDelta);
}

static void
OffbeatDragOV4(const char* Label, ov4* Value, float MinValue, float MaxValue, float ScreenDelta)
{
    UI::DragFloat4(Label, Value->E, MinValue, MaxValue, ScreenDelta);
}

#define OffbeatGUIExpression(type) void OffbeatGUIExpression##type(const char* Name, ob_expr* Expression, bool* Header, bool MinZero, float ScreenDelta = 5.0f)\
{\
    char FunctionName[50];\
    char ParameterName[50];\
    char FreqName[50];\
    char MaxValName[50];\
    char StepName[50];\
    char LowName[50];\
    char HighName[50];\
\
    strcpy(FunctionName, Name);\
    strcat(FunctionName, " Function");\
    strcpy(ParameterName, Name);\
    strcat(ParameterName, " Parameter");\
    strcpy(FreqName, Name);\
    strcat(FreqName, " Frequency");\
    strcpy(MaxValName, Name);\
    strcat(MaxValName, " Max Value");\
    strcpy(StepName, Name);\
    strcat(StepName, " Step Count");\
    strcpy(LowName, Name);\
    strcat(LowName, " Low");\
    strcpy(HighName, Name);\
    strcat(HighName, " High");\
\
    if(UI::CollapsingHeader(Name, Header))\
    {\
        UI::Combo(FunctionName, (int*)&Expression->Function, g_FunctionStrings, OFFBEAT_FunctionCount, UI::StringArrayToString);\
\
        if(Expression->Function == OFFBEAT_FunctionConst)\
        {\
            if(MinZero)\
            {\
                OffbeatDrag##type(HighName, &Expression->High, 0.0f, INFINITY, ScreenDelta);\
            }\
            else\
            {\
                OffbeatDrag##type(HighName, &Expression->High, -INFINITY, INFINITY, ScreenDelta);\
            }\
        }\
        else\
        {\
            if(Expression->Function == OFFBEAT_FunctionPeriodicTime ||\
               Expression->Function == OFFBEAT_FunctionPeriodicSquareTime)\
            {\
                UI::DragFloat(FreqName, &Expression->Float, -INFINITY, INFINITY, 5.0f);\
            }\
            else\
            {\
                if(Expression->Function ==OFFBEAT_FunctionMaxLerp)\
                {\
                    UI::DragFloat(MaxValName, &Expression->Float, 0.0f, INFINITY, 10.0f);\
                }\
                else if(Expression->Function == OFFBEAT_FunctionPeriodic ||\
                   Expression->Function == OFFBEAT_FunctionPeriodicSquare)\
                {\
                    UI::DragFloat(FreqName, &Expression->Float, -INFINITY, INFINITY, 5.0f);\
                }\
                else if(Expression->Function == OFFBEAT_FunctionStep)\
                {\
                    UI::DragFloat(MaxValName, &Expression->Float, 0.0f, INFINITY, 10.0f);\
                    UI::SliderInt(StepName, (int32_t*)&Expression->Uint, 1, 30);\
                }\
                UI::Combo(ParameterName, (int*)&Expression->Parameter, g_ParameterStrings, OFFBEAT_ParameterCount, UI::StringArrayToString);\
            }\
\
            if(MinZero)\
            {\
                OffbeatDrag##type(LowName, &Expression->Low, 0.0f, INFINITY, ScreenDelta);\
                OffbeatDrag##type(HighName, &Expression->High, 0.0f, INFINITY, ScreenDelta);\
            }\
            else\
            {\
                OffbeatDrag##type(LowName, &Expression->Low, -INFINITY, INFINITY, ScreenDelta);\
                OffbeatDrag##type(HighName, &Expression->High, -INFINITY, INFINITY, ScreenDelta);\
            }\
        }\
    }\
}

static OffbeatGUIExpression(F32);
static OffbeatGUIExpression(OV3);
static OffbeatGUIExpression(OV4);

void
OffbeatWindow(game_state* GameState, const game_input* Input)
{
    ob_state* OffbeatState = GameState->OffbeatState;
    // TODO(rytis): Handle the case when ParticleSystemCount is zero.
    ob_particle_system* ParticleSystem = OffbeatGetCurrentParticleSystem();
    UI::BeginWindow("Offbeat Window", {25, 25}, {500, 800});
    {
        static u32 s_OffbeatCurrentParticleSystem = 0;
        static s32 s_OffbeatCurrentFile = 0;

        static bool s_OffbeatShowEmission = false;
        static bool s_OffbeatShowMotion = false;
        static bool s_OffbeatShowAppearance = false;
        static bool s_OffbeatEmissionLocation = false;
        static bool s_OffbeatEmissionRate = false;
        static bool s_OffbeatEmissionLifetime = false;
        static bool s_OffbeatEmissionRadius = false;
        static bool s_OffbeatEmissionNormal = false;
        static bool s_OffbeatEmissionInitialVelocityScale = false;
        static bool s_OffbeatEmissionConeDirection = false;
        static bool s_OffbeatEmissionConeHeight = false;
        static bool s_OffbeatEmissionConeRadius = false;
        static bool s_OffbeatMotionStrength = false;
        static bool s_OffbeatMotionGravity = false;
        static bool s_OffbeatMotionDrag = false;
        static bool s_OffbeatMotionPosition = false;
        static bool s_OffbeatMotionLineDirection = false;
        static bool s_OffbeatMotionSphereRadius = false;
        static bool s_OffbeatAppearanceColor = false;
        static bool s_OffbeatAppearanceSize = false;

        char SystemIDBuffer[50];
        sprintf(SystemIDBuffer, "Current particle system ID: %u", OffbeatState->CurrentParticleSystem);
        UI::Text(SystemIDBuffer);

        if(UI::Button("<"))
        {
            ParticleSystem = OffbeatPreviousParticleSystem();
        }

        UI::SameLine();
        if(UI::Button("Add Particle System"))
        {
            ParticleSystem = OffbeatNewParticleSystem();
        }

        UI::SameLine();
        if(UI::Button(">"))
        {
            ParticleSystem = OffbeatNextParticleSystem();
        }

        UI::NewLine();
        if(UI::Button("Remove Particle System"))
        {
            OffbeatRemoveCurrentParticleSystem();
            ParticleSystem = OffbeatGetCurrentParticleSystem();
        }

        UI::NewLine();
        if(GameState->Resources.ParticleSystemPathCount > 0)
        {
            static s32 SelectedParticleSystemIndex = 0;
            if(UI::Button("Load"))
            {
                OffbeatImportParticleSystem(GameState, GameState->Resources.ParticleSystemPaths[SelectedParticleSystemIndex].Name);
                ParticleSystem = OffbeatGetCurrentParticleSystem();
            }
            UI::SameLine();
            UI::Combo("Load Path", &SelectedParticleSystemIndex, GameState->Resources.ParticleSystemPaths, GameState->Resources.ParticleSystemPathCount, OffbeatGUIPathArrayToString);
            UI::NewLine();
        }

        if(OffbeatState->ParticleSystemCount == 0)
        {
            s_OffbeatShowEmission = false;
            s_OffbeatShowMotion = false;
            s_OffbeatShowAppearance = false;
            s_OffbeatEmissionLocation = false;
            s_OffbeatEmissionRate = false;
            s_OffbeatEmissionLifetime = false;
            s_OffbeatEmissionRadius = false;
            s_OffbeatEmissionNormal = false;
            s_OffbeatEmissionInitialVelocityScale = false;
            s_OffbeatEmissionConeDirection = false;
            s_OffbeatEmissionConeHeight = false;
            s_OffbeatEmissionConeRadius = false;
            s_OffbeatMotionStrength = false;
            s_OffbeatMotionGravity = false;
            s_OffbeatMotionDrag = false;
            s_OffbeatMotionPosition = false;
            s_OffbeatMotionLineDirection = false;
            s_OffbeatMotionSphereRadius = false;
            s_OffbeatAppearanceColor = false;
            s_OffbeatAppearanceSize = false;
            goto end_window;
        }

        if(GameState->Resources.ParticleSystemPathCount > 0)
        {
            static s32 SelectedParticleSystemIndex = 0;
            if(UI::Button("Save"))
            {
                OffbeatExportCurrentParticleSystem(GameState->Resources.ParticleSystemPaths[SelectedParticleSystemIndex].Name);
            }
            UI::SameLine();
            UI::Combo("Save Path", &SelectedParticleSystemIndex, GameState->Resources.ParticleSystemPaths, GameState->Resources.ParticleSystemPathCount, OffbeatGUIPathArrayToString);
            UI::NewLine();
        }

        UI::NewLine();
        if(UI::Button("Save Current As New"))
        {
            struct tm* TimeInfo;
            time_t CurrentTime;
            char Path[100];
            time(&CurrentTime);
            TimeInfo = localtime(&CurrentTime);
            strftime(Path, sizeof(Path), "data/offbeat/%H_%M_%S.obp", TimeInfo);

            OffbeatExportCurrentParticleSystem(Path);
        }

        bool DrawDebugData = OffbeatState->DrawDebugData;
        UI::Checkbox("Draw Debug", &DrawDebugData);
        OffbeatState->DrawDebugData = DrawDebugData;

        bool UseGPU = ParticleSystem->UseGPU;
        UI::Checkbox("Use GPU", &UseGPU);
        if(UseGPU != (bool)ParticleSystem->UseGPU)
        {
            OffbeatToggleGPU(ParticleSystem);
        }

        if(UI::CollapsingHeader("Emission", &s_OffbeatShowEmission))
        {
            OffbeatGUIExpressionOV3("Location", &ParticleSystem->Emission.Location,
                                    &s_OffbeatEmissionLocation, false, 10.0f);
            OffbeatGUIExpressionF32("Emission Rate", &ParticleSystem->Emission.EmissionRate,
                                    &s_OffbeatEmissionRate, true, 500.0f);
            OffbeatGUIExpressionF32("Particle Lifetime", &ParticleSystem->Emission.ParticleLifetime,
                                    &s_OffbeatEmissionLifetime, true);

            ob_emission_shape* CurrentShape = &ParticleSystem->Emission.Shape;
            UI::Combo("Emission Shape", (int*)CurrentShape, g_EmissionShapeStrings, OFFBEAT_EmissionCount, UI::StringArrayToString);

            switch(*CurrentShape)
            {
                case OFFBEAT_EmissionPoint:
                {
                } break;

                case OFFBEAT_EmissionRing:
                case OFFBEAT_EmissionDisk:
                case OFFBEAT_EmissionSquare:
                case OFFBEAT_EmissionCubeVolume:
                {
                    OffbeatGUIExpressionF32("Radius", &ParticleSystem->Emission.EmissionRadius,
                                            &s_OffbeatEmissionRadius, true, 10.0f);
                    OffbeatGUIExpressionOV3("Normal", &ParticleSystem->Emission.EmissionNormal,
                                            &s_OffbeatEmissionNormal, false, 10.0f);
                } break;

                case OFFBEAT_EmissionSphere:
                case OFFBEAT_EmissionSphereVolume:
                {
                    OffbeatGUIExpressionF32("Radius", &ParticleSystem->Emission.EmissionRadius,
                                            &s_OffbeatEmissionRadius, true, 10.0f);
                } break;
            }

            OffbeatGUIExpressionF32("Initial Velocity Scale",
                                    &ParticleSystem->Emission.InitialVelocityScale,
                                    &s_OffbeatEmissionInitialVelocityScale, false);

            ob_emission_velocity* CurrentVelocityType = &ParticleSystem->Emission.VelocityType;
            UI::Combo("Velocity Type", (int*)CurrentVelocityType, g_EmissionVelocityStrings, OFFBEAT_VelocityCount, UI::StringArrayToString);

            switch(*CurrentVelocityType)
            {
                case OFFBEAT_VelocityRandom:
                {
                } break;

                case OFFBEAT_VelocityCone:
                {
                    OffbeatGUIExpressionOV3("Cone Direction", &ParticleSystem->Emission.ConeDirection,
                                            &s_OffbeatEmissionConeDirection, false, 10.0f);
                    OffbeatGUIExpressionF32("Cone Height", &ParticleSystem->Emission.ConeHeight,
                                            &s_OffbeatEmissionConeHeight, true, 10.0f);
                    OffbeatGUIExpressionF32("Cone Radius", &ParticleSystem->Emission.ConeRadius,
                                            &s_OffbeatEmissionConeRadius, true, 10.0f);
                } break;
            }
        }

        if(UI::CollapsingHeader("Motion", &s_OffbeatShowMotion))
        {
            OffbeatGUIExpressionOV3("Gravity", &ParticleSystem->Motion.Gravity,
                                    &s_OffbeatMotionGravity, false, 10.0f);
            OffbeatGUIExpressionF32("Drag Strength", &ParticleSystem->Motion.Drag,
                                    &s_OffbeatMotionDrag, false, 1.0f);
            OffbeatGUIExpressionF32("Strength", &ParticleSystem->Motion.Strength,
                                    &s_OffbeatMotionStrength, false);
            UI::Checkbox("Collision Detection", (bool*)&ParticleSystem->Motion.Collision);

            ob_motion_primitive* CurrentPrimitive = &ParticleSystem->Motion.Primitive;
            UI::Combo("Motion Primitive", (int*)CurrentPrimitive, g_MotionPrimitiveStrings, OFFBEAT_MotionCount, UI::StringArrayToString);

            switch(*CurrentPrimitive)
            {
                case OFFBEAT_MotionPoint:
                {
                    OffbeatGUIExpressionOV3("Position", &ParticleSystem->Motion.Position,
                                            &s_OffbeatMotionPosition, false, 10.0f);
                } break;

                case OFFBEAT_MotionLine:
                {
                    OffbeatGUIExpressionOV3("Position", &ParticleSystem->Motion.Position,
                                            &s_OffbeatMotionPosition, false, 10.0f);
                    OffbeatGUIExpressionOV3("Direction", &ParticleSystem->Motion.LineDirection,
                                            &s_OffbeatMotionLineDirection, false, 10.0f);
                } break;

                case OFFBEAT_MotionSphere:
                {
                    OffbeatGUIExpressionOV3("Position", &ParticleSystem->Motion.Position,
                                            &s_OffbeatMotionPosition, false, 10.0f);
                    OffbeatGUIExpressionF32("Radius", &ParticleSystem->Motion.SphereRadius,
                                            &s_OffbeatMotionSphereRadius, false, 10.0f);
                } break;
            }
        }

        if(UI::CollapsingHeader("Appearance", &s_OffbeatShowAppearance))
        {
            OffbeatGUIExpressionOV4("Color", &ParticleSystem->Appearance.Color,
                                    &s_OffbeatAppearanceColor, true);
            OffbeatGUIExpressionF32("Size", &ParticleSystem->Appearance.Size,
                                    &s_OffbeatAppearanceSize, true);

            if(GameState->Resources.TexturePathCount > 0)
            {
                s32 CurrentTextureIndex = ParticleSystem->Appearance.TextureIndex;
                s32 NewTextureIndex = CurrentTextureIndex;
                UI::Combo("Texture", &NewTextureIndex, g_TextureStrings, g_TextureCount, OffbeatGUITextureString);
                if(NewTextureIndex != CurrentTextureIndex)
                {
                    CurrentTextureIndex = NewTextureIndex;
                    ParticleSystem->Appearance.TextureIndex = CurrentTextureIndex;
                }
            }
        }
        {
            uint32_t ParticleSum = 0;
            int64_t CycleSum = 0;
            float MilisecondSum = 0;

            for(int i = 0; i < OFFBEAT_FRAME_COUNT; ++i)
            {
                ParticleSum += g_TotalParticleCounts[i];
                CycleSum += g_CycleCounts[i];
                MilisecondSum += g_MilisecondCounts[i];
            }

            u64 FrameCount = OFFBEAT_FRAME_COUNT;
            u64 AvgParticleCount = ParticleSum / FrameCount;
            u64 AvgCycleCount = CycleSum / FrameCount;
            f32 AvgCyclesPerParticle = AvgParticleCount ? ((f32)AvgCycleCount / (f32)AvgParticleCount) : 0.0f;
            f32 AvgMSCount = MilisecondSum / (f32)FrameCount;
            u32 AvgParticlesPerMS = (u32)((f32)AvgParticleCount / AvgMSCount);

            static bool BufferInit = false;
            static char ParticleBuffer[50];
            static char TimeBuffer[50];
            static char MSBuffer[50];
            static char ParticlesPerMSBuffer[50];
            static char CountBuffer[50];
            static char PerParticleBuffer[50];

            sprintf(ParticleBuffer, "Particles: %llu", (unsigned long long)OffbeatState->TotalParticleCount);
            sprintf(TimeBuffer, "%.2f s", OffbeatState->t);
            sprintf(MSBuffer, "%.3f ms average", AvgMSCount);
            sprintf(ParticlesPerMSBuffer, "%u particles/ms", AvgParticlesPerMS);
            sprintf(CountBuffer, "%llu cycle average", (unsigned long long)AvgCycleCount);
            sprintf(PerParticleBuffer, "%.2f cycles/particle", AvgCyclesPerParticle);

            UI::Text(ParticleBuffer);
            UI::Text(TimeBuffer);
            UI::Text(MSBuffer);
            UI::Text(ParticlesPerMSBuffer);
            UI::Text(CountBuffer);
            UI::Text(PerParticleBuffer);
        }
    }
end_window:
    UI::EndWindow();
}
