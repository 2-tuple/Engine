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
            GameState->Camera.Position = vec3{ 0, 1, 7 };
            GameState->LastDrawCubemap = GameState->DrawCubemap;
            GameState->DrawCubemap = false;
            GameState->R.CurrentClearColor = GameState->R.ParticleSystemClearColor;
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

    if(Input->o.EndedDown && Input->o.Changed && GameState->ParticleMode)
    {
        GameState->DrawParticleCelGrid = !GameState->DrawParticleCelGrid;
    }

    if(!GameState->ParticleMode)
    {
        return false;
    }

    return true;
}

static void
ParticleSystemDrawGrid(game_state* GameState)
{
    for(u32 i = 0; i < GameState->SquareGridLineCount; ++i)
    {
        Debug::PushLine(GameState->SquareGridLines[i].A,
                        GameState->SquareGridLines[i].B,
                        GameState->SquareGridColor);
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
TruncateR32ToS32(r32 R32)
{
    return (s32)R32;
}

static void
ParticleSystemDrawParticleCels(game_state* GameState)
{
    for(u32 i = 0; i < GameState->ParticleCelGridLineCount; ++i)
    {
        Debug::PushLine(GameState->ParticleCelGridLines[i].A,
                        GameState->ParticleCelGridLines[i].B,
                        GameState->ParticleCelGridColor);
    }
}

/*
 * NOTE(rytis): OK, so, I did this particle simulation the way Casey Muratori did on
 * the Handmade Hero, but (at least so far) he focused more on the physics side of
 * the simulation (meaning, the created particle "fountain" is using more physics
 * calculations, thus being a more accurate simulation).
 * Yet I don't think this is the way the "effects" system should be working? In my case,
 * it doesn't really matter if the simulation is physically accurate, since the accuracy
 * might not be the thing that the artist might want (meaning the required physical properties
 * of the simulation might be radically different for every effect).
 * Customization is the key here.
 */
void
ParticleSystem(game_state* GameState, game_input* Input)
{
    if(!ParticleSystemHandleInput(GameState, Input))
    {
        return;
    }

    ParticleSystemDrawGrid(GameState);

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

            Particle->P = v3{RandomBetween(&GameState->EffectsEntropy, -0.05f, 0.05f),
                             0,
                             RandomBetween(&GameState->EffectsEntropy, -0.05f, 0.05f)};
            Particle->dP = v3{RandomBetween(&GameState->EffectsEntropy, -0.01f, 0.01f),
                              7.0f * RandomBetween(&GameState->EffectsEntropy, 0.7f, 1.0f),
                              RandomBetween(&GameState->EffectsEntropy, -0.01f, 0.01f)};
            Particle->ddP = v3{0.0f, -9.8f, 0.0f};
            Particle->Color = v4{RandomBetween(&GameState->EffectsEntropy, 0.45f, 0.5f),
                                 RandomBetween(&GameState->EffectsEntropy, 0.5f, 0.8f),
                                 RandomBetween(&GameState->EffectsEntropy, 0.8f, 1.0f),
                                 1.0f};
            Particle->dColor = v4{0.0f, 0.0f, 0.0f, -0.2f};
        }
    }

    ZeroMemory(GameState->ParticleCels, PARTICLE_CEL_DIM * PARTICLE_CEL_DIM * PARTICLE_CEL_DIM * sizeof(particle_cel));

    v3 GridOrigin = GameState->ParticleCelGridOrigin;
    r32 InvGridScale = 1.0f / GameState->ParticleCelGridScale;
    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
    {
        particle* Particle = GameState->Particles + ParticleIndex;

        v3 P = InvGridScale * (Particle->P - GridOrigin);

        s32 X = TruncateR32ToS32(P.X);
        s32 Y = TruncateR32ToS32(P.Y);
        s32 Z = TruncateR32ToS32(P.Z);

        if(X < 0) {X = 0;}
        if(X > (PARTICLE_CEL_DIM - 1)) {X = (PARTICLE_CEL_DIM - 1);}
        if(Y < 0) {Y = 0;}
        if(Y > (PARTICLE_CEL_DIM - 1)) {Y = (PARTICLE_CEL_DIM - 1);}
        if(Z < 0) {Z = 0;}
        if(Z > (PARTICLE_CEL_DIM - 1)) {Z = (PARTICLE_CEL_DIM - 1);}

        particle_cel* Cel = &GameState->ParticleCels[Z][Y][X];
        r32 Density = Particle->Color.A;
        Cel->Density += Density;
        Cel->VelocityTimesDensity += Density * Particle->dP;
    }

    if(GameState->DrawParticleCelGrid)
    {
        ParticleSystemDrawParticleCels(GameState);
    }

    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particles); ++ParticleIndex)
    {
        particle* Particle = GameState->Particles + ParticleIndex;

        if(GameState->UpdateParticles)
        {
            v3 P = InvGridScale * (Particle->P - GridOrigin);

            s32 X = TruncateR32ToS32(P.X);
            s32 Y = TruncateR32ToS32(P.Y);
            s32 Z = TruncateR32ToS32(P.Z);

            if(X < 1) {X = 0;}
            if(X > (PARTICLE_CEL_DIM - 2)) {X = (PARTICLE_CEL_DIM - 2);}
            if(Y < 1) {Y = 0;}
            if(Y > (PARTICLE_CEL_DIM - 2)) {Y = (PARTICLE_CEL_DIM - 2);}
            if(Z < 1) {Z = 0;}
            if(Z > (PARTICLE_CEL_DIM - 2)) {Z = (PARTICLE_CEL_DIM - 2);}

            particle_cel* CelCenter = &GameState->ParticleCels[Z][Y][X];
            particle_cel* CelUp = &GameState->ParticleCels[Z][Y + 1][X];
            particle_cel* CelDown = &GameState->ParticleCels[Z][Y - 1][X];
            particle_cel* CelFront = &GameState->ParticleCels[Z - 1][Y][X];
            particle_cel* CelBack = &GameState->ParticleCels[Z + 1][Y][X];
            particle_cel* CelLeft = &GameState->ParticleCels[Z][Y][X - 1];
            particle_cel* CelRight = &GameState->ParticleCels[Z][Y][X + 1];

            v3 Dispersion = {};
            r32 Dc = 1.0f;
            Dispersion += Dc * (CelCenter->Density - CelUp->Density) * vec3{0.0f, 1.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelDown->Density) * vec3{0.0f, -1.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelFront->Density) * vec3{0.0f, 0.0f, -1.0f};
            Dispersion += Dc * (CelCenter->Density - CelBack->Density) * vec3{0.0f, 0.0f, 1.0f};
            Dispersion += Dc * (CelCenter->Density - CelLeft->Density) * vec3{-1.0f, 0.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelRight->Density) * vec3{1.0f, 0.0f, 0.0f};

            v3 ddP = Particle->ddP + Dispersion;

            // NOTE(rytis): Simulate the particle forward in time
            Particle->P += 0.5f * Square(Input->dt) * ddP + Input->dt * Particle->dP;
            Particle->dP += Input->dt * ddP;
            Particle->Color += Particle->dColor * Input->dt;

            if(Particle->P.Y < 0.0f)
            {
                r32 CoefficientOfRestitution = 0.3f;
                r32 CoefficientOfFriction = 0.7f;
                Particle->P.Y = -Particle->P.Y;
                Particle->dP.Y = -CoefficientOfRestitution * Particle->dP.Y;
                Particle->dP.X = CoefficientOfFriction * Particle->dP.X;
                Particle->dP.Z = CoefficientOfFriction * Particle->dP.Z;
                Particle->Color += v4{-0.1f, 0.0f, 0.0f, 0.0f};
            }
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
