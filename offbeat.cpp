#include "debug_drawing.h"
#include "offbeat.h"
#include "offbeat_math.h"

static void
OffbeatHandleInput(offbeat_state* OffbeatState, game_input* Input)
{
    OffbeatState->t += Input->dt;
    if(Input->q.EndedDown && Input->q.Changed)
    {
        OffbeatState->UpdateParticles = !OffbeatState->UpdateParticles;
    }

    if(Input->e.EndedDown && Input->e.Changed)
    {
        OffbeatState->DrawParticleCelGrid = !OffbeatState->DrawParticleCelGrid;
    }
}

static void
OffbeatDrawGrid(offbeat_state* OffbeatState)
{
    for(u32 i = 0; i < OffbeatState->SquareGridLineCount; ++i)
    {
        Debug::PushLine(OffbeatState->SquareGridLines[i].A,
                        OffbeatState->SquareGridLines[i].B,
                        OffbeatState->SquareGridColor);
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
OffbeatDrawParticleCels(offbeat_state* OffbeatState)
{
    for(u32 i = 0; i < OffbeatState->ParticleCelGridLineCount; ++i)
    {
        Debug::PushLine(OffbeatState->ParticleCelGridLines[i].A,
                        OffbeatState->ParticleCelGridLines[i].B,
                        OffbeatState->ParticleCelGridColor);
    }
}

offbeat_state*
OffbeatInit(void* Memory, u64 MemorySize)
{
    Assert(MemorySize >= sizeof(offbeat_state));
    offbeat_state* OffbeatState = (offbeat_state*)Memory;
    {
        OffbeatState->t = 0.0f;

        OffbeatState->EffectsEntropy = RandomSeed(1234);
        OffbeatState->UpdateParticles = true;
        OffbeatState->DrawParticleCelGrid = false;

        OffbeatState->SquareGridColor = v4{0.5f, 0.5f, 0.85f, 1.0f};
        OffbeatState->SquareGridLineCount = 0;
        OffbeatState->SquareGridStep = 2.0f;
        OffbeatState->SquareGridRange = 10.0f;
        r32 Limit = OffbeatState->SquareGridRange + 0.5f * OffbeatState->SquareGridStep;
        for(r32 Coordinate = -OffbeatState->SquareGridRange;
          Coordinate < Limit;
          Coordinate += OffbeatState->SquareGridStep)
        {
          OffbeatState->SquareGridLines[OffbeatState->SquareGridLineCount++] = offbeat_grid_line{
            v3{Coordinate, 0.0f, -OffbeatState->SquareGridRange},
            v3{Coordinate, 0.0f, OffbeatState->SquareGridRange}
          };
          OffbeatState->SquareGridLines[OffbeatState->SquareGridLineCount++] = offbeat_grid_line{
            v3{-OffbeatState->SquareGridRange, 0.0f, Coordinate},
            v3{OffbeatState->SquareGridRange, 0.0f, Coordinate}
          };
        }
        Assert(OffbeatState->SquareGridLineCount < MAX_SQUARE_GRID_LINE_COUNT);

        OffbeatState->ParticleCelGridScale = 0.25f;
        OffbeatState->ParticleCelGridOrigin =
          v3{-0.5f * PARTICLE_CEL_DIM * OffbeatState->ParticleCelGridScale,
               0.0f,
               -0.5f * PARTICLE_CEL_DIM * OffbeatState->ParticleCelGridScale};
        OffbeatState->ParticleCelGridColor = v4{0.85f, 0.3f, 0.3f, 0.1f};
        OffbeatState->ParticleCelGridLineCount = 0;
        v3 GridOrigin = OffbeatState->ParticleCelGridOrigin;
        for(u32 i = 0; i < PARTICLE_CEL_DIM + 1; ++i)
        {
          r32 Plane = i * OffbeatState->ParticleCelGridScale;
          for(u32 j = 0; j < PARTICLE_CEL_DIM + 1; ++j)
          {
              r32 PlaneLine = j * OffbeatState->ParticleCelGridScale;
              r32 LineLength = PARTICLE_CEL_DIM * OffbeatState->ParticleCelGridScale;
              // X
              OffbeatState->ParticleCelGridLines[OffbeatState->ParticleCelGridLineCount++] = offbeat_grid_line{
                  v3{GridOrigin.X, GridOrigin.Y + Plane, GridOrigin.Z + PlaneLine},
                  v3{GridOrigin.X + LineLength, GridOrigin.Y + Plane, GridOrigin.Z + PlaneLine}
              };
              // Z
              OffbeatState->ParticleCelGridLines[OffbeatState->ParticleCelGridLineCount++] = offbeat_grid_line{
                  v3{GridOrigin.X + PlaneLine, GridOrigin.Y + Plane, GridOrigin.Z},
                  v3{GridOrigin.X + PlaneLine, GridOrigin.Y + Plane, GridOrigin.Z + LineLength}
              };
              // Y
              OffbeatState->ParticleCelGridLines[OffbeatState->ParticleCelGridLineCount++] = offbeat_grid_line{
                  v3{GridOrigin.X + Plane, GridOrigin.Y, GridOrigin.Z + PlaneLine},
                  v3{GridOrigin.X + Plane, GridOrigin.Y + LineLength, GridOrigin.Z + PlaneLine}
              };
          }
        }
    }
    return OffbeatState;
}

/*
 * NOTE(rytis): OK, so, I did this particle simulation the way Casey Muratori did on
 * the Handmade Hero, but (at least so far) he focused more on the physics side of
 * the simulation (meaning, the created particle "fountain" is using more accurate
 * physics calculations, thus being a more realistic simulation).
 * I don't know if the density calculations are needed for this project. Destiny
 * particle system, which is a big inspiration for this project, uses attractors and
 * repulsors (which, I believe, are based on Newtonian gravity calculations) and
 * I don't know if I need anything fancier than that.
 */
void
OffbeatParticleSystem(offbeat_state* OffbeatState, game_input* Input)
{
    OffbeatHandleInput(OffbeatState, Input);
    OffbeatDrawGrid(OffbeatState);

    glEnable(GL_BLEND);

    if(OffbeatState->UpdateParticles)
    {
        for(u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 4; ++ParticleSpawnIndex)
        {
            offbeat_particle* Particle = OffbeatState->Particles + OffbeatState->NextParticle++;
            if(OffbeatState->NextParticle >= ArrayCount(OffbeatState->Particles))
            {
                OffbeatState->NextParticle = 0;
            }

            Particle->P = v3{RandomBetween(&OffbeatState->EffectsEntropy, -0.05f, 0.05f),
                             0,
                             RandomBetween(&OffbeatState->EffectsEntropy, -0.05f, 0.05f)};
            Particle->dP = v3{RandomBetween(&OffbeatState->EffectsEntropy, -0.01f, 0.01f),
                              7.0f * RandomBetween(&OffbeatState->EffectsEntropy, 0.7f, 1.0f),
                              RandomBetween(&OffbeatState->EffectsEntropy, -0.01f, 0.01f)};
            Particle->ddP = v3{0.0f, -9.8f, 0.0f};
            Particle->Color = v4{RandomBetween(&OffbeatState->EffectsEntropy, 0.45f, 0.5f),
                                 RandomBetween(&OffbeatState->EffectsEntropy, 0.5f, 0.8f),
                                 RandomBetween(&OffbeatState->EffectsEntropy, 0.8f, 1.0f),
                                 1.0f};
            Particle->dColor = v4{0.0f, 0.0f, 0.0f, -0.2f};
        }
    }

    ZeroMemory(OffbeatState->ParticleCels, PARTICLE_CEL_DIM * PARTICLE_CEL_DIM * PARTICLE_CEL_DIM * sizeof(offbeat_particle_cel));

    v3 GridOrigin = OffbeatState->ParticleCelGridOrigin;
    r32 InvGridScale = 1.0f / OffbeatState->ParticleCelGridScale;
    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(OffbeatState->Particles); ++ParticleIndex)
    {
        offbeat_particle* Particle = OffbeatState->Particles + ParticleIndex;

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

        offbeat_particle_cel* Cel = &OffbeatState->ParticleCels[Z][Y][X];
        r32 Density = Particle->Color.A;
        Cel->Density += Density;
        Cel->VelocityTimesDensity += Density * Particle->dP;
    }

    if(OffbeatState->DrawParticleCelGrid)
    {
        OffbeatDrawParticleCels(OffbeatState);
    }

    v3 Attractor = v3{-3.0f * sinf(0.25f * PI * OffbeatState->t), 1.0f, 1.0f};
    v4 AttractorColor = v4{1.0f, 0.0f, 0.0f, 1.0f};

    Debug::PushWireframeSphere(Attractor, 0.05f, AttractorColor);

    for(u32 ParticleIndex = 0; ParticleIndex < ArrayCount(OffbeatState->Particles); ++ParticleIndex)
    {
        offbeat_particle* Particle = OffbeatState->Particles + ParticleIndex;

        if(OffbeatState->UpdateParticles)
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

            offbeat_particle_cel* CelCenter = &OffbeatState->ParticleCels[Z][Y][X];
            offbeat_particle_cel* CelUp = &OffbeatState->ParticleCels[Z][Y + 1][X];
            offbeat_particle_cel* CelDown = &OffbeatState->ParticleCels[Z][Y - 1][X];
            offbeat_particle_cel* CelFront = &OffbeatState->ParticleCels[Z - 1][Y][X];
            offbeat_particle_cel* CelBack = &OffbeatState->ParticleCels[Z + 1][Y][X];
            offbeat_particle_cel* CelLeft = &OffbeatState->ParticleCels[Z][Y][X - 1];
            offbeat_particle_cel* CelRight = &OffbeatState->ParticleCels[Z][Y][X + 1];

            v3 Dispersion = {};
            r32 Dc = 1.0f;
            Dispersion += Dc * (CelCenter->Density - CelUp->Density) * v3{0.0f, 1.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelDown->Density) * v3{0.0f, -1.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelFront->Density) * v3{0.0f, 0.0f, -1.0f};
            Dispersion += Dc * (CelCenter->Density - CelBack->Density) * v3{0.0f, 0.0f, 1.0f};
            Dispersion += Dc * (CelCenter->Density - CelLeft->Density) * v3{-1.0f, 0.0f, 0.0f};
            Dispersion += Dc * (CelCenter->Density - CelRight->Density) * v3{1.0f, 0.0f, 0.0f};

            r32 Strength = 10.0f;
            v3 Direction = Attractor - Particle->P;
            r32 Distance = Math::Length(Direction);
            v3 ddPAttraction = Strength * Math::Normalized(Direction) / (Distance * Distance);

            v3 ddP = Particle->ddP + Dispersion + ddPAttraction;

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
