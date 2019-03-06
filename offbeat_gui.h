#pragma once

#include "ui.h"
#include "offbeat.h"

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

void
OffbeatWindow(game_state* GameState, const game_input* Input)
{
    ob_state* OffbeatState = GameState->OffbeatState;
    static ob_particle_system* ParticleSystem = &OffbeatState->ParticleSystems[0];
    UI::BeginWindow("Offbeat Window", {25, 25}, {500, 800});
    {
        static u32 s_OffbeatCurrentParticleSystem = 0;
        static s32 s_OffbeatCurrentFile = 0;

        static bool s_OffbeatShowEmission = false;
        static bool s_OffbeatShowMotion = false;
        static bool s_OffbeatShowAppearance = false;

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
        if(UI::Button("Save Particle System"))
        {
        }

        // TODO(rytis): Combo box here for file loading.
        // UI::Combo("OPB files", &s_OffbeatCurrentFile, );

        if(UI::CollapsingHeader("Emission", &s_OffbeatShowEmission))
        {
            UI::DragFloat3("Location", ParticleSystem->Emission.Location.E, -INFINITY, INFINITY, 10.0f);
            UI::DragFloat("Emission Rate", &ParticleSystem->Emission.EmissionRate, 0.0f, INFINITY, 200.0f);
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
                    UI::DragFloat("Ring Radius", &ParticleSystem->Emission.Shape.Ring.Radius, 0.0f, INFINITY, 200.0f);
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
                    UI::DragFloat("Cone Height", &ParticleSystem->Emission.Velocity.Cone.Height, 0.0f, INFINITY, 200.0f);
                    UI::DragFloat("Cone Radius", &ParticleSystem->Emission.Velocity.Cone.Radius, 0.0f, INFINITY, 200.0f);
                } break;

                case OFFBEAT_VelocityRandom:
                {
                } break;
            }
        }

        if(UI::CollapsingHeader("Motion", &s_OffbeatShowMotion))
        {
            UI::DragFloat3("Gravity", ParticleSystem->Motion.Gravity.E, -INFINITY, INFINITY, 10.0f);

            ob_motion_primitive* CurrentPrimitive = &ParticleSystem->Motion.Primitive;
            UI::Combo("Motion Primitive", (int*)CurrentPrimitive, g_MotionPrimitiveStrings, OFFBEAT_MotionCount, UI::StringArrayToString);

            switch(*CurrentPrimitive)
            {
                case OFFBEAT_MotionPoint:
                {
                    UI::DragFloat3("Position", ParticleSystem->Motion.Point.Position.E, -INFINITY, INFINITY, 10.0f);
                    UI::DragFloat("Strength", &ParticleSystem->Motion.Point.Strength, -INFINITY, INFINITY, 200.0f);
                } break;
            }
        }

        if(UI::CollapsingHeader("Appearance", &s_OffbeatShowAppearance))
        {
            UI::DragFloat4("Color", ParticleSystem->Appearance.Color.E, 0.0f, INFINITY, 5.0f);
            UI::DragFloat("Size", &ParticleSystem->Appearance.Size, 0.0f, INFINITY, 5.0f);

            static ob_texture_type CurrentTexture = OffbeatGetCurrentTextureType(ParticleSystem);
            ob_texture_type NewTexture = CurrentTexture;
            UI::Combo("Texture", (int*)&NewTexture, g_TextureStrings, OFFBEAT_TextureCount, UI::StringArrayToString);
            if(NewTexture != CurrentTexture)
            {
                CurrentTexture = NewTexture;
                OffbeatSetTextureID(ParticleSystem, CurrentTexture);
            }
        }
    }
    UI::EndWindow();
}
