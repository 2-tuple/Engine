#include <GL/glew.h>
#include <cassert>
#include <stddef.h>

#include "mesh.h"

void
Render::SetUpMesh(Render::mesh* Mesh)
{
  // Setting up vertex array object
  glGenVertexArrays(1, &Mesh->VAO);
  glBindVertexArray(Mesh->VAO);

  // Setting up vertex buffer object
  glGenBuffers(1, &Mesh->VBO);
  glBindBuffer(GL_ARRAY_BUFFER, Mesh->VBO);
  glBufferData(GL_ARRAY_BUFFER, Mesh->VerticeCount * sizeof(Render::vertex), Mesh->Vertices,
               GL_STATIC_DRAW);

  // Setting up element buffer object
  glGenBuffers(1, &Mesh->EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mesh->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, Mesh->IndiceCount * sizeof(uint32_t), Mesh->Indices,
               GL_STATIC_DRAW);

  // Setting vertex attribute pointers
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Render::vertex),
                        (GLvoid*)(offsetof(Render::vertex, Position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Render::vertex),
                        (GLvoid*)(offsetof(Render::vertex, Normal)));
  if(Mesh->HasUVs)
  {
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Render::vertex),
                          (GLvoid*)(offsetof(Render::vertex, UV)));
  }

  if(Mesh->HasTangents)
  {
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Render::vertex),
                          (GLvoid*)(offsetof(Render::vertex, Tangent)));
  }

  if(Mesh->BoneCount > 0)
  {
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, VERTEX_MAX_BONE_COUNT, GL_INT, sizeof(Render::vertex),
                           (GLvoid*)(offsetof(Render::vertex, BoneIndices)));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, VERTEX_MAX_BONE_COUNT, GL_FLOAT, GL_FALSE, sizeof(Render::vertex),
                          (GLvoid*)(offsetof(Render::vertex, BoneWeights)));
  }

  // Unbind VAO
  glBindVertexArray(0);
}

void
Render::CleanUpMesh(Render::mesh* Mesh)
{
  glDeleteBuffers(1, &Mesh->VBO);
  glDeleteBuffers(1, &Mesh->EBO);

  // Delete vertex array
  glDeleteVertexArrays(1, &Mesh->VAO);

  // Unbind VAO
  glBindVertexArray(0);
}
