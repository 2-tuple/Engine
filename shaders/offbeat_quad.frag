#version 330 core

in vertex_output
{
    vec2 UV;
    vec4 Color;
} VertexOutput;

out vec4 FinalColor;

void main()
{
    FinalColor = VertexOutput.Color;
}
