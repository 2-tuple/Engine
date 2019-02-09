#include "debug_drawing.h"
#include "particles.h"
#include "math_engine.h"
#include "random.h"
#include "profile.h"

typedef vec2 v2;
typedef vec3 v3;
typedef vec4 v4;
typedef mat3 m3;
typedef mat4 m4;

static b32
ParticleSystemHandleInput(game_state* GameState, game_input* Input)
{
    if(Input->p.EndedDown && Input->p.Changed)
    {
        GameState->ParticleMode = !GameState->ParticleMode;
        GameState->UpdateParticles = GameState->ParticleMode;

        if(GameState->ParticleMode)
        {
            GameState->LastCameraPosition = GameState->Camera.Position;
            GameState->Camera.Position = vec3{ 0, 1, 3 };
            GameState->LastDrawCubemap = GameState->DrawCubemap;
            GameState->DrawCubemap = false;
            GameState->R.CurrentClearColor = vec4{0.0f, 0.0f, 0.0f, 1.0f};
        }
        else
        {
            GameState->Camera.Position = GameState->LastCameraPosition;
            GameState->DrawCubemap = GameState->LastDrawCubemap;
            GameState->R.CurrentClearColor = GameState->R.DefaultClearColor;
        }
    }

    if(Input->q.EndedDown && Input->q.Changed && GameState->ParticleMode)
    {
        GameState->UpdateParticles = !GameState->UpdateParticles;
    }

    if(!GameState->ParticleMode)
    {
        return false;
    }

    return true;
}

void
ParticleSystem(game_state* GameState, game_input* Input)
{
    if(!ParticleSystemHandleInput(GameState, Input))
    {
        return;
    }

    TIMED_BLOCK(Particles);
    glEnable(GL_BLEND);

    if(GameState->UpdateParticles)
    {
        for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 4; ++ParticleSpawnIndex)
        {
            particle* Particle = GameState->Particles + GameState->NextParticle++;
            if(GameState->NextParticle >= ArrayCount(GameState->Particles))
            {
                GameState->NextParticle = 0;
            }

            Particle->P = v3{RandomBetween(&GameState->EffectsEntropy, -0.25f, 0.25f),
                             0,
                             RandomBetween(&GameState->EffectsEntropy, -0.25f, 0.25f)};
            Particle->dP = v3{RandomBetween(&GameState->EffectsEntropy, -0.5f, 0.5f),
                              RandomBetween(&GameState->EffectsEntropy, 0.7f, 1.0f),
                              RandomBetween(&GameState->EffectsEntropy, -0.5f, 0.5f)};
            Particle->Color = v4{RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                 RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                 RandomBetween(&GameState->EffectsEntropy, 0.75f, 1.0f),
                                 1.0f};
            Particle->dColor = v4{0.0f, 0.0f, 0.0f, -0.2f};
        }
    }

    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
    {
        particle* Particle = GameState->Particles + ParticleIndex;

        if(GameState->UpdateParticles)
        {
            // NOTE(rytis): Simulate the particle forward in time
            Particle->P += Particle->dP * Input->dt;
            Particle->Color += Particle->dColor * Input->dt;
        }

        v4 Color;
        Color.R = Clamp01(Particle->Color.R);
        Color.G = Clamp01(Particle->Color.G);
        Color.B = Clamp01(Particle->Color.B);
        Color.A = Clamp01(Particle->Color.A);

        if(Color.A > 0.9f)
        {
            Color.A = 0.9f * Clamp01MapToRange(1.0f, Color.A, 0.9f);
        }

        // NOTE(rytis): Render the particle (as a debug output for now)
        Debug::PushWireframeSphere(Particle->P, 0.01f, Color);
    }
    glDisable(GL_BLEND);
}
