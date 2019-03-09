#pragma once

#include "shader_def.h"

GLuint
SetMaterial(game_state* GameState, const camera* Camera, const material* Material)
{
  TIMED_BLOCK(SetMaterial);
  if(Material->Common.UseBlending)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  if(Material->Common.ShaderType == SHADER_Phong)
  {
    GLuint PhongShaderID = GameState->Resources.GetShader(GameState->R.ShaderPhong);
    glUseProgram(PhongShaderID);

    glUniform1i(glGetUniformLocation(PhongShaderID, "flags"), Material->Phong.Flags);
    glUniform1f(glGetUniformLocation(PhongShaderID, "u_Time"), GameState->R.CumulativeTime);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "material.ambientColor"), 1,
                 (float*)&Material->Phong.AmbientColor);
    glUniform4fv(glGetUniformLocation(PhongShaderID, "material.diffuseColor"), 1,
                 (float*)&Material->Phong.DiffuseColor);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "material.specularColor"), 1,
                 (float*)&Material->Phong.SpecularColor);
    glUniform1i(glGetUniformLocation(PhongShaderID, "material.diffuseMap"), 0);
    glUniform1i(glGetUniformLocation(PhongShaderID, "material.specularMap"), 1);
    glUniform1i(glGetUniformLocation(PhongShaderID, "material.normalMap"), 2);
    glUniform1i(glGetUniformLocation(PhongShaderID, "u_AmbientOcclusion"), 3);
    glUniform1i(glGetUniformLocation(PhongShaderID, "shadowMap_0"), 4);
    glUniform1i(glGetUniformLocation(PhongShaderID, "shadowMap_1"), 5);
    glUniform1i(glGetUniformLocation(PhongShaderID, "shadowMap_2"), 6);
    glUniform1i(glGetUniformLocation(PhongShaderID, "shadowMap_3"), 7);
    glUniform1f(glGetUniformLocation(PhongShaderID, "material.shininess"),
                Material->Phong.Shininess);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "sunDirection"), 1,
                 (float*)&GameState->R.Sun.Direction);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "sun.ambient"), 1,
                 (float*)&GameState->R.Sun.AmbientColor);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "sun.diffuse"), 1,
                 (float*)&GameState->R.Sun.DiffuseColor);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "lightPosition"), 1,
                 (float*)&GameState->R.LightPosition);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "light.ambient"), 1,
                 (float*)&GameState->R.LightAmbientColor);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "light.diffuse"), 1,
                 (float*)&GameState->R.LightDiffuseColor);
    glUniform3fv(glGetUniformLocation(PhongShaderID, "cameraPosition"), 1,
                 (float*)&Camera->Position);
    glUniformMatrix4fv(glGetUniformLocation(PhongShaderID, "mat_sun_vp"), SHADOWMAP_CASCADE_COUNT,
                       GL_FALSE, GameState->R.Sun.CascadeVP[0].e);
    glUniform1fv(glGetUniformLocation(PhongShaderID, "u_CascadeFarPlanes"), SHADOWMAP_CASCADE_COUNT,
                 GameState->R.Sun.CascadeFarPlaneDistances);
    assert(
      ((Material->Phong.Flags & PHONG_UseDiffuseMap) && Material->Phong.DiffuseMapID.Value > 0) ||
      !(Material->Phong.Flags & PHONG_UseDiffuseMap));
    assert(
      ((Material->Phong.Flags & PHONG_UseSpecularMap) && Material->Phong.SpecularMapID.Value > 0) ||
      !(Material->Phong.Flags & PHONG_UseSpecularMap));
    assert(
      ((Material->Phong.Flags & PHONG_UseNormalMap) && Material->Phong.NormalMapID.Value > 0) ||
      !(Material->Phong.Flags & PHONG_UseNormalMap));

    uint32_t DiffuseTexture = (Material->Phong.Flags & PHONG_UseDiffuseMap)
                                ? GameState->Resources.GetTexture(Material->Phong.DiffuseMapID)
                                : 0;
    uint32_t SpecularTexture = (Material->Phong.Flags & PHONG_UseSpecularMap)
                                 ? GameState->Resources.GetTexture(Material->Phong.SpecularMapID)
                                 : 0;
    uint32_t NormalTexture = (Material->Phong.Flags & PHONG_UseNormalMap)
                               ? GameState->Resources.GetTexture(Material->Phong.NormalMapID)
                               : 0;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, DiffuseTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, SpecularTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, NormalTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, GameState->R.SSAOTexID);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, GameState->R.ShadowMapTextures[0]);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, GameState->R.ShadowMapTextures[1]);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, GameState->R.ShadowMapTextures[2]);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, GameState->R.ShadowMapTextures[3]);
    glActiveTexture(GL_TEXTURE0);
    return PhongShaderID;
  }
  else if(Material->Common.ShaderType == SHADER_Env)
  {
    GLuint EnvShaderID = GameState->Resources.GetShader(GameState->R.ShaderEnv);
    glUseProgram(EnvShaderID);
    glUniform1i(glGetUniformLocation(EnvShaderID, "flags"), Material->Env.Flags);
    glUniform1i(glGetUniformLocation(EnvShaderID, "cubemap"), 0);
    glUniform1i(glGetUniformLocation(EnvShaderID, "normalMap"), 1);
    glUniform1f(glGetUniformLocation(EnvShaderID, "refractive_index"),
                Material->Env.RefractiveIndex);
    glUniform3fv(glGetUniformLocation(EnvShaderID, "cameraPosition"), 1, (float*)&Camera->Position);
    assert(((Material->Env.Flags & ENV_UseNormalMap) && Material->Env.NormalMapID.Value > 0) ||
           !(Material->Env.Flags & ENV_UseNormalMap));

    uint32_t CubemapTexture = GameState->R.Cubemap.CubemapTexture;
    uint32_t NormalTexture  = (Material->Env.Flags & ENV_UseNormalMap)
                               ? GameState->Resources.GetTexture(Material->Env.NormalMapID)
                               : 0;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, CubemapTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, NormalTexture);
    glActiveTexture(GL_TEXTURE0);
    return EnvShaderID;
  }
  else if(Material->Common.ShaderType == SHADER_Color)
  {
    GLuint ColorShaderID = GameState->Resources.GetShader(GameState->R.ShaderColor);
    glUseProgram(ColorShaderID);
    glUniform4fv(glGetUniformLocation(ColorShaderID, "g_color"), 1, (float*)&Material->Color.Color);
    return ColorShaderID;
  }
  else
  {
    struct shader_def* ShaderDef = NULL;
    assert(GetShaderDef(&ShaderDef, Material->Common.ShaderType));
    int32_t CurrentGLTextureBindIndex  = GL_TEXTURE0;
    int32_t MaximalBoundGLTextureCount = 0;

    rid    CurrentShaderRID = GetShaderRID(ShaderDef);
    GLuint CurrentShaderID  = GameState->Resources.GetShader(CurrentShaderRID);

    glUseProgram(CurrentShaderID);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MaximalBoundGLTextureCount);

    named_shader_param_def ParamDef = {};

    ResetShaderDefIterator(ShaderDef);
    // TODO(Lukas): Move all glGetUniformLocation calls to shader_def.cpp and return actual
    // location variable inside of ParamDef
    while(GetNextShaderParam(&ParamDef, ShaderDef))
    {
      uint8_t* ParamPtr = (((uint8_t*)Material) + ParamDef.OffsetIntoMaterial);
      switch(ParamDef.Type)
      {
        case SHADER_PARAM_TYPE_Int:
        {
          int32_t Value = *((int32_t*)ParamPtr);
          glUniform1i(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), Value);
        }
        break;
        case SHADER_PARAM_TYPE_Bool:
        {
          bool    BoolValue = *((bool*)ParamPtr);
          int32_t Value     = (int32_t)BoolValue;
          glUniform1i(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), Value);
        }
        break;
        case SHADER_PARAM_TYPE_Float:
        {
          float Value = *((float*)ParamPtr);
          glUniform1f(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), Value);
        }
        break;
        case SHADER_PARAM_TYPE_Vec3:
        {
          vec3 Value = *((vec3*)ParamPtr);
          glUniform3fv(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), 1,
                       (float*)ParamPtr);
        }
        break;
        case SHADER_PARAM_TYPE_Vec4:
        {
          glUniform4fv(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName), 1,
                       (float*)ParamPtr);
        }
        break;
        case SHADER_PARAM_TYPE_Map:
        {
          rid RIDValue = *((rid*)ParamPtr);

          assert(CurrentGLTextureBindIndex < (GL_TEXTURE0 + MaximalBoundGLTextureCount));
          glActiveTexture(CurrentGLTextureBindIndex);
          glBindTexture(GL_TEXTURE_2D, GameState->Resources.GetTexture(RIDValue));
          glUniform1i(glGetUniformLocation(CurrentShaderID, ParamDef.UniformName),
                      CurrentGLTextureBindIndex - GL_TEXTURE0);

          glActiveTexture(GL_TEXTURE0);
          CurrentGLTextureBindIndex++;
        }
        break;
      }
    }
    {
      //<TODO(Lukas) make constant things upload only on shader change>
      // TODO(Lukas) Change dt for time uniform to a cumulative time
      glUniform1f(glGetUniformLocation(CurrentShaderID, "u_Time"), GameState->R.CumulativeTime);
      glUniform1f(glGetUniformLocation(CurrentShaderID, "cameraNearPlane"),
                  GameState->Camera.NearClipPlane);
      glUniform1f(glGetUniformLocation(CurrentShaderID, "cameraFarPlane"),
                  GameState->Camera.FarClipPlane);
      glUniform3fv(glGetUniformLocation(CurrentShaderID, "lightPosition"), 1,
                   (float*)&GameState->R.LightPosition);
      glUniform3fv(glGetUniformLocation(CurrentShaderID, "light.ambient"), 1,
                   (float*)&GameState->R.LightAmbientColor);
      glUniform3fv(glGetUniformLocation(CurrentShaderID, "light.diffuse"), 1,
                   (float*)&GameState->R.LightDiffuseColor);
      glUniform3fv(glGetUniformLocation(CurrentShaderID, "light.specular"), 1,
                   (float*)&GameState->R.LightSpecularColor);
      glUniform3fv(glGetUniformLocation(CurrentShaderID, "sun.direction"), 1,
                   (float*)&GameState->R.Sun.Direction);
      glUniform3fv(glGetUniformLocation(CurrentShaderID, "sun.ambient"), 1,
                   (float*)&GameState->R.Sun.AmbientColor);
      glUniform3fv(glGetUniformLocation(CurrentShaderID, "sun.diffuse"), 1,
                   (float*)&GameState->R.Sun.DiffuseColor);
      glUniform3fv(glGetUniformLocation(CurrentShaderID, "cameraPosition"), 1,
                   (float*)&Camera->Position);
      glUniformMatrix4fv(glGetUniformLocation(CurrentShaderID, "mat_sun_vp"), 1, GL_FALSE,
                         GameState->R.Sun.CascadeVP[GameState->R.Sun.CurrentCascadeIndex].e);
      // Cubemap
      {
        assert(CurrentGLTextureBindIndex < (GL_TEXTURE0 + MaximalBoundGLTextureCount));
        glActiveTexture(CurrentGLTextureBindIndex);
        glBindTexture(GL_TEXTURE_CUBE_MAP, GameState->R.Cubemap.CubemapTexture);
        glUniform1i(glGetUniformLocation(CurrentShaderID, "cubemap"),
                    CurrentGLTextureBindIndex - GL_TEXTURE0);

        glActiveTexture(GL_TEXTURE0);
        CurrentGLTextureBindIndex++;
      }
      // Shadow Map
      {
        assert(CurrentGLTextureBindIndex < (GL_TEXTURE0 + MaximalBoundGLTextureCount));
        glActiveTexture(CurrentGLTextureBindIndex);
        glBindTexture(GL_TEXTURE_2D,
                      GameState->R.ShadowMapTextures[GameState->R.Sun.CurrentCascadeIndex]);
        glUniform1i(glGetUniformLocation(CurrentShaderID, "shadowMap"),
                    CurrentGLTextureBindIndex - GL_TEXTURE0);

        glActiveTexture(GL_TEXTURE0);
        CurrentGLTextureBindIndex++;
      }
      // AO map
      {
        assert(CurrentGLTextureBindIndex < (GL_TEXTURE0 + MaximalBoundGLTextureCount));
        glActiveTexture(CurrentGLTextureBindIndex);
        glBindTexture(GL_TEXTURE_2D, GameState->R.SSAOTexID);
        glUniform1i(glGetUniformLocation(CurrentShaderID, "u_AmbientOcclusionMap"),
                    CurrentGLTextureBindIndex - GL_TEXTURE0);

        glActiveTexture(GL_TEXTURE0);
        CurrentGLTextureBindIndex++;
      }
      //<\TODO>
    }
    return CurrentShaderID;
  }
  return -1;
}
