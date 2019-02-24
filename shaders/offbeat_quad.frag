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
    // TODO(rytis): Pass a proper texture.
    // FinalColor = TextureColor * VertexOutput.Color;
    FinalColor = VertexOutput.Color;
}
