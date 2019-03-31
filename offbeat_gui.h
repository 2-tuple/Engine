#pragma once

#include "offbeat.h"
#include "ui.h"
#include "file_io.h"

static char* g_EmissionShapeStrings[OFFBEAT_EmissionCount] = {
    "Point",
    "Ring",
};

static char* g_EmissionVelocityStrings[OFFBEAT_VelocityCount] = {
    "Random",
    "Cone",
};

static char* g_MotionPrimitiveStrings[OFFBEAT_MotionCount] = {
    "None",
    "Point",
    "Line",
};

static char* g_TextureStrings[OFFBEAT_TextureCount] = {
    "Square",
    "Disc",
    "Fat Cross",
    "Slim Cross",
    // "Ring",
};

static char* g_FunctionStrings[OFFBEAT_FunctionCount] = {
    "Const",
    "Lerp",
    "Smoothstep",
    "Squared",
    "Cubed",
    "Fourth",
    "Triangle",
    "Two Triangles",
    "Four Triangles",
    "Periodic",
};

static char* g_ParameterStrings[OFFBEAT_ParameterCount] = {
    "Age",
    "Random",
    "CameraDistance",
    "ID",
};

char*
OffbeatGUIPathArrayToString(void* Data, int Index)
{
  path* Paths = (path*)Data;
  return Paths[Index].Name;
}

void
OffbeatExportParticleSystem(ob_state* OffbeatState, u32 ParticleSystemIndex, char* Path)
{
    ob_file_data FileData = OffbeatPackParticleSystem(OffbeatState, ParticleSystemIndex);
    Platform::WriteEntireFile(Path, FileData.Size, FileData.Data);
}

void
OffbeatExportCurrentParticleSystem(ob_state* OffbeatState, char* Path)
{
    OffbeatExportParticleSystem(OffbeatState, OffbeatState->CurrentParticleSystem, Path);
}

void
OffbeatImportParticleSystem(game_state* GameState, ob_state* OffbeatState, char* Path)
{
    GameState->TemporaryMemStack->NullifyClear();
    debug_read_file_result ReadResult = Platform::ReadEntireFile(GameState->TemporaryMemStack, Path);
    OffbeatAssert(ReadResult.Contents);

    ob_particle_system NewParticleSystem = {};
    // TODO(rytis): Check if unpack succeeded.
    OffbeatUnpackParticleSystem(&NewParticleSystem, ReadResult.Contents);
    OffbeatState->CurrentParticleSystem = OffbeatState->ParticleSystemCount;
    OffbeatState->ParticleSystems[OffbeatState->ParticleSystemCount++] = NewParticleSystem;
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

#define OffbeatGUIExpression(type) void OffbeatGUIExpression##type(const char* Name, ob_expr* Expression, bool* Header, bool MinZero)\
{\
    char FunctionName[50];\
    char ParameterName[50];\
    char FreqName[50];\
    char LowName[50];\
    char HighName[50];\
\
    strcpy(FunctionName, Name);\
    strcat(FunctionName, " Function");\
    strcpy(ParameterName, Name);\
    strcat(ParameterName, " Parameter");\
    strcpy(FreqName, Name);\
    strcat(FreqName, " Frequency");\
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
                OffbeatDrag##type(HighName, &Expression->High, 0.0f, INFINITY, 5.0f);\
            }\
            else\
            {\
                OffbeatDrag##type(HighName, &Expression->High, -INFINITY, INFINITY, 5.0f);\
            }\
        }\
        else\
        {\
            if(Expression->Function == OFFBEAT_FunctionPeriodic)\
            {\
                UI::DragFloat(FreqName, &Expression->Frequency, -INFINITY, INFINITY, 5.0f);\
            }\
            else\
            {\
                UI::Combo(ParameterName, (int*)&Expression->Parameter, g_ParameterStrings, OFFBEAT_ParameterCount, UI::StringArrayToString);\
            }\
\
            if(MinZero)\
            {\
                OffbeatDrag##type(LowName, &Expression->Low, 0.0f, INFINITY, 5.0f);\
                OffbeatDrag##type(HighName, &Expression->High, 0.0f, INFINITY, 5.0f);\
            }\
            else\
            {\
                OffbeatDrag##type(LowName, &Expression->Low, -INFINITY, INFINITY, 5.0f);\
                OffbeatDrag##type(HighName, &Expression->High, -INFINITY, INFINITY, 5.0f);\
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
    static ob_particle_system* ParticleSystem = &OffbeatState->ParticleSystems[0];
    UI::BeginWindow("Offbeat Window", {25, 25}, {500, 800});
    {
        static u32 s_OffbeatCurrentParticleSystem = 0;
        static s32 s_OffbeatCurrentFile = 0;

        static bool s_OffbeatShowEmission = false;
        static bool s_OffbeatShowMotion = false;
        static bool s_OffbeatShowAppearance = false;
        static bool s_OffbeatEmissionLifetime = false;
        static bool s_OffbeatMotionStrength = false;
        static bool s_OffbeatAppearanceColor = false;
        static bool s_OffbeatAppearanceSize = false;

        char SystemIDBuffer[50];
        sprintf(SystemIDBuffer, "Current particle system ID: %u", OffbeatState->CurrentParticleSystem);
        UI::Text(SystemIDBuffer);

        if(UI::Button("<"))
        {
            ParticleSystem = OffbeatPreviousParticleSystem(OffbeatState);
        }

        UI::SameLine();
        if(UI::Button("Add Particle System"))
        {
            ParticleSystem = OffbeatNewParticleSystem(OffbeatState);
        }

        UI::SameLine();
        if(UI::Button(">"))
        {
            ParticleSystem = OffbeatNextParticleSystem(OffbeatState);
        }

        UI::NewLine();
        if(UI::Button("Remove Particle System"))
        {
            OffbeatRemoveCurrentParticleSystem(OffbeatState);
            ParticleSystem = OffbeatGetCurrentParticleSystem(OffbeatState);
        }

        UI::NewLine();
        if(GameState->Resources.ParticleSystemPathCount > 0)
        {
            static s32 SelectedParticleSystemIndex = 0;
            if(UI::Button("Load"))
            {
                OffbeatImportParticleSystem(GameState, OffbeatState, GameState->Resources.ParticleSystemPaths[SelectedParticleSystemIndex].Name);
                ParticleSystem = OffbeatGetCurrentParticleSystem(OffbeatState);
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
            s_OffbeatEmissionLifetime = false;
            s_OffbeatMotionStrength = false;
            s_OffbeatAppearanceColor = false;
            s_OffbeatAppearanceSize = false;
            goto end_window;
        }

        if(GameState->Resources.ParticleSystemPathCount > 0)
        {
            static s32 SelectedParticleSystemIndex = 0;
            if(UI::Button("Save"))
            {
                OffbeatExportCurrentParticleSystem(OffbeatState, GameState->Resources.ParticleSystemPaths[SelectedParticleSystemIndex].Name);
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

            OffbeatExportCurrentParticleSystem(OffbeatState, Path);
        }

        if(UI::CollapsingHeader("Emission", &s_OffbeatShowEmission))
        {
            UI::DragFloat3("Location", ParticleSystem->Emission.Location.E, -INFINITY, INFINITY, 10.0f);
            UI::DragFloat("Emission Rate", &ParticleSystem->Emission.EmissionRate, 0.0f, INFINITY, 500.0f);
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
                {
                    UI::DragFloat("Ring Radius", &ParticleSystem->Emission.RingRadius, 0.0f, INFINITY, 10.0f);
                    UI::DragFloat3("Ring Normal", ParticleSystem->Emission.RingNormal.E, -INFINITY, INFINITY, 10.0f);
                } break;
            }

            UI::DragFloat("Initial Velocity Scale", &ParticleSystem->Emission.InitialVelocityScale, -INFINITY, INFINITY, 5.0f);

            ob_emission_velocity* CurrentVelocityType = &ParticleSystem->Emission.VelocityType;
            UI::Combo("Velocity Type", (int*)CurrentVelocityType, g_EmissionVelocityStrings, OFFBEAT_VelocityCount, UI::StringArrayToString);

            switch(*CurrentVelocityType)
            {
                case OFFBEAT_VelocityRandom:
                {
                } break;

                case OFFBEAT_VelocityCone:
                {
                    UI::DragFloat3("Cone Direction", ParticleSystem->Emission.ConeDirection.E, -INFINITY, INFINITY, 10.0f);
                    UI::DragFloat("Cone Height", &ParticleSystem->Emission.ConeHeight, 0.0f, INFINITY, 10.0f);
                    UI::DragFloat("Cone Radius", &ParticleSystem->Emission.ConeRadius, 0.0f, INFINITY, 10.0f);
                } break;
            }
        }

        if(UI::CollapsingHeader("Motion", &s_OffbeatShowMotion))
        {
            UI::DragFloat3("Gravity", ParticleSystem->Motion.Gravity.E, -INFINITY, INFINITY, 10.0f);
            UI::DragFloat("Drag Strength", &ParticleSystem->Motion.Drag, -INFINITY, INFINITY, 10.0f);

            ob_motion_primitive* CurrentPrimitive = &ParticleSystem->Motion.Primitive;
            UI::Combo("Motion Primitive", (int*)CurrentPrimitive, g_MotionPrimitiveStrings, OFFBEAT_MotionCount, UI::StringArrayToString);

            switch(*CurrentPrimitive)
            {
                case OFFBEAT_MotionPoint:
                {
                    OffbeatGUIExpressionF32("Strength", &ParticleSystem->Motion.Strength,
                                            &s_OffbeatMotionStrength, false);
                    UI::DragFloat3("Position", ParticleSystem->Motion.Position.E, -INFINITY, INFINITY, 10.0f);
                } break;

                case OFFBEAT_MotionLine:
                {
                    OffbeatGUIExpressionF32("Strength", &ParticleSystem->Motion.Strength,
                                            &s_OffbeatMotionStrength, false);
                    UI::DragFloat3("Position", ParticleSystem->Motion.Position.E, -INFINITY, INFINITY, 10.0f);
                    UI::DragFloat3("Direction", ParticleSystem->Motion.LineDirection.E, -INFINITY, INFINITY, 10.0f);
                } break;
            }
        }

        if(UI::CollapsingHeader("Appearance", &s_OffbeatShowAppearance))
        {
            OffbeatGUIExpressionOV4("Color", &ParticleSystem->Appearance.Color,
                                    &s_OffbeatAppearanceColor, true);
            OffbeatGUIExpressionF32("Size", &ParticleSystem->Appearance.Size,
                                    &s_OffbeatAppearanceSize, true);

            static ob_texture_type CurrentTexture = OffbeatGetCurrentTextureType(ParticleSystem);
            ob_texture_type NewTexture = CurrentTexture;
            UI::Combo("Texture", (int*)&NewTexture, g_TextureStrings, OFFBEAT_TextureCount, UI::StringArrayToString);
            if(NewTexture != CurrentTexture)
            {
                CurrentTexture = NewTexture;
                OffbeatSetTextureID(ParticleSystem, CurrentTexture);
            }
        }
        {
            u64 TotalParticleCount = 0;
            for(u32 i = 0; i < OffbeatState->ParticleSystemCount; ++i)
            {
                TotalParticleCount += OffbeatState->ParticleSystems[i].ParticleCount;
            }

            u64 CycleCount = OffbeatState->CycleCount;
            f32 CyclesPerParticle = TotalParticleCount ? CycleCount / (f32)TotalParticleCount : 0.0f;
            f32 MSCount = OffbeatState->MilisecondCount;
            f32 ParticlesPerMS = (MSCount > 0.0f) ? ((f32)TotalParticleCount / MSCount) : 0.0f;

            char ParticleBuffer[50];
            sprintf(ParticleBuffer, "Particles: %llu", TotalParticleCount);
            UI::Text(ParticleBuffer);

            char TimeBuffer[50];
            sprintf(TimeBuffer, "%.2f s", OffbeatState->t);
            UI::Text(TimeBuffer);

            char MSBuffer[50];
            sprintf(MSBuffer, "%.3f ms", MSCount);
            UI::Text(MSBuffer);

            char ParticlesPerMSBuffer[50];
            sprintf(ParticlesPerMSBuffer, "%d particles/ms", (s32)ParticlesPerMS);
            UI::Text(ParticlesPerMSBuffer);

            char CountBuffer[50];
            sprintf(CountBuffer, "Cycles: %llu", CycleCount);
            UI::Text(CountBuffer);

            char PerParticleBuffer[50];
            sprintf(PerParticleBuffer, "%.2f cycles/particle", CyclesPerParticle);
            UI::Text(PerParticleBuffer);
        }
    }
end_window:
    UI::EndWindow();
}
