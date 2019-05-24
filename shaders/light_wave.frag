#version 330 core

#define PI 3.14159265359

uniform float u_Time;

struct Material
{
  vec3 ambientColor;
  vec4 diffuseColor;
  vec3 specularColor;

  float shininess;
};

uniform sampler2D u_AmbientOcclusion;

struct Light
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct Sun	
{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

in VertexOut
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 lightPos;
  vec3 sunDir;
  vec3 cameraPos;
  vec4 sunFragPos;
}
frag;

uniform Material material;
uniform Light light;
uniform Sun sun;

uniform sampler2D shadowMap;

out vec4 out_color;

float
ShadowCalculation(vec4 sunFragPos, vec3 normal, vec3 lightDir)
{
  vec3 projCoords = sunFragPos.xyz / sunFragPos.w;
  projCoords = projCoords * 0.5f + 0.5f;

  float closestDepth = texture(shadowMap, projCoords.xy).r;
  float currentDepth = projCoords.z;

  float shadow = 0.0f;
  if(currentDepth <= 1.0f)
  {
    //float bias = max(0.005f * (1.0f - dot(normal, lightDir)), 0.005f);
    float bias = 0.005f;
    //shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

    vec2 texelSize = 1.0f / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
      for(int y = -1; y <= 1; ++y)
      {
        float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0f : 0.0f;
      }
    }
    shadow /= 9.0f;
  }

  return shadow;
}

float
cubicPulse(float c, float w, float x)
{
    x = abs(x - c);
    if(x > w)
    {
      return 0.0;
    }
    x /= w;
    return 1.0 - x * x * (3.0 - 2.0 * x);
}

void
main()
{
  vec3 normal   = vec3(0.0f);
  vec3 lightDir = vec3(0.0f);
  vec3 viewDir  = vec3(0.0f);

  normal   = normalize(frag.normal);
  lightDir = normalize(frag.lightPos - frag.position);
  viewDir  = normalize(frag.cameraPos - frag.position);

  // --------SHADOW--------
  float shadow = ShadowCalculation(frag.sunFragPos, normal, frag.sunDir);
  float lighting = 1.0f - shadow;

  vec3 sunDir = normalize(-frag.sunDir);
  vec3 half_vector = normalize(lightDir + viewDir);
  vec3 reflectDir = reflect(-sunDir, normal);

  // --------AMBIENT--------
  vec3 ambient = light.ambient;
  ambient += sun.ambient * lighting ;

  // --------DIFFUSE------
  float diffuse_intensity = max(dot(normal, lightDir), 0.0f);
  vec3  diffuse           = diffuse_intensity * light.diffuse;

  float sun_diffuse_intensity = max(dot(normal, sunDir), 0.0f);
  diffuse += sun_diffuse_intensity * sun.diffuse * lighting;

  ambient *= material.ambientColor;
  diffuse *= material.diffuseColor.rgb;
  
  vec2 screen_tex_coords = gl_FragCoord.xy/textureSize(u_AmbientOcclusion, 0).xy; 
  float ambient_occlusion = texture(u_AmbientOcclusion, screen_tex_coords).r;
  ambient *= ambient_occlusion;

  // --------SPECULAR------
  float specular_intensity = pow(max(dot(normal, half_vector), 0.0f), material.shininess);
  specular_intensity *= (diffuse_intensity > 0.0f) ? 1.0f : 0.0f;
  vec3 specular = specular_intensity * light.specular;

  float sun_specular_intensity = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
  specular += sun_specular_intensity * sun.specular * lighting;

  specular *= material.specularColor;

  // --------FINAL----------
  vec4 result = vec4(diffuse + specular + ambient, material.diffuseColor.a);

  float PeriodicWaveBrightness = 0.5 * cubicPulse(0.0, 0.5, 10.0 * sin(u_Time + 0.05 * frag.position.x * frag.position.y * frag.position.z * PI));
  float ColorValue = 0.5 * (1.0 + sin(u_Time + PI));
  vec3 PeriodicWaveColor = vec3(ColorValue, ColorValue, 0.0);
  result += vec4(PeriodicWaveBrightness * PeriodicWaveColor, 1.0);

  out_color = result;
}
