#version 330 core

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
