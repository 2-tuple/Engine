#version 330 core

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
