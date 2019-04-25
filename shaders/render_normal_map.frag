#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D NormalMap;

void main()
{
    vec3 Normal = texture(NormalMap, TexCoords).rgb;
    FragColor = vec4(Normal, 1.0);
}
