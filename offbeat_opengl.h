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
OffbeatCreateProgram(char* HeaderCode, char* VertexCode, char* FragmentCode)
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
OffbeatCreateSpawnProgram()
{
    return 0;
}

static GLuint
OffbeatCreateUpdateProgram()
{
    return 0;
}

static GLuint
OffbeatCreateRenderProgram()
{
    // NOTE(rytis): Version number can be replaced with a macro.
    char* HeaderCode = R"SHADER(
    #version 330 core
    )SHADER";

    char* VertexCode = R"SHADER(
    layout (location = 0) in vec3 Position;
    layout (location = 1) in vec2 UV;
    layout (location = 2) in vec4 Color;

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

    return OffbeatCreateProgram(HeaderCode, VertexCode, FragmentCode);
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex), (GLvoid*)(offsetof(ob_draw_vertex, Position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex), (GLvoid*)(offsetof(ob_draw_vertex, UV)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ob_draw_vertex), (GLvoid*)(offsetof(ob_draw_vertex, Color)));

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

    // NOTE(rytis): Draw particles.
    glDepthMask(GL_FALSE);
    ob_draw_data* DrawData = &OffbeatState->DrawData;
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

    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glBindVertexArray(0);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
