#version 330 core

layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec4 Color;

uniform mat4 Projection;

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
    gl_Position = Projection * vec4(Position.xy, 0.0f, 1.0f);
}
