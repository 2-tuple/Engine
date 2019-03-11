#pragma once

#include "offbeat.h"
#include "ui.h"

static char* g_EmissionShapeStrings[OFFBEAT_EmissionCount] = {
    "Point",
    "Ring",
};

static char* g_EmissionVelocityStrings[OFFBEAT_VelocityCount] = {
    "Cone",
    "Random",
};

static char* g_MotionPrimitiveStrings[OFFBEAT_MotionCount] = {
    "None",
    "Point",
};

static char* g_TextureStrings[OFFBEAT_TextureCount] = {
    "Square",
    "Disc",
    // "Ring",
};

static char* g_FunctionStrings[OFFBEAT_FunctionCount] = {
    "Const",
    "Lerp",
    "Sin",
    "Cos",
    "Floor",
    "Round",
    "Ceil",
};

static char* g_ParameterStrings[OFFBEAT_ParameterCount] = {
    "GlobalTime",
    "SystemTime",
    "Age",
    "Random",
    "CameraDistance",
    "ID",
};

static void
OffbeatExpressionF32(const char* Name, ob_expr_f32* Expression, bool MinZero)
{
    char FunctionName[50];
    char ParameterName[50];
    char LowName[50];
    char HighName[50];

    strcpy(FunctionName, Name);
    strcat(FunctionName, " Function");
    strcpy(ParameterName, Name);
    strcat(ParameterName, " Parameter");
    strcpy(LowName, Name);
    strcat(LowName, " Low");
    strcpy(HighName, Name);
    strcat(HighName, " High");

    UI::Combo(FunctionName, (int*)&Expression->Function, g_FunctionStrings, OFFBEAT_FunctionCount, UI::StringArrayToString);

    if(Expression->Function == OFFBEAT_FunctionConst)
    {
        if(MinZero)
        {
            UI::DragFloat(HighName, &Expression->High, 0.0f, INFINITY, 5.0f);
        }
        else
        {
            UI::DragFloat(HighName, &Expression->High, -INFINITY, INFINITY, 5.0f);
        }
    }
    else
    {
        UI::Combo(ParameterName, (int*)&Expression->Parameter, g_ParameterStrings, OFFBEAT_ParameterCount, UI::StringArrayToString);

        if(MinZero)
        {
            UI::DragFloat(LowName, &Expression->Low, 0.0f, INFINITY, 5.0f);
            UI::DragFloat(HighName, &Expression->High, 0.0f, INFINITY, 5.0f);
        }
        else
        {
            UI::DragFloat(LowName, &Expression->Low, -INFINITY, INFINITY, 5.0f);
            UI::DragFloat(HighName, &Expression->High, -INFINITY, INFINITY, 5.0f);
        }
    }
}

static void
OffbeatExpressionOV3(const char* Name, ob_expr_ov3* Expression, bool MinZero)
{
    char FunctionName[50];
    char ParameterName[50];
    char LowName[50];
    char HighName[50];

    strcpy(FunctionName, Name);
    strcat(FunctionName, " Function");
    strcpy(ParameterName, Name);
    strcat(ParameterName, " Parameter");
    strcpy(LowName, Name);
    strcat(LowName, " Low");
    strcpy(HighName, Name);
    strcat(HighName, " High");

    UI::Combo(FunctionName, (int*)&Expression->Function, g_FunctionStrings, OFFBEAT_FunctionCount, UI::StringArrayToString);

    if(Expression->Function == OFFBEAT_FunctionConst)
    {
        if(MinZero)
        {
            UI::DragFloat3(HighName, Expression->High.E, 0.0f, INFINITY, 5.0f);
        }
        else
        {
            UI::DragFloat3(HighName, Expression->High.E, -INFINITY, INFINITY, 5.0f);
        }
    }
    else
    {
        UI::Combo(ParameterName, (int*)&Expression->Parameter, g_ParameterStrings, OFFBEAT_ParameterCount, UI::StringArrayToString);

        if(MinZero)
        {
            UI::DragFloat3(LowName, Expression->Low.E, 0.0f, INFINITY, 5.0f);
            UI::DragFloat3(HighName, Expression->High.E, 0.0f, INFINITY, 5.0f);
        }
        else
        {
            UI::DragFloat3(LowName, Expression->Low.E, -INFINITY, INFINITY, 5.0f);
            UI::DragFloat3(HighName, Expression->High.E, -INFINITY, INFINITY, 5.0f);
        }
    }
}

static void
OffbeatExpressionOV4(const char* Name, ob_expr_ov4* Expression, bool MinZero)
{
    char FunctionName[50];
    char ParameterName[50];
    char LowName[50];
    char HighName[50];

    strcpy(FunctionName, Name);
    strcat(FunctionName, " Function");
    strcpy(ParameterName, Name);
    strcat(ParameterName, " Parameter");
    strcpy(LowName, Name);
    strcat(LowName, " Low");
    strcpy(HighName, Name);
    strcat(HighName, " High");

    UI::Combo(FunctionName, (int*)&Expression->Function, g_FunctionStrings, OFFBEAT_FunctionCount, UI::StringArrayToString);

    if(Expression->Function == OFFBEAT_FunctionConst)
    {
        if(MinZero)
        {
            UI::DragFloat4(HighName, Expression->High.E, 0.0f, INFINITY, 5.0f);
        }
        else
        {
            UI::DragFloat4(HighName, Expression->High.E, -INFINITY, INFINITY, 5.0f);
        }
    }
    else
    {
        UI::Combo(ParameterName, (int*)&Expression->Parameter, g_ParameterStrings, OFFBEAT_ParameterCount, UI::StringArrayToString);

        if(MinZero)
        {
            UI::DragFloat4(LowName, Expression->Low.E, 0.0f, INFINITY, 5.0f);
            UI::DragFloat4(HighName, Expression->High.E, 0.0f, INFINITY, 5.0f);
        }
        else
        {
            UI::DragFloat4(LowName, Expression->Low.E, -INFINITY, INFINITY, 5.0f);
            UI::DragFloat4(HighName, Expression->High.E, -INFINITY, INFINITY, 5.0f);
        }
    }
}

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

        char SystemIDBuffer[50];
        sprintf(SystemIDBuffer, "Current particle system ID: %u", s_OffbeatCurrentParticleSystem);
        UI::Text(SystemIDBuffer);

        if(UI::Button("<"))
        {
            if(s_OffbeatCurrentParticleSystem > 0)
            {
                ParticleSystem = &OffbeatState->ParticleSystems[--s_OffbeatCurrentParticleSystem];
            }
        }

        UI::SameLine();
        if(UI::Button("Add Particle System"))
        {
            s_OffbeatCurrentParticleSystem = OffbeatNewParticleSystem(OffbeatState, &ParticleSystem);
        }

        UI::SameLine();
        if(UI::Button(">"))
        {
            if(s_OffbeatCurrentParticleSystem + 1 < OffbeatState->ParticleSystemCount)
            {
                ParticleSystem = &OffbeatState->ParticleSystems[++s_OffbeatCurrentParticleSystem];
            }
        }

        UI::NewLine();
        if(UI::Button("Remove Particle System"))
        {
            OffbeatRemoveParticleSystem(OffbeatState, s_OffbeatCurrentParticleSystem);
            if(s_OffbeatCurrentParticleSystem >= OffbeatState->ParticleSystemCount)
            {
                s_OffbeatCurrentParticleSystem = OffbeatState->ParticleSystemCount - 1;
            }
        }

        UI::NewLine();
        if(UI::Button("Save Particle System"))
        {
        }

        // TODO(rytis): Combo box here for file loading.
        // UI::Combo("OPB files", &s_OffbeatCurrentFile, );

        if(UI::CollapsingHeader("Emission", &s_OffbeatShowEmission))
        {
            UI::DragFloat3("Location", ParticleSystem->Emission.Location.E, -INFINITY, INFINITY, 10.0f);
            UI::DragFloat("Emission Rate", &ParticleSystem->Emission.EmissionRate, 0.0f, INFINITY, 500.0f);
            UI::DragFloat("Particle Lifetime", &ParticleSystem->Emission.ParticleLifetime, 0.0f, INFINITY, 5.0f);

            ob_emission_shape_type* CurrentShapeType = &ParticleSystem->Emission.Shape.Type;
            UI::Combo("Emission Shape", (int*)CurrentShapeType, g_EmissionShapeStrings, OFFBEAT_EmissionCount, UI::StringArrayToString);

            switch(*CurrentShapeType)
            {
                case OFFBEAT_EmissionPoint:
                {
                } break;

                case OFFBEAT_EmissionRing:
                {
                    UI::DragFloat("Ring Radius", &ParticleSystem->Emission.Shape.Ring.Radius, 0.0f, INFINITY, 10.0f);
                    UI::DragFloat3("Ring Normal", ParticleSystem->Emission.Shape.Ring.Normal.E, -INFINITY, INFINITY, 10.0f);
                } break;
            }

            UI::DragFloat("Initial Velocity Scale", &ParticleSystem->Emission.InitialVelocityScale, -INFINITY, INFINITY, 5.0f);

            ob_emission_velocity_type* CurrentVelocityType = &ParticleSystem->Emission.Velocity.Type;
            UI::Combo("Velocity Type", (int*)CurrentVelocityType, g_EmissionVelocityStrings, OFFBEAT_VelocityCount, UI::StringArrayToString);

            switch(*CurrentVelocityType)
            {
                case OFFBEAT_VelocityCone:
                {
                    UI::DragFloat3("Cone Direction", ParticleSystem->Emission.Velocity.Cone.Direction.E, -INFINITY, INFINITY, 10.0f);
                    UI::DragFloat("Cone Height", &ParticleSystem->Emission.Velocity.Cone.Height, 0.0f, INFINITY, 10.0f);
                    UI::DragFloat("Cone Radius", &ParticleSystem->Emission.Velocity.Cone.Radius, 0.0f, INFINITY, 10.0f);
                } break;

                case OFFBEAT_VelocityRandom:
                {
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
                    UI::DragFloat3("Position", ParticleSystem->Motion.Point.Position.E, -INFINITY, INFINITY, 10.0f);
                    OffbeatExpressionF32("Strength", &ParticleSystem->Motion.Point.Strength, false);
                } break;
            }
        }

        if(UI::CollapsingHeader("Appearance", &s_OffbeatShowAppearance))
        {
            OffbeatExpressionOV4("Color", &ParticleSystem->Appearance.Color, true);
            OffbeatExpressionF32("Size", &ParticleSystem->Appearance.Size, true);

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
            f32 CyclesPerParticle = TotalParticleCount ? CycleCount / (float)TotalParticleCount : 0.0f;

            char CountBuffer[50];
            sprintf(CountBuffer, "Cycles: %llu", CycleCount);
            UI::Text(CountBuffer);

            char ParticleBuffer[50];
            sprintf(ParticleBuffer, "Particles: %llu", TotalParticleCount);
            UI::Text(ParticleBuffer);

            char PerParticleBuffer[50];
            sprintf(PerParticleBuffer, "%.2f cycles/particle", CyclesPerParticle);
            UI::Text(PerParticleBuffer);
        }
    }
    UI::EndWindow();
}
