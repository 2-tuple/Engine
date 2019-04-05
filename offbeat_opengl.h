#pragma once

#ifdef OFFBEAT_OPENGL_COMPUTE
static GLuint GlobalRandomTable;
static GLuint GlobalEmissionBlock;
static GLuint GlobalMotionBlock;
static GLuint GlobalAppearanceBlock;
static GLuint GlobalUpdatedParticleCount;
static GLuint GlobalOldParticleBinding;
static GLuint GlobalUpdatedParticleBinding;

static GLuint
OffbeatCompileAndLinkComputeProgram(char* HeaderCode, char* ComputeCode)
{
    GLuint ComputeShaderID = glCreateShader(GL_COMPUTE_SHADER);
    GLchar* ComputeShaderCode[] =
    {
        HeaderCode,
        ComputeCode,
    };
    glShaderSource(ComputeShaderID, OffbeatArrayCount(ComputeShaderCode), ComputeShaderCode, 0);
    glCompileShader(ComputeShaderID);

    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, ComputeShaderID);
    glLinkProgram(ProgramID);

    glValidateProgram(ProgramID);
    GLint Linked = false;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Linked);
    if(!Linked)
    {
        GLsizei Ignored;
        char ComputeErrors[4096];
        char ProgramErrors[4096];
        glGetShaderInfoLog(ComputeShaderID, sizeof(ComputeErrors), &Ignored, ComputeErrors);
        glGetProgramInfoLog(ProgramID, sizeof(ProgramErrors), &Ignored, ProgramErrors);

        OffbeatAssert(!"Shader validation failed");
    }
    return ProgramID;
}

static void
OffbeatCreateComputePrograms(GLuint* SpawnProgram, GLuint* UpdateProgram, GLuint* StatelessEvaluationProgram)
{
    // TODO(rytis): IMPORTANT: Seems, like using work groups is a MAJOR performance boost. DO IT!!!
    char* HeaderCode = R"SHADER(
    #version 430 core

    #define PI 3.14159265358979323846

    #define OFFBEAT_FunctionConst 0
    #define OFFBEAT_FunctionLerp 1
    #define OFFBEAT_FunctionSmoothstep 2
    #define OFFBEAT_FunctionSquared 3
    #define OFFBEAT_FunctionCubed 4
    #define OFFBEAT_FunctionFourth 5
    #define OFFBEAT_FunctionTriangle 6
    #define OFFBEAT_FunctionTwoTriangles 7
    #define OFFBEAT_FunctionFourTriangles 8
    #define OFFBEAT_FunctionPeriodic 9

    #define OFFBEAT_ParameterAge 0
    #define OFFBEAT_ParameterRandom 1
    #define OFFBEAT_ParameterCameraDistance 2
    #define OFFBEAT_ParameterID 3

    #define OFFBEAT_EmissionPoint 0
    #define OFFBEAT_EmissionRing 1

    #define OFFBEAT_VelocityRandom 0
    #define OFFBEAT_VelocityCone 1

    #define OFFBEAT_MotionNone 0
    #define OFFBEAT_MotionPoint 1
    #define OFFBEAT_MotionLine 2

    #define OFFBEAT_MIN_RANDOM_NUMBER 0x0000cc93
    #define OFFBEAT_MAX_RANDOM_NUMBER 0x3b8a2a26
    #define OFFBEAT_RANDOM_TABLE_SIZE 4096

    layout(local_size_x = 64) in;
    layout(row_major) uniform;
    layout(std430) buffer;

    uniform float u_t;
    uniform float dt;
    uniform uint RunningParticleID;
    uniform vec3 CameraPosition;
    uniform uint ParticleSpawnCount;
    uniform uint ParticleCount;
    uniform uint OldParticleCount;
    uniform uint RandomTableIndex;

    layout(binding = 0) readonly buffer random
    {
        uint RandomTable[OFFBEAT_RANDOM_TABLE_SIZE];
    };

    struct ob_particle
    {
        vec4 PositionAge;
        vec4 VelocityID;
        vec4 RandomCamera;
    };

    struct ob_expr
    {
        vec4 FuncParamFreq;
        vec4 Low;
        vec4 High;
    };

    layout(std140, binding = 0) uniform ob_emission
    {
        vec3 Location;
        float EmissionRate;
        ob_expr ParticleLifetime;

        uint Shape;
        float RingRadius;
        vec3 RingNormal;
        mat3 RingRotation;

        float InitialVelocityScale;

        uint VelocityType;
        float ConeHeight;
        float ConeRadius;
        vec3 ConeDirection;
        mat3 ConeRotation;
    } Emission;

    layout(std140, binding = 1) uniform ob_motion
    {
        vec3 Gravity;
        float Drag;
        ob_expr Strength;

        uint Primitive;
        vec3 Position;
        vec3 LineDirection;
    } Motion;

    layout(std140, binding = 2) uniform ob_appearance
    {
        ob_expr Color;
        ob_expr Size;
    } Appearance;

    struct ob_globals
    {
        vec3 EmissionLocation;
        float EmissionParticleLifetime;
        uint EmissionShape;
        float EmissionRingRadius;
        vec3 EmissionRingNormal;
        mat3 EmissionRingRotation;
        float EmissionInitialVelocity;
        uint EmissionVelocityType;
        float EmissionConeHeight;
        float EmissionConeRadius;
        vec3 EmissionConeDirection;
        mat3 EmissionConeRotation;

        vec3 MotionGravity;
        float MotionDrag;
        float MotionStrength;
        uint MotionPrimitive;
        vec3 MotionPosition;
        vec3 MotionLineDirection;

        vec4 AppearanceColor;
        float AppearanceSize;
    } Globals;

    uint Index;
    uint Offset;
    uint UID;

    float
    ObRandom()
    {
        float Divisor = 1.0f / float(OFFBEAT_MAX_RANDOM_NUMBER);
        uint RandomIndex = (RandomTableIndex + UID + Offset) % OFFBEAT_RANDOM_TABLE_SIZE;
        Offset += 1;
        return Divisor * float(RandomTable[RandomIndex]);
    }

    float
    ObLengthSq(vec3 Vector)
    {
        return dot(Vector, Vector);
    }

    vec3
    ObNOZ(vec3 Vector)
    {
        vec3 Result = vec3(0.0f);
        if(ObLengthSq(Vector) > (0.0001f * 0.0001f))
        {
            Result = normalize(Vector);
        }
        return Result;
    }

    float
    OffbeatEvaluateParameter(uint Parameter, ob_particle Particle)
    {
        float Result = 0.0f;

        switch(Parameter)
        {
            case OFFBEAT_ParameterAge:
            {
                if(Particle.PositionAge.w == 0.0f)
                {
                    return Result;
                }
                Result = Particle.PositionAge.w / Emission.ParticleLifetime.High.x;
            } break;

            case OFFBEAT_ParameterRandom:
            {
                Result = Particle.RandomCamera.x;
            } break;

            case OFFBEAT_ParameterCameraDistance:
            {
                Result = Particle.RandomCamera.y;
            } break;

            case OFFBEAT_ParameterID:
            {
                Result = Particle.VelocityID.w;
            } break;
        }

        return Result;
    }

    vec4
    OffbeatEvaluateExpression(ob_expr Expr, ob_particle Particle)
    {
        uint Function = floatBitsToUint(Expr.FuncParamFreq.x);
        uint Parameter = floatBitsToUint(Expr.FuncParamFreq.y);
        float Frequency = Expr.FuncParamFreq.z;

        if(Function == OFFBEAT_FunctionPeriodic)
        {
            return mix(Expr.Low, Expr.High, 0.5f * (sin(2.0f * PI * Frequency * u_t) + 1.0f));
        }

        float Param = clamp(OffbeatEvaluateParameter(Parameter, Particle), 0.0f, 1.0f);
        switch(Function)
        {
            case OFFBEAT_FunctionConst:
            {
                return Expr.High;
            } break;

            case OFFBEAT_FunctionLerp:
            {
            } break;

            case OFFBEAT_FunctionSmoothstep:
            {
                Param = Param * Param * (3.0f - 2.0f * Param);
            } break;

            case OFFBEAT_FunctionSquared:
            {
                Param *= Param;
            } break;

            case OFFBEAT_FunctionCubed:
            {
                Param = Param * Param * Param;
            } break;

            case OFFBEAT_FunctionFourth:
            {
                Param *= Param;
                Param *= Param;
            } break;

            case OFFBEAT_FunctionTriangle:
            {
                Param = 1.0f - abs(2.0f * Param - 1.0f);
            } break;

            case OFFBEAT_FunctionTwoTriangles:
            {
                Param = 1.0f - abs(2.0f * Param - 1.0f);
                Param = 1.0f - abs(2.0f * Param - 1.0f);
            } break;

            case OFFBEAT_FunctionFourTriangles:
            {
                Param = 1.0f - abs(2.0f * Param - 1.0f);
                Param = 1.0f - abs(2.0f * Param - 1.0f);
                Param = 1.0f - abs(2.0f * Param - 1.0f);
            } break;

            default:
            {
                return vec4(0.0f);
            } break;
        }

        return mix(Expr.Low, Expr.High, Param);
    }

    mat3
    ObRotationAlign(vec3 Start, vec3 Destination)
    {
        if((length(Start) == 0.0f) || (length(Destination) == 0.0f))
        {
            return mat3(1.0f);
        }

        // IMPORTANT(rytis): We assume that Start and Destination are normalized already.
        // Start = normalize(Start);
        // Destination = normalize(Destination);

        vec3 Axis = cross(Start, Destination);
        float CosTheta = dot(Start, Destination);
        float K = 1.0f / (1.0f + CosTheta);

        mat3 Result;

#if 1
        vec3 Col0 = vec3(CosTheta, Axis.z, -Axis.y);
        vec3 Col1 = vec3(-Axis.z, CosTheta, Axis.x);
        vec3 Col2 = vec3(Axis.y, -Axis.x, CosTheta);
        Result[0] = Axis.xxx * Axis.xyz * K + Col0;
        Result[1] = Axis.yyy * Axis.xyz * K + Col1;
        Result[2] = Axis.zzz * Axis.xyz * K + Col2;
#else
        Result[0][0] = Axis.x * Axis.x * K + CosTheta;
        Result[0][1] = Axis.x * Axis.y * K + Axis.z;
        Result[0][2] = Axis.x * Axis.z * K - Axis.y;

        Result[1][0] = Axis.y * Axis.x * K - Axis.z;
        Result[1][1] = Axis.y * Axis.y * K + CosTheta;
        Result[1][2] = Axis.y * Axis.z * K + Axis.x;

        Result[2][0] = Axis.z * Axis.x * K + Axis.y;
        Result[2][1] = Axis.z * Axis.y * K - Axis.x;
        Result[2][2] = Axis.z * Axis.z * K + CosTheta;
#endif

        return Result;
    }

    void
    OffbeatUpdateRotationMatrices()
    {
        vec3 Z = vec3(0.0f, 0.0f, 1.0f);

        // NOTE(rytis): Assuming all used vectors are normalized.
        switch(Globals.EmissionShape)
        {
            case OFFBEAT_EmissionRing:
            {
                // Globals.EmissionRingNormal = ObNOZ(Globals.EmissionRingNormal);
                Globals.EmissionRingRotation = ObRotationAlign(Z, Globals.EmissionRingNormal);
            } break;
        }

        switch(Globals.EmissionVelocityType)
        {
            case OFFBEAT_VelocityCone:
            {
                // Globals.EmissionConeDirection = ObNOZ(Globals.EmissionConeDirection);
                Globals.EmissionConeRotation = ObRotationAlign(Z, Globals.EmissionConeDirection);
            } break;
        }

        /*
        switch(Globals.MotionPrimitive)
        {
            case OFFBEAT_MotionLine:
            {
                Globals.MotionLineDirection = ObNOZ(Globals.MotionLineDirection);
            } break;
        }
        */
    }
    )SHADER";

    char* SpawnCode = R"SHADER(
    layout(binding = 1) writeonly buffer particle_buffer
    {
        ob_particle Particles[];
    };

    vec3
    OffbeatParticleInitialPosition()
    {
        vec3 Result = vec3(0.0f);

        switch(Globals.EmissionShape)
        {
            case OFFBEAT_EmissionPoint:
            {
                Result = Globals.EmissionLocation;
            } break;

            case OFFBEAT_EmissionRing:
            {
                float RandomValue = 2.0f * PI * ObRandom();
                Result.x = Globals.EmissionRingRadius * sin(RandomValue);
                Result.y = Globals.EmissionRingRadius * cos(RandomValue);

                Result = Globals.EmissionRingRotation * Result;

                Result += Globals.EmissionLocation;
            } break;
        }

        return Result;
    }

    vec3
    OffbeatParticleInitialVelocity()
    {
        vec3 Result = vec3(0.0f);

        switch(Globals.EmissionVelocityType)
        {
            case OFFBEAT_VelocityRandom:
            {
                float Theta = 2.0f * PI * ObRandom();
                float Z = 2.0f * ObRandom() - 1.0f;

                Result.x = sqrt(1.0f - (Z * Z)) * cos(Theta);
                Result.y = sqrt(1.0f - (Z * Z)) * sin(Theta);
                Result.z = Z;
            } break;

            case OFFBEAT_VelocityCone:
            {
                float Denom = sqrt(Globals.EmissionConeHeight * Globals.EmissionConeHeight +
                                   Globals.EmissionConeRadius * Globals.EmissionConeRadius);
                float CosTheta = (Denom > 0.0f) ? (Globals.EmissionConeHeight / Denom) : 0.0f;

                float Phi = 2.0f * PI * ObRandom();
                float Z = mix(CosTheta, 1.0f, ObRandom());

                vec3 RandomVector = vec3(0.0f);
                RandomVector.x = sqrt(1.0f - (Z * Z)) * cos(Phi);
                RandomVector.y = sqrt(1.0f - (Z * Z)) * sin(Phi);
                RandomVector.z = Z;

                Result = Globals.EmissionConeRotation * RandomVector;
            } break;
        }

        Result *= Globals.EmissionInitialVelocity;

        return Result;
    }

    void
    main()
    {
        Index = gl_GlobalInvocationID.x;
        if(Index < ParticleSpawnCount)
        {
            Offset = 0;
            UID = RunningParticleID + Index;

            ob_particle ZeroP = ob_particle(vec4(0.0f), vec4(0.0f), vec4(0.0f));

            Globals.EmissionLocation = Emission.Location;
            Globals.EmissionParticleLifetime = OffbeatEvaluateExpression(Emission.ParticleLifetime, ZeroP).x;
            Globals.EmissionShape = Emission.Shape;
            Globals.EmissionRingRadius = Emission.RingRadius;
            Globals.EmissionRingNormal = Emission.RingNormal;
            // Globals.EmissionRingRotation = Emission.RingRotation;
            Globals.EmissionInitialVelocity = Emission.InitialVelocityScale;
            Globals.EmissionVelocityType = Emission.VelocityType;
            Globals.EmissionConeHeight = Emission.ConeHeight;
            Globals.EmissionConeRadius = Emission.ConeRadius;
            Globals.EmissionConeDirection = Emission.ConeDirection;
            // Globals.EmissionConeRotation = Emission.ConeRotation;
            OffbeatUpdateRotationMatrices();

            vec3 Position = OffbeatParticleInitialPosition();
            vec3 Velocity = OffbeatParticleInitialVelocity();
            float Random = ObRandom();
            float ID = float(UID);
            ob_particle Particle = ob_particle(
                                   vec4(Position, 0.0f),
                                   vec4(Velocity, ID),
                                   vec4(Random, 0.0f, 0.0f, 0.0f));
            Particles[Index] = Particle;
        }
    }
    )SHADER";

    char* UpdateCode = R"SHADER(
    layout(binding = 0) uniform atomic_uint UpdatedParticleCount;
    layout(binding = 2) readonly buffer particle_input_buffer
    {
        ob_particle InputParticles[];
    };

    layout(binding = 1) writeonly buffer particle_output_buffer
    {
        ob_particle OutputParticles[];
    };

    vec3
    OffbeatUpdateParticleAcceleration(ob_particle Particle)
    {
        vec3 Position = Particle.PositionAge.xyz;
        vec3 Velocity = Particle.VelocityID.xyz;
        float LengthSq = ObLengthSq(Velocity);
        vec3 Drag = Globals.MotionDrag * LengthSq * normalize(-Velocity);
        vec3 Result = Globals.MotionGravity + Drag;

        vec3 Direction = vec3(0.0f);

        switch(Globals.MotionPrimitive)
        {
            case OFFBEAT_MotionPoint:
            {
                Direction = Globals.MotionPosition - Position;
            } break;

            case OFFBEAT_MotionLine:
            {
                float t = dot(Position - Globals.MotionPosition, Globals.MotionLineDirection) /
                          ObLengthSq(Globals.MotionLineDirection);
                Direction = (Globals.MotionPosition + t * Globals.MotionLineDirection) - Position;
            } break;
        }
        Result += Globals.MotionStrength * ObNOZ(Direction);

        return Result;
    }

    void
    main()
    {
        Index = gl_GlobalInvocationID.x;
        if(Index < OldParticleCount)
        {
            ob_particle ZeroP = ob_particle(vec4(0.0f), vec4(0.0f), vec4(0.0f));
            ob_particle Particle = InputParticles[Index];

            Particle.PositionAge.w += dt;

            Globals.EmissionParticleLifetime = OffbeatEvaluateExpression(Emission.ParticleLifetime, ZeroP).x;
            float Age = Particle.PositionAge.w;
            if(Age < Globals.EmissionParticleLifetime)
            {
                Globals.MotionGravity = Motion.Gravity;
                Globals.MotionDrag = Motion.Drag;
                Globals.MotionStrength = OffbeatEvaluateExpression(Motion.Strength, Particle).x;
                Globals.MotionPrimitive = Motion.Primitive;
                Globals.MotionPosition = Motion.Position;
                Globals.MotionLineDirection = Motion.LineDirection;

                vec3 Acceleration = OffbeatUpdateParticleAcceleration(Particle);
                vec3 Position = Particle.PositionAge.xyz + 0.5f * dt * dt * Acceleration +
                                dt * Particle.VelocityID.xyz;
                vec3 Velocity = Particle.VelocityID.xyz + dt * Acceleration;

                uint NewIndex = ParticleSpawnCount + atomicCounterIncrement(UpdatedParticleCount);
                OutputParticles[NewIndex] = ob_particle(
                                            vec4(Position, Age),
                                            vec4(Velocity, Particle.VelocityID.w),
                                            Particle.RandomCamera);
            }
        }
    }
    )SHADER";

    char* StatelessEvaluationCode = R"SHADER(
    uniform vec3 Horizontal;
    uniform vec3 Vertical;

    layout(binding = 1) readonly buffer particles
    {
        ob_particle Particles[];
    };

    layout(binding = 3) writeonly buffer indices
    {
        uint Indices[];
    };

    struct ob_draw_vertex
    {
        vec3 Position;
        vec2 UV;
        vec4 Color;
    };

    layout(binding = 4) writeonly buffer vertices
    {
        ob_draw_vertex Vertices[];
    };

    void
    main()
    {
        Index = gl_GlobalInvocationID.x;
        if(Index < ParticleCount)
        {
            ob_particle ZeroP = ob_particle(vec4(0.0f), vec4(0.0f), vec4(0.0f));
            ob_particle Particle = Particles[Index];

            Globals.EmissionParticleLifetime = OffbeatEvaluateExpression(Emission.ParticleLifetime, ZeroP).x;
            Globals.AppearanceColor = OffbeatEvaluateExpression(Appearance.Color, Particle);
            Globals.AppearanceSize = OffbeatEvaluateExpression(Appearance.Size, Particle).x;

            // NOTE(rytis): Vertices (facing the camera plane)
            vec3 BottomLeft = Particle.PositionAge.xyz +
                              0.5f * Globals.AppearanceSize * (-Horizontal - Vertical);
            vec3 BottomRight = Particle.PositionAge.xyz +
                               0.5f * Globals.AppearanceSize * (Horizontal - Vertical);
            vec3 TopRight = Particle.PositionAge.xyz +
                            0.5f * Globals.AppearanceSize * (Horizontal + Vertical);
            vec3 TopLeft = Particle.PositionAge.xyz +
                           0.5f * Globals.AppearanceSize * (-Horizontal + Vertical);

            // NOTE(rytis): UVs
            vec2 BottomLeftUV = vec2(0.0f, 0.0f);
            vec2 BottomRightUV = vec2(1.0f, 0.0f);
            vec2 TopRightUV = vec2(1.0f, 1.0f);
            vec2 TopLeftUV = vec2(0.0f, 1.0f);

            uint VertexIndex0 = 4 * Index;
            uint VertexIndex1 = 4 * Index + 1;
            uint VertexIndex2 = 4 * Index + 2;
            uint VertexIndex3 = 4 * Index + 3;
            // NOTE(rytis): Updating draw list vertex array
            Vertices[VertexIndex0] = ob_draw_vertex(BottomLeft, BottomLeftUV, Globals.AppearanceColor);
            Vertices[VertexIndex1] = ob_draw_vertex(BottomRight, BottomRightUV, Globals.AppearanceColor);
            Vertices[VertexIndex2] = ob_draw_vertex(TopRight, TopRightUV, Globals.AppearanceColor);
            Vertices[VertexIndex3] = ob_draw_vertex(TopLeft, TopLeftUV, Globals.AppearanceColor);

            // NOTE(rytis): Updating draw list index array
            uint Index0 = 6 * Index;
            uint Index1 = 6 * Index + 1;
            uint Index2 = 6 * Index + 2;
            uint Index3 = 6 * Index + 3;
            uint Index4 = 6 * Index + 4;
            uint Index5 = 6 * Index + 5;
            // NOTE(rytis): CCW bottom right triangle
            Indices[Index0] = VertexIndex0;
            Indices[Index1] = VertexIndex1;
            Indices[Index2] = VertexIndex2;
            // NOTE(rytis): CCW top left triangle
            Indices[Index3] = VertexIndex0;
            Indices[Index4] = VertexIndex2;
            Indices[Index5] = VertexIndex3;
        }
    }
    )SHADER";

    *SpawnProgram = OffbeatCompileAndLinkComputeProgram(HeaderCode, SpawnCode);
    *UpdateProgram = OffbeatCompileAndLinkComputeProgram(HeaderCode, UpdateCode);
    *StatelessEvaluationProgram = OffbeatCompileAndLinkComputeProgram(HeaderCode, StatelessEvaluationCode);
}

void
OffbeatComputeInit()
{
    glGenBuffers(1, &GlobalUpdatedParticleCount);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, GlobalUpdatedParticleCount);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(u32), 0, GL_STREAM_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, GlobalUpdatedParticleCount);

    glGenBuffers(1, &GlobalEmissionBlock);
    glBindBuffer(GL_UNIFORM_BUFFER, GlobalEmissionBlock);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ob_emission_uniform_aligned), 0, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, GlobalEmissionBlock);

    glGenBuffers(1, &GlobalMotionBlock);
    glBindBuffer(GL_UNIFORM_BUFFER, GlobalMotionBlock);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ob_motion_uniform_aligned), 0, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, GlobalMotionBlock);

    glGenBuffers(1, &GlobalAppearanceBlock);
    glBindBuffer(GL_UNIFORM_BUFFER, GlobalAppearanceBlock);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ob_appearance_uniform_aligned), 0, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, GlobalAppearanceBlock);

    glGenBuffers(1, &GlobalRandomTable);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, GlobalRandomTable);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 4096 * sizeof(u32), ObGetRandomNumberTable(),
                 GL_STREAM_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, GlobalRandomTable);

    GlobalUpdatedParticleBinding = 1;
    GlobalOldParticleBinding = 2;
}

static ob_emission_uniform_aligned
OffbeatTranslateEmission(ob_emission* Emission)
{
    ob_emission_uniform_aligned Result = {};
    Result.Location = Emission->Location;
    Result.EmissionRate = Emission->EmissionRate;
    Result.ParticleLifetime = Emission->ParticleLifetime;

    Result.Shape = Emission->Shape;
    Result.RingRadius = Emission->RingRadius;
    Result.RingNormal = Emission->RingNormal;
    {
        Result.RingRotation._11 = Emission->RingRotation._11;
        Result.RingRotation._12 = Emission->RingRotation._12;
        Result.RingRotation._13 = Emission->RingRotation._13;
        Result.RingRotation._21 = Emission->RingRotation._21;
        Result.RingRotation._22 = Emission->RingRotation._22;
        Result.RingRotation._23 = Emission->RingRotation._23;
        Result.RingRotation._31 = Emission->RingRotation._31;
        Result.RingRotation._32 = Emission->RingRotation._32;
        Result.RingRotation._33 = Emission->RingRotation._33;
    }

    Result.InitialVelocityScale = Emission->InitialVelocityScale;

    Result.VelocityType = Emission->VelocityType;
    Result.ConeHeight = Emission->ConeHeight;
    Result.ConeRadius = Emission->ConeRadius;
    Result.ConeDirection = Emission->ConeDirection;
    {
        Result.ConeRotation._11 = Emission->ConeRotation._11;
        Result.ConeRotation._12 = Emission->ConeRotation._12;
        Result.ConeRotation._13 = Emission->ConeRotation._13;
        Result.ConeRotation._21 = Emission->ConeRotation._21;
        Result.ConeRotation._22 = Emission->ConeRotation._22;
        Result.ConeRotation._23 = Emission->ConeRotation._23;
        Result.ConeRotation._31 = Emission->ConeRotation._31;
        Result.ConeRotation._32 = Emission->ConeRotation._32;
        Result.ConeRotation._33 = Emission->ConeRotation._33;
    }

    return Result;
}

static ob_motion_uniform_aligned
OffbeatTranslateMotion(ob_motion* Motion)
{
    ob_motion_uniform_aligned Result = {};

    Result.Gravity = Motion->Gravity;
    Result.Drag = Motion->Drag;
    Result.Strength = Motion->Strength;

    Result.Primitive = Motion->Primitive;
    Result.Position = Motion->Position;
    Result.LineDirection = Motion->LineDirection;

    return Result;
}

static ob_appearance_uniform_aligned
OffbeatTranslateAppearance(ob_appearance* Appearance)
{
    ob_appearance_uniform_aligned Result = {};

    Result.Color = Appearance->Color;
    Result.Size = Appearance->Size;

    return Result;
}

void
OffbeatComputeSwapBuffers(ob_particle_system* ParticleSystem)
{
    // NOTE(rytis): Buffer swap.
    {
        u32 Temp = ParticleSystem->ParticleSSBO;
        ParticleSystem->ParticleSSBO = ParticleSystem->OldParticleSSBO;
        ParticleSystem->OldParticleSSBO = Temp;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GlobalUpdatedParticleBinding, ParticleSystem->ParticleSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GlobalOldParticleBinding, ParticleSystem->OldParticleSSBO);
    }

    // NOTE(rytis): Memory allocation for updated particles.
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ParticleSystem->ParticleSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ParticleSystem->ParticleCount * sizeof(ob_particle),
                     0, GL_STREAM_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    u32 Zero = 0;
    ob_emission_uniform_aligned Emission = OffbeatTranslateEmission(&ParticleSystem->Emission);
    ob_motion_uniform_aligned Motion = OffbeatTranslateMotion(&ParticleSystem->Motion);
    ob_appearance_uniform_aligned Appearance = OffbeatTranslateAppearance(&ParticleSystem->Appearance);

    glBindBuffer(GL_UNIFORM_BUFFER, GlobalEmissionBlock);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ob_emission_uniform_aligned), &Emission);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBuffer(GL_UNIFORM_BUFFER, GlobalMotionBlock);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ob_motion_uniform_aligned), &Motion);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBuffer(GL_UNIFORM_BUFFER, GlobalAppearanceBlock);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ob_appearance_uniform_aligned), &Appearance);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, GlobalUpdatedParticleCount);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(u32), &Zero);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

static void
OffbeatComputeSpawnParticles(ob_particle_system* ParticleSystem, ob_state* OffbeatState)
{
    u32 Zero = 0;

    GLenum ErrorCode = glGetError();
    ob_program SpawnProgramID = OffbeatState->SpawnProgramID;
    glUseProgram(SpawnProgramID);

    glUniform1f(glGetUniformLocation(SpawnProgramID, "u_t"), OffbeatState->t);
    glUniform1f(glGetUniformLocation(SpawnProgramID, "dt"), OffbeatState->dt);
    glUniform1ui(glGetUniformLocation(SpawnProgramID, "RunningParticleID"), OffbeatState->RunningParticleID);
    glUniform3fv(glGetUniformLocation(SpawnProgramID, "CameraPosition"), 1, (float*)OffbeatState->CameraPosition.E);
    glUniform1ui(glGetUniformLocation(SpawnProgramID, "ParticleSpawnCount"), ParticleSystem->ParticleSpawnCount);
    glUniform1ui(glGetUniformLocation(SpawnProgramID, "RandomTableIndex"), ObRandomNextRandomUInt32(&OffbeatState->EffectsEntropy));

    glDispatchCompute(ParticleSystem->ParticleSpawnCount / OFFBEAT_WORKGROUP_SIZE + 1, 1, 1);
    OffbeatState->RunningParticleID += ParticleSystem->ParticleSpawnCount;
}

static void
OffbeatComputeUpdateParticles(ob_particle_system* ParticleSystem, ob_state* OffbeatState)
{
    u32 Zero = 0;
    u32 OldParticleCount = ParticleSystem->ParticleCount - ParticleSystem->ParticleSpawnCount;
    if(!OldParticleCount)
    {
        return;
    }

    GLenum ErrorCode = glGetError();
    ob_program UpdateProgramID = OffbeatState->UpdateProgramID;
    glUseProgram(UpdateProgramID);

    glUniform1f(glGetUniformLocation(UpdateProgramID, "u_t"), OffbeatState->t);
    glUniform1f(glGetUniformLocation(UpdateProgramID, "dt"), OffbeatState->dt);
    glUniform1ui(glGetUniformLocation(UpdateProgramID, "RunningParticleID"), OffbeatState->RunningParticleID);
    glUniform3fv(glGetUniformLocation(UpdateProgramID, "CameraPosition"), 1, (float*)OffbeatState->CameraPosition.E);
    glUniform1ui(glGetUniformLocation(UpdateProgramID, "ParticleSpawnCount"), ParticleSystem->ParticleSpawnCount);
    glUniform1ui(glGetUniformLocation(UpdateProgramID, "OldParticleCount"), OldParticleCount);
    glUniform1ui(glGetUniformLocation(UpdateProgramID, "RandomTableIndex"), ObRandomNextRandomUInt32(&OffbeatState->EffectsEntropy));

    glDispatchCompute(OldParticleCount / OFFBEAT_WORKGROUP_SIZE + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, GlobalUpdatedParticleCount);
    u32* Count = (u32*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(u32), GL_MAP_READ_BIT);
    ParticleSystem->ParticleCount = ParticleSystem->ParticleSpawnCount + *Count;
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    OffbeatAssert(ParticleSystem->ParticleCount <= ParticleSystem->MaxParticleCount);
}

static void
OffbeatComputeStatelessEvaluation(ob_draw_list* DrawList, ob_particle_system* ParticleSystem, ob_state* OffbeatState)
{
    GLenum ErrorCode = glGetError();
    ob_program StatelessEvaluationProgramID = OffbeatState->StatelessEvaluationProgramID;
    glUseProgram(StatelessEvaluationProgramID);

    glUniform1f(glGetUniformLocation(StatelessEvaluationProgramID, "u_t"), OffbeatState->t);
    glUniform1f(glGetUniformLocation(StatelessEvaluationProgramID, "dt"), OffbeatState->dt);
    glUniform1ui(glGetUniformLocation(StatelessEvaluationProgramID, "RunningParticleID"), OffbeatState->RunningParticleID);
    glUniform3fv(glGetUniformLocation(StatelessEvaluationProgramID, "CameraPosition"), 1, (float*)OffbeatState->CameraPosition.E);
    glUniform1ui(glGetUniformLocation(StatelessEvaluationProgramID, "ParticleSpawnCount"), ParticleSystem->ParticleSpawnCount);
    glUniform1ui(glGetUniformLocation(StatelessEvaluationProgramID, "ParticleCount"), ParticleSystem->ParticleCount);
    glUniform1ui(glGetUniformLocation(StatelessEvaluationProgramID, "RandomTableIndex"), ObRandomNextRandomUInt32(&OffbeatState->EffectsEntropy));
    glUniform3fv(glGetUniformLocation(StatelessEvaluationProgramID, "Horizontal"), 1, OffbeatState->QuadData.Horizontal.E);
    glUniform3fv(glGetUniformLocation(StatelessEvaluationProgramID, "Vertical"), 1, OffbeatState->QuadData.Vertical.E);

    // NOTE(rytis): Draw list stuff.
    DrawList->IndexCount = 6 * ParticleSystem->ParticleCount;
    DrawList->TextureID = OffbeatGetTextureID(ParticleSystem);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, DrawList->EBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, DrawList->EBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 6 * ParticleSystem->ParticleCount * sizeof(u32),
                 0, GL_STREAM_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, DrawList->VBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, DrawList->VBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 4 * ParticleSystem->ParticleCount * sizeof(ob_draw_vertex_aligned),
                 0, GL_STREAM_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glDispatchCompute(ParticleSystem->ParticleCount / OFFBEAT_WORKGROUP_SIZE + 1, 1, 1);
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_ELEMENT_ARRAY_BARRIER_BIT);
}
#endif

static GLuint
OffbeatRGBATextureID(void* TextureData, u32 Width, u32 Height)
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

static GLuint
OffbeatCompileAndLinkRenderProgram(char* HeaderCode, char* VertexCode, char* FragmentCode)
{
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLchar* VertexShaderCode[] =
    {
        HeaderCode,
        VertexCode,
    };
    glShaderSource(VertexShaderID, OffbeatArrayCount(VertexShaderCode), VertexShaderCode, 0);
    glCompileShader(VertexShaderID);

    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    GLchar* FragmentShaderCode[] =
    {
        HeaderCode,
        FragmentCode,
    };
    glShaderSource(FragmentShaderID, OffbeatArrayCount(FragmentShaderCode), FragmentShaderCode, 0);
    glCompileShader(FragmentShaderID);

    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    glValidateProgram(ProgramID);
    GLint Linked = false;
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Linked);
    if(!Linked)
    {
        GLsizei Ignored;
        char VertexErrors[4096];
        char FragmentErrors[4096];
        char ProgramErrors[4096];
        glGetShaderInfoLog(VertexShaderID, sizeof(VertexErrors), &Ignored, VertexErrors);
        glGetShaderInfoLog(FragmentShaderID, sizeof(FragmentErrors), &Ignored, FragmentErrors);
        glGetProgramInfoLog(ProgramID, sizeof(ProgramErrors), &Ignored, ProgramErrors);

        OffbeatAssert(!"Shader validation failed");
    }

    return ProgramID;
}

static GLuint
OffbeatCreateRenderProgram()
{
    // NOTE(rytis): Version number can be replaced with a macro.
    char* HeaderCode = R"SHADER(
    #version 330 core
    )SHADER";

    char* VertexCode = R"SHADER(
    layout(location = 0) in vec3 Position;
    layout(location = 1) in vec2 UV;
    layout(location = 2) in vec4 Color;

    uniform mat4 Projection;
    uniform mat4 View;

    out vertex_output
    {
        vec2 UV;
        vec4 Color;
    } VertexOutput;

    void
    main()
    {
        VertexOutput.UV = UV;
        VertexOutput.Color = Color;
        gl_Position = Projection * View * vec4(Position, 1.0f);
    }
    )SHADER";

    char* FragmentCode = R"SHADER(
    in vertex_output
    {
        vec2 UV;
        vec4 Color;
    } VertexOutput;

    uniform sampler2D Texture;

    out vec4 FinalColor;

    void main()
    {
        vec4 TextureColor = texture(Texture, VertexOutput.UV);
        FinalColor = TextureColor * VertexOutput.Color;
    }
    )SHADER";

    return OffbeatCompileAndLinkRenderProgram(HeaderCode, VertexCode, FragmentCode);
}

void
OffbeatRenderParticles(ob_state* OffbeatState, float* ViewMatrix, float* ProjectionMatrix)
{
    glEnable(GL_BLEND);

    GLuint RenderProgramID = OffbeatState->RenderProgramID;
    glUseProgram(RenderProgramID);
    glUniformMatrix4fv(glGetUniformLocation(RenderProgramID, "Projection"), 1, GL_FALSE,
                       ProjectionMatrix);
    glUniformMatrix4fv(glGetUniformLocation(RenderProgramID, "View"), 1, GL_FALSE,
                       ViewMatrix);

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex), (GLvoid*)(OffbeatOffsetOf(ob_draw_vertex, Position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex), (GLvoid*)(OffbeatOffsetOf(ob_draw_vertex, UV)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex), (GLvoid*)(OffbeatOffsetOf(ob_draw_vertex, Color)));

    // NOTE(rytis): Draw square grid.
    {
        glBindTexture(GL_TEXTURE_2D, OffbeatState->GridTextureID);
        glBufferData(GL_ARRAY_BUFFER,
                     OffbeatArrayCount(OffbeatState->GridVertices) * sizeof(ob_draw_vertex),
                     OffbeatState->GridVertices,
                     GL_STREAM_DRAW);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     OffbeatArrayCount(OffbeatState->GridIndices) * sizeof(uint32_t),
                     OffbeatState->GridIndices,
                     GL_STREAM_DRAW);

        glDrawElements(GL_TRIANGLES, OffbeatArrayCount(OffbeatState->GridIndices), GL_UNSIGNED_INT, 0);
    }

#ifdef OFFBEAT_DEBUG
    // NOTE(rytis): Draw debug data.
    ob_draw_data_debug* DebugDrawData = OffbeatGetDebugDrawData();
    for(int i = 0; i < DebugDrawData->DrawListCount; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, DebugDrawData->DrawLists[i].TextureID);
        glBufferData(GL_ARRAY_BUFFER,
                     DebugDrawData->DrawLists[i].VertexCount * sizeof(ob_draw_vertex),
                     DebugDrawData->DrawLists[i].Vertices,
                     GL_STREAM_DRAW);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     DebugDrawData->DrawLists[i].IndexCount * sizeof(uint32_t),
                     DebugDrawData->DrawLists[i].Indices,
                     GL_STREAM_DRAW);

        glDrawElements(GL_TRIANGLES, DebugDrawData->DrawLists[i].IndexCount, GL_UNSIGNED_INT, 0);
    }
#endif

    ob_draw_data* DrawData;
    // NOTE(rytis): Draw particles.
    glDepthMask(GL_FALSE);
#ifdef OFFBEAT_OPENGL_COMPUTE
    DrawData = &OffbeatState->DrawData;
    for(int i = 0; i < DrawData->DrawListCount; ++i)
    {
        glBindVertexArray(DrawData->DrawLists[i].VAO);
        glBindTexture(GL_TEXTURE_2D, DrawData->DrawLists[i].TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, DrawData->DrawLists[i].VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, DrawData->DrawLists[i].EBO);

        glDrawElements(GL_TRIANGLES, DrawData->DrawLists[i].IndexCount, GL_UNSIGNED_INT, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);
#else
    DrawData = &OffbeatState->DrawData;
    for(int i = 0; i < DrawData->DrawListCount; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, DrawData->DrawLists[i].TextureID);
        glBufferData(GL_ARRAY_BUFFER,
                     DrawData->DrawLists[i].VertexCount * sizeof(ob_draw_vertex),
                     DrawData->DrawLists[i].Vertices,
                     GL_STREAM_DRAW);

        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     DrawData->DrawLists[i].IndexCount * sizeof(uint32_t),
                     DrawData->DrawLists[i].Indices,
                     GL_STREAM_DRAW);

        glDrawElements(GL_TRIANGLES, DrawData->DrawLists[i].IndexCount, GL_UNSIGNED_INT, 0);
    }
#endif

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
