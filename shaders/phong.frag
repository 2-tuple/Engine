#version 330 core

#define DIFFUSE_MAP 1
#define SPECULAR_MAP 2
#define NORMAL_MAP 4

struct Material
{
  vec4 diffuseColor;

  sampler2D diffuseMap;
  sampler2D specularMap;
  sampler2D normalMap;

  float shininess;
};

struct Light
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

in VertexOut
{
  flat int flags;

  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 lightPos;
  vec3 cameraPos;
  vec3 tangentLightPos;
  vec3 tangentViewPos;
  vec3 tangentFragPos;
}
frag;

uniform Material material;
uniform Light light;

out vec4 out_color;

void
main()
{
  vec3 normal   = vec3(0.0f);
  vec3 lightDir = vec3(0.0f);
  vec3 viewDir  = vec3(0.0f);

  if((frag.flags & NORMAL_MAP) != 0)
  {
    normal   = normalize(texture(material.normalMap, frag.texCoord).rgb);
    normal   = normalize(normal * 2.0f - 1.0f);
    lightDir = normalize(frag.tangentLightPos - frag.tangentFragPos);
    viewDir  = normalize(frag.tangentViewPos - frag.tangentFragPos);
  }
  else
  {
    normal   = normalize(frag.normal);
    lightDir = normalize(frag.lightPos - frag.position);
    viewDir  = normalize(frag.cameraPos - frag.position);
  }

  vec3 half_vector = normalize(lightDir + viewDir);

  // --------DIFFUSE------
  float diffuse_intensity = max(dot(normal, lightDir), 0.0f);
  vec3  diffuse           = diffuse_intensity * light.diffuse;

  // --------SPECULAR------
  float specular_intensity = pow(max(dot(normal, half_vector), 0.0f), material.shininess);
  vec3  specular           = vec3(0.0f);
  if((frag.flags & SPECULAR_MAP) != 0)
  {
    specular =
      vec3(texture(material.specularMap, frag.texCoord)) * specular_intensity * light.specular;
  }
  else
  {
    specular = specular_intensity * light.specular;
  }

  // --------AMBIENT--------
  vec3 ambient = light.ambient;

  // --------FINAL----------
  vec4 result = vec4(0.0f);
  if((frag.flags & DIFFUSE_MAP) != 0)
  {
    result =
      vec4((diffuse + specular + ambient) * vec3(texture(material.diffuseMap, frag.texCoord)),
           1.0f);
  }
  else
  {
    result =
      vec4((diffuse + specular + ambient) * material.diffuseColor.rgb, material.diffuseColor.a);
  }

  out_color = result;
}
