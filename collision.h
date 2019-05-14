#pragma once

#include <stdint.h>
#include <float.h>
#include "linear_math/vector.h"
//#include "game.h"
#include "debug_drawing.h"
#include "mesh.h"
#include "profile.h"

#define MAX_CONTACT_POINTS 50

bool g_VisualizeContactPoints;
bool g_VisualizeContactManifold;

vec3
TransformVector(vec3 Vector, mat4 Matrix)
{
  return Math::Vec4ToVec3(Math::MulMat4Vec4(Matrix, Math::Vec4(Vector, 1.0f)));
}

vec3
Support(Render::mesh* Mesh, vec3 Direction, mat4 ModelMatrix)
{
  vec3 Transformed = TransformVector(Mesh->Vertices[0].Position, ModelMatrix);

  float   Max   = Math::Dot(Transformed, Direction);
  int32_t Index = 0;

  float DotProduct;

  for(int i = 1; i < Mesh->VerticeCount; i++)
  {
    Transformed = TransformVector(Mesh->Vertices[i].Position, ModelMatrix);
    DotProduct  = Math::Dot(Transformed, Direction);
    if(DotProduct > Max)
    {
      Max   = DotProduct;
      Index = i;
    }
  }

  return TransformVector(Mesh->Vertices[Index].Position, ModelMatrix);
}

void
DoSimplex1(vec3* Simplex, vec3* Direction)
{
  vec3 AO = -Simplex[1];
  vec3 AB = Simplex[0] - Simplex[1];

  *Direction = Math::Cross(Math::Cross(AB, AO), AB);
}

void
DoSimplex2(vec3* Simplex, int32_t* SimplexOrder, vec3* Direction)
{
  vec3 AO  = -Simplex[2];
  vec3 AB  = Simplex[1] - Simplex[2];
  vec3 AC  = Simplex[0] - Simplex[2];
  vec3 ABC = Math::Cross(AB, AC);

  if(Math::Dot(Math::Cross(ABC, AC), AO) > 0)
  {
    Simplex[1]    = Simplex[2];
    *SimplexOrder = 1;
    *Direction    = Math::Cross(Math::Cross(AC, AO), AC);
  }
  else if(Math::Dot(Math::Cross(AB, ABC), AO) > 0)
  {
    Simplex[0]    = Simplex[1];
    Simplex[1]    = Simplex[2];
    *SimplexOrder = 1;
    *Direction    = Math::Cross(Math::Cross(AB, AO), AB);
  }
  else
  {
    if(Math::Dot(ABC, AO) > 0)
    {
      *Direction = ABC;
    }
    else
    {
      vec3 Temp  = Simplex[0];
      Simplex[0] = Simplex[1];
      Simplex[1] = Temp;
      *Direction = -ABC;
    }
  }
}

bool
DoSimplex3(vec3* Simplex, int32_t* SimplexOrder, vec3* Direction)
{
  vec3 AO  = -Simplex[3];
  vec3 AB  = Simplex[2] - Simplex[3];
  vec3 AC  = Simplex[1] - Simplex[3];
  vec3 AD  = Simplex[0] - Simplex[3];
  vec3 ABC = Math::Cross(AB, AC);
  vec3 ADB = Math::Cross(AD, AB);
  vec3 ACD = Math::Cross(AC, AD);

  // Counter-clockwise
  if(Math::Dot(ABC, AO) > 0)
  {
    if(Math::Dot(Math::Cross(ABC, AC), AO) > 0)
    {
      // AC region
      Simplex[0]    = Simplex[1];
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AC, AO), AC);
      return false;
    }
    else if(Math::Dot(Math::Cross(AB, ABC), AO) > 0)
    {
      // AB region
      Simplex[0]    = Simplex[2];
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AB, AO), AB);
      return false;
    }
    // ABC region
    Simplex[0]    = Simplex[1]; // C -> 0
    Simplex[1]    = Simplex[2]; // B -> 1
    Simplex[2]    = Simplex[3]; // A -> 2
    *SimplexOrder = 2;
    *Direction    = ABC;
    return false;
  }
  else if(Math::Dot(ACD, AO) > 0)
  {
    if(Math::Dot(Math::Cross(ACD, AD), AO) > 0)
    {
      // AD region
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AD, AO), AD);
      return false;
    }
    else if(Math::Dot(Math::Cross(AC, ACD), AO) > 0)
    {
      // AC region
      Simplex[0]    = Simplex[1];
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AC, AO), AC);
      return false;
    }
    // ACD region
    Simplex[0]    = Simplex[0]; // D -> 0
    Simplex[1]    = Simplex[1]; // C -> 1
    Simplex[2]    = Simplex[3]; // A -> 2
    *SimplexOrder = 2;
    *Direction    = ACD;
    return false;
  }
  else if(Math::Dot(ADB, AO) > 0)
  {
    if(Math::Dot(Math::Cross(ADB, AB), AO) > 0)
    {
      // AB region
      Simplex[0]    = Simplex[2];
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AB, AO), AB);
      return false;
    }
    else if(Math::Dot(Math::Cross(AD, ADB), AO) > 0)
    {
      // AD region
      Simplex[1]    = Simplex[3];
      *SimplexOrder = 1;
      *Direction    = Math::Cross(Math::Cross(AD, AO), AD);
      return false;
    }
    // ADB region
    Simplex[1]    = Simplex[0]; // D -> 1
    Simplex[0]    = Simplex[2]; // B -> 0
    Simplex[2]    = Simplex[3]; // A -> 2
    *SimplexOrder = 2;
    *Direction    = ADB;
    return false;
  }
  return true;
}

bool
GJK(vec3* Simplex, int32_t* SimplexOrder, Render::mesh* MeshA, Render::mesh* MeshB,
    mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount, int32_t* FoundInIterations,
    vec3* Direction)
{
  vec3 TransformedA = TransformVector(MeshA->Vertices[0].Position, ModelAMatrix);
  vec3 TransformedB = TransformVector(MeshB->Vertices[0].Position, ModelBMatrix);

  Simplex[0]    = TransformedA - TransformedB;
  *Direction    = -Simplex[0];
  *SimplexOrder = 0;

  for(int i = 0; i < IterationCount; i++)
  {
    vec3 SupportA = Support(MeshA, *Direction, ModelAMatrix);
    vec3 SupportB = Support(MeshB, -*Direction, ModelBMatrix);
    vec3 A        = SupportA - SupportB;

    if(Math::Dot(A, *Direction) < 0)
    {
      return false;
    }

    ++(*SimplexOrder);
    Simplex[*SimplexOrder] = A;

    ++i;
    if(IterationCount <= i)
    {
      break;
    }

    switch(*SimplexOrder)
    {
      case 1:
      {
        DoSimplex1(Simplex, Direction);
      }
      break;
      case 2:
      {
        DoSimplex2(Simplex, SimplexOrder, Direction);
      }
      break;
      case 3:
      {
        if(DoSimplex3(Simplex, SimplexOrder, Direction))
        {
          *FoundInIterations = i;
          return true;
        }
      }
      break;
    }
  }
  return false;
}

struct edge
{
  vec3 A;
  vec3 B;
};

struct triangle
{
  vec3 A;
  vec3 B;
  vec3 C;

  vec3 Normal;
};

void
GeneratePolytopeFrom3Simplex(triangle* Polytope, int32_t* TriangleCount, vec3* Simplex)
{
  // ABC
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[2];
  Polytope[*TriangleCount].C = Simplex[1];

  Polytope[*TriangleCount].Normal =
    Math::Normalized(Math::Cross(Polytope[*TriangleCount].B - Polytope[*TriangleCount].A,
                                 Polytope[*TriangleCount].C - Polytope[*TriangleCount].A));

  ++(*TriangleCount);

  // ACD
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[1];
  Polytope[*TriangleCount].C = Simplex[0];

  Polytope[*TriangleCount].Normal =
    Math::Normalized(Math::Cross(Polytope[*TriangleCount].B - Polytope[*TriangleCount].A,
                                 Polytope[*TriangleCount].C - Polytope[*TriangleCount].A));
  ++(*TriangleCount);

  // ADB
  Polytope[*TriangleCount].A = Simplex[3];
  Polytope[*TriangleCount].B = Simplex[0];
  Polytope[*TriangleCount].C = Simplex[2];

  Polytope[*TriangleCount].Normal =
    Math::Normalized(Math::Cross(Polytope[*TriangleCount].B - Polytope[*TriangleCount].A,
                                 Polytope[*TriangleCount].C - Polytope[*TriangleCount].A));

  ++(*TriangleCount);

  // BDC
  Polytope[*TriangleCount].A = Simplex[2];
  Polytope[*TriangleCount].B = Simplex[0];
  Polytope[*TriangleCount].C = Simplex[1];

  Polytope[*TriangleCount].Normal =
    Math::Normalized(Math::Cross(Polytope[*TriangleCount].B - Polytope[*TriangleCount].A,
                                 Polytope[*TriangleCount].C - Polytope[*TriangleCount].A));

  ++(*TriangleCount);
}

float
PointToPlaneDistance(vec3 Point, vec3 PlanePoint, vec3 PlaneNormal)
{
  return Math::Dot(Point - PlanePoint, PlaneNormal);
}

vec3
ClosestPointInPlane(vec3 Point, vec3 PlanePoint, vec3 PlaneNormal)
{
  return Point - PointToPlaneDistance(Point, PlanePoint, PlaneNormal) * PlaneNormal;
}

int32_t
FindEdge(edge* Edges, int32_t EdgeCount, vec3 A, vec3 B)
{
  for(int i = 0; i < EdgeCount; i++)
  {
    if((A == Edges[i].B) && (B == Edges[i].A))
    {
      return i;
    }
  }
  return -1;
}

void
BarycentricCoordinates(float* U, float* V, float* W, vec3 P, vec3 A, vec3 B, vec3 C)
{
  vec3 V0 = B - A;
  vec3 V1 = C - A;
  vec3 V2 = P - A;

  float D00 = Math::Dot(V0, V0);
  float D01 = Math::Dot(V0, V1);
  float D11 = Math::Dot(V1, V1);
  float D20 = Math::Dot(V2, V0);
  float D21 = Math::Dot(V2, V1);

  float Denominator = D00 * D11 - D01 * D01;

  *V = (D11 * D20 - D01 * D21) / Denominator;
  *W = (D00 * D21 - D01 * D20) / Denominator;
  *U = 1.0f - *V - *W;
}

#define DEBUG_COLLISION 0
#define DEBUG_SHOW_RESULT (1 || DEBUG_COLLISION)

vec3
EPA(vec3* CollisionPoint, vec3* Simplex, Render::mesh* MeshA, Render::mesh* MeshB,
    mat4 ModelAMatrix, mat4 ModelBMatrix, int32_t IterationCount)
{
  vec3 Result;

  const float MinThreshold = 0.00001f;

  float PrevMinDistance = FLT_MAX;

  vec3 Origin = { 0.0f, 0.0f, 0.0f };

  triangle Polytope[100];
  int32_t  TriangleCount = 0;

  GeneratePolytopeFrom3Simplex(Polytope, &TriangleCount, Simplex);

  for(int Iteration = 0; Iteration < IterationCount; Iteration++)
  {
    if(Iteration == IterationCount - 1)
    {
#if DEBUG_COLLISION
      for(int i = 0; i < TriangleCount; i++)
      {
        Debug::PushLine(Polytope[i].A, Polytope[i].B, { 0, 0, 1, 1 });
        Debug::PushLine(Polytope[i].B, Polytope[i].C, { 0, 0, 1, 1 });
        Debug::PushLine(Polytope[i].C, Polytope[i].A, { 0, 0, 1, 1 });
        vec3 NormalStart = 0.33f * Polytope[i].A + 0.33f * Polytope[i].B + 0.33f * Polytope[i].C;
        vec3 NormalEnd   = NormalStart + Polytope[i].Normal;
        Debug::PushLine(NormalStart, NormalEnd, { 1, 0, 1, 1 });
        Debug::PushWireframeSphere(NormalEnd, 0.05f);
      }
#endif
    }
    int32_t TriangleIndex = 0;
    float   MinDistance   = PointToPlaneDistance(Origin, -Polytope[0].A, Polytope[0].Normal);

    for(int i = 1; i < TriangleCount; i++)
    {
      float CurrentDistance = PointToPlaneDistance(Origin, -Polytope[i].A, Polytope[i].Normal);
      if(CurrentDistance < MinDistance)
      {
        MinDistance   = CurrentDistance;
        TriangleIndex = i;
      }
    }

    vec3 SupportA = Support(MeshA, Polytope[TriangleIndex].Normal, ModelAMatrix);
    vec3 SupportB = Support(MeshB, -Polytope[TriangleIndex].Normal, ModelBMatrix);
    vec3 NewPoint = SupportA - SupportB;

    Result = Polytope[TriangleIndex].Normal * Math::Dot(NewPoint, Polytope[TriangleIndex].Normal);
#if DEBUG_COLLISION
    if(Iteration == IterationCount - 1)
    {
      printf("NewPoint Distance = %.16f\n", Math::Dot(NewPoint, Polytope[TriangleIndex].Normal));
      printf("Result Length = %.16f\n", Math::Length(Result));
      printf("MinDistance = %.16f\n", MinDistance);
      printf("PrevMinDistance = %.16f\n", PrevMinDistance);
      printf("================\n");
      Debug::PushLine({}, MinDistance * Polytope[TriangleIndex].Normal, { 1, 1, 0, 1 });
      Debug::PushWireframeSphere(NewPoint, 0.05f, { 0, 1, 0, 1 });
    }
#endif
    if(fabs(PrevMinDistance - MinDistance) < MinThreshold)
    {
      float U, V, W;

      BarycentricCoordinates(&U, &V, &W, NewPoint, Polytope[TriangleIndex].A,
                             Polytope[TriangleIndex].B, Polytope[TriangleIndex].C);

      *CollisionPoint  = U * SupportA + V * SupportA + W * SupportA;
      vec3 SecondPoint = U * SupportB + V * SupportB + W * SupportB;

#if DEBUG_SHOW_RESULT
// Debug::PushLine(SecondPoint, SecondPoint + Result, { 1, 1, 0, 1 });
// Debug::PushWireframeSphere(SecondPoint + Result, 0.05f, { 1, 1, 0, 1 });
// Debug::PushLine(*CollisionPoint, *CollisionPoint - Result, { 0, 1, 0, 1 });
// Debug::PushWireframeSphere(*CollisionPoint - Result, 0.05f, { 0, 1, 0, 1 });
#endif

      return -Result;
    }

    edge    Edges[100];
    int32_t EdgeCount = 0;

    Iteration++;

    if(Iteration < IterationCount)
    {
      for(int i = 0; i < TriangleCount; i++)
      {
        if(Math::Dot(Polytope[i].Normal, NewPoint - Polytope[i].A) > MinThreshold)
        {
#if DEBUG_COLLISION
          if(Iteration == IterationCount - 1)
          {
            vec3 NormalStart =
              0.33f * Polytope[i].A + 0.33f * Polytope[i].B + 0.33f * Polytope[i].C;
            vec3 NormalEnd = NormalStart + Polytope[i].Normal;
            Debug::PushLine(NormalStart, NormalEnd, { 0, 1, 0, 1 });
            Debug::PushWireframeSphere(NormalEnd, 0.05f);
          }
#endif
          int32_t EdgeIndex = FindEdge(Edges, EdgeCount, Polytope[i].A, Polytope[i].B);
          if(EdgeIndex != -1)
          {
            for(int k = EdgeIndex; k < EdgeCount - 1; k++)
            {
              Edges[k] = Edges[k + 1];
            }
            --EdgeCount;
          }
          else
          {
            Edges[EdgeCount].A = Polytope[i].A;
            Edges[EdgeCount].B = Polytope[i].B;
            ++EdgeCount;
          }

          EdgeIndex = FindEdge(Edges, EdgeCount, Polytope[i].B, Polytope[i].C);
          if(EdgeIndex != -1)
          {
            for(int k = EdgeIndex; k < EdgeCount - 1; k++)
            {
              Edges[k] = Edges[k + 1];
            }
            --EdgeCount;
          }
          else
          {
            Edges[EdgeCount].A = Polytope[i].B;
            Edges[EdgeCount].B = Polytope[i].C;
            ++EdgeCount;
          }

          EdgeIndex = FindEdge(Edges, EdgeCount, Polytope[i].C, Polytope[i].A);
          if(EdgeIndex != -1)
          {
            for(int k = EdgeIndex; k < EdgeCount - 1; k++)
            {
              Edges[k] = Edges[k + 1];
            }
            --EdgeCount;
          }
          else
          {
            Edges[EdgeCount].A = Polytope[i].C;
            Edges[EdgeCount].B = Polytope[i].A;
            ++EdgeCount;
          }

          for(int j = i; j < TriangleCount - 1; j++)
          {
            Polytope[j] = Polytope[j + 1];
          }
          --i;
          --TriangleCount;
        }
      }
    }

    for(int i = 0; i < EdgeCount; i++)
    {
      Polytope[TriangleCount].A = Edges[i].A;
      Polytope[TriangleCount].B = Edges[i].B;
      Polytope[TriangleCount].C = NewPoint;
      Polytope[TriangleCount].Normal =
        Math::Normalized(Math::Cross(Polytope[TriangleCount].B - Polytope[TriangleCount].A,
                                     Polytope[TriangleCount].C - Polytope[TriangleCount].A));

      ++TriangleCount;
    }
    PrevMinDistance = MinDistance;
  }
  return {};
}

// ==================
// SAT Implementation
// ==================
// TODO(rytis): Manifold point reduction (face case)

struct sat_contact_point
{
  vec3  Position;
  float Penetration;
};

struct sat_contact_manifold
{
  int32_t           PointCount;
  sat_contact_point Points[MAX_CONTACT_POINTS];
  vec3              Normal;
  bool              NormalFromA;
};

struct vertex
{
  vertex* Next;
  vertex* Previous;

  vec3 Position;
};

struct face;

struct half_edge
{
  vertex* Tail;

  half_edge* Next;
  half_edge* Previous;
  half_edge* Twin;

  face* Face;
};

struct face
{
  half_edge* Edge;

  vertex* ConflictListHead;

  vec3 Centroid;
  vec3 Normal;
};

struct hull
{
  vec3 Centroid;

  int32_t VertexCount;
  vertex  Vertices[30];

  int32_t   EdgeCount;
  half_edge Edges[60];

  int32_t FaceCount;
  face    Faces[10];
};

vec3
HullSupport(const hull* Hull, vec3 Direction)
{
  vec3 Vertex = Hull->Vertices[0].Position;

  float   Max   = Math::Dot(Vertex, Direction);
  int32_t Index = 0;

  float DotProduct;

  for(int i = 1; i < Hull->VertexCount; i++)
  {
    Vertex     = Hull->Vertices[i].Position;
    DotProduct = Math::Dot(Vertex, Direction);
    if(DotProduct > Max)
    {
      Max   = DotProduct;
      Index = i;
    }
  }

  return Hull->Vertices[Index].Position;
}

void
CalculateFaceCentroid(face* Face)
{
  vec3 Centroid = {};
  int  Count    = 0;

  half_edge* Edge = Face->Edge;
  half_edge* i    = Edge;
  do
  {
    Centroid += i->Tail->Position;
    i = i->Next;
    ++Count;
  } while(i != Edge);

  Face->Centroid = Centroid / Count;
}

void
CalculateFaceNormal(face* Face)
{
  vec3 A = Face->Edge->Tail->Position;
  vec3 B = Face->Edge->Next->Tail->Position;
  vec3 C = Face->Edge->Next->Next->Tail->Position;

  Face->Normal = Math::Normalized(Math::Cross(B - A, C - A));
}

struct face_query
{
  int32_t Index;
  float   Separation;
};

struct edge_query
{
  int32_t IndexA;
  int32_t IndexB;
  float   Separation;
};

void
TransformedFaceParameters(vec3* Centroid, vec3* Normal, const face* Face, mat4 Transform)
{
  *Centroid = TransformVector(Face->Centroid, Transform);

  mat4 NormalMatrix = Math::Transposed4(Math::InvMat4(Transform));
  *Normal = Math::Vec4ToVec3(Math::MulMat4Vec4(NormalMatrix, Math::Vec4(Face->Normal, 0)));
}

face_query
QueryFaceDirections(const mat4 TransformA, const hull* HullA, const mat4 TransformB,
                    const hull* HullB)
{
  face_query Result;

  // Local space of HullB
  mat4 Transform = Math::MulMat4(Math::InvMat4(TransformB), TransformA);

  vec3 Centroid;
  vec3 Normal;
  TransformedFaceParameters(&Centroid, &Normal, &HullA->Faces[0], Transform);
  vec3 SupportPoint = HullSupport(HullB, -Normal);

  int32_t MaxIndex      = 0;
  float   MaxSeparation = PointToPlaneDistance(SupportPoint, Centroid, Normal);

  for(int i = 1; i < HullA->FaceCount; ++i)
  {
    TransformedFaceParameters(&Centroid, &Normal, &HullA->Faces[i], Transform);
    SupportPoint = HullSupport(HullB, -Normal);

    float Separation = PointToPlaneDistance(SupportPoint, Centroid, Normal);
    if(Separation > MaxSeparation)
    {
      MaxIndex      = i;
      MaxSeparation = Separation;
    }
  }

  Result.Index      = MaxIndex;
  Result.Separation = MaxSeparation;
  return Result;
}

bool
IsMinkowskiFace(vec3 A, vec3 B, vec3 BA, vec3 C, vec3 D, vec3 DC)
{
  float CBA = Math::Dot(C, BA);
  float DBA = Math::Dot(D, BA);
  float ADC = Math::Dot(A, DC);
  float BDC = Math::Dot(B, DC);

  return (CBA * DBA < 0.0f) && (ADC * BDC < 0.0f) && (CBA * BDC > 0.0f);
}

float
Project(vec3 PointA, vec3 EdgeA, vec3 PointB, vec3 EdgeB, vec3 CentroidA)
{
  vec3 Axis = Math::Cross(EdgeA, EdgeB);

  const float kTolerance = 0.005f;

  float L = Math::Length(Axis);
  if(L < kTolerance * sqrt(Math::Length(EdgeA) * Math::Length(EdgeA) * Math::Length(EdgeB) *
                           Math::Length(EdgeB)))
  {
    return -FLT_MAX;
  }

  vec3 Normal = Math::Normalized(Axis);
  if(Math::Dot(Normal, PointA - CentroidA) < 0.0f)
  {
    Normal = -Normal;
  }

  return Math::Dot(Normal, PointB - PointA);
}

edge_query
QueryEdgeDirections(mat4 TransformA, const hull* HullA, mat4 TransformB, const hull* HullB)
{
  edge_query Result;
  int32_t    MaxIndexA     = -1;
  int32_t    MaxIndexB     = -1;
  float      MaxSeparation = -FLT_MAX;

  // Local space of HullB
  mat4 Transform = Math::MulMat4(Math::InvMat4(TransformB), TransformA);

  vec3 CentroidA = TransformVector(HullA->Centroid, Transform);

  for(int i = 0; i < HullA->EdgeCount; ++i)
  {
    const half_edge* HalfEdgeA = &HullA->Edges[i];
    vec3             EdgeATail = TransformVector(HalfEdgeA->Tail->Position, Transform);
    vec3             EdgeAHead = TransformVector(HalfEdgeA->Next->Tail->Position, Transform);
    vec3             EdgeA     = EdgeAHead - EdgeATail;

    vec3 FaceACenter;
    vec3 FaceNormalA;
    vec3 TwinFaceNormalA;
    TransformedFaceParameters(&FaceACenter, &FaceNormalA, HalfEdgeA->Face, Transform);
    TransformedFaceParameters(&FaceACenter, &TwinFaceNormalA, HalfEdgeA->Twin->Face, Transform);

    for(int j = 0; j < HullB->EdgeCount; ++j)
    {
      const half_edge* HalfEdgeB = &HullB->Edges[j];
      vec3             EdgeBTail = HalfEdgeB->Tail->Position;
      vec3             EdgeBHead = HalfEdgeB->Next->Tail->Position;
      vec3             EdgeB     = EdgeBHead - EdgeBTail;

      vec3 FaceNormalB     = HalfEdgeB->Face->Normal;
      vec3 TwinFaceNormalB = HalfEdgeB->Twin->Face->Normal;

      if(IsMinkowskiFace(FaceNormalA, TwinFaceNormalA, -EdgeA, -FaceNormalB, -TwinFaceNormalB,
                         -EdgeB))
      {
        float Separation = Project(EdgeATail, EdgeA, EdgeBTail, EdgeB, CentroidA);

        if(Separation > MaxSeparation)
        {
          MaxIndexA     = i;
          MaxIndexB     = j;
          MaxSeparation = Separation;
        }
      }
    }
  }

  Result.IndexA     = MaxIndexA;
  Result.IndexB     = MaxIndexB;
  Result.Separation = MaxSeparation;
  return Result;
}

vec3
IntersectEdgePlane(vec3 EdgeTail, vec3 EdgeHead, vec3 PlanePoint, vec3 PlaneNormal)
{
  vec3  IntersectionPoint;
  vec3  Edge = EdgeHead - EdgeTail;
  float d    = Math::Dot(PlaneNormal, PlanePoint);
  float t    = (d - Math::Dot(PlaneNormal, EdgeTail)) / Math::Dot(PlaneNormal, Edge);

  if(t >= 0.0f && t <= 1.0f)
  {
    IntersectionPoint = EdgeTail + t * Edge;
  }
  else if(t < 0.0f)
  {
    IntersectionPoint = EdgeTail;
  }
  else
  {
    IntersectionPoint = EdgeHead;
  }
  return IntersectionPoint;
}

int32_t
ClipPolygonToPlane(vec3* Polygon, int32_t PolygonPointCount, vec3 PlanePoint, vec3 PlaneNormal)
{
  vec3    NewPolygon[MAX_CONTACT_POINTS];
  int32_t VertexCount = 0;

  vec3 Tail = Polygon[PolygonPointCount - 1];
  vec3 Head;

  float TailDistance = PointToPlaneDistance(Tail, PlanePoint, PlaneNormal);
  float HeadDistance;

  for(int i = 0; i < PolygonPointCount; ++i)
  {
    Head = Polygon[i];

    HeadDistance = PointToPlaneDistance(Head, PlanePoint, PlaneNormal);

    if(TailDistance <= 0.0f && HeadDistance <= 0.0f)
    {
      NewPolygon[VertexCount++] = Head;
    }
    else if(TailDistance <= 0.0f && HeadDistance > 0.0f)
    {
      NewPolygon[VertexCount++] = IntersectEdgePlane(Tail, Head, PlanePoint, PlaneNormal);
    }
    else if(TailDistance > 0.0f && HeadDistance <= 0.0f)
    {
      NewPolygon[VertexCount++] = IntersectEdgePlane(Tail, Head, PlanePoint, PlaneNormal);
      NewPolygon[VertexCount++] = Head;
    }

    Tail         = Head;
    TailDistance = HeadDistance;
  }

  for(int i = 0; i < VertexCount; ++i)
  {
    Polygon[i] = NewPolygon[i];
  }

  return VertexCount;
}

int32_t
ClipPolygonToFace(vec3* Polygon, int32_t PolygonVertexCount, const face* Face, const mat4 Transform)
{
  half_edge* FaceEdge = Face->Edge;
  half_edge* r        = FaceEdge;

  int32_t VertexCount = PolygonVertexCount;
  do
  {
    vec3 TwinCentroid;
    vec3 TwinNormal;
    TransformedFaceParameters(&TwinCentroid, &TwinNormal, r->Twin->Face, Transform);

    VertexCount = ClipPolygonToPlane(Polygon, VertexCount, TwinCentroid, TwinNormal);

    r = r->Next;
  } while(r != FaceEdge);

  return VertexCount;
}

int32_t
ReducePolygon(vec3* Polygon, int32_t PolygonPointCount, vec3 ReferenceFaceCentroid,
              vec3 ReferenceFaceNormal)
{
  for(int i = 0; i < PolygonPointCount; ++i)
  {
    if(PointToPlaneDistance(Polygon[i], ReferenceFaceCentroid, ReferenceFaceNormal) >= 0.0f)
    {
      for(int j = i; j < PolygonPointCount - 1; ++j)
      {
        Polygon[j] = Polygon[j + 1];
      }
      --PolygonPointCount;
      --i;
    }
  }

  if(PolygonPointCount > 4)
  {
    vec3 ProjectedPolygon[MAX_CONTACT_POINTS];

    int32_t MinSeparationIndex = -1;
    float   MinSeparation      = FLT_MAX;

    for(int i = 0; i < PolygonPointCount; ++i)
    {
      float Separation =
        PointToPlaneDistance(Polygon[i], ReferenceFaceCentroid, ReferenceFaceNormal);

      if(Separation < MinSeparation)
      {
        MinSeparationIndex = i;
        MinSeparation      = Separation;
      }

      ProjectedPolygon[i] =
        ClosestPointInPlane(Polygon[i], ReferenceFaceCentroid, ReferenceFaceNormal);
    }

    vec3 A = ProjectedPolygon[MinSeparationIndex];

    int32_t MaxLengthIndex = -1;
    float   MaxLength      = -FLT_MAX;

    for(int i = 0; i < PolygonPointCount; ++i)
    {
      vec3  B      = ProjectedPolygon[i];
      vec3  Vector = B - A;
      float Length = Math::Length(Vector);
      if(Length > MaxLength)
      {
        MaxLengthIndex = i;
        MaxLength      = Length;
      }
    }

    vec3 B = ProjectedPolygon[MaxLengthIndex];

    int32_t MaxAreaIndex = -1;
    float   MaxArea      = -FLT_MAX;

    for(int i = 0; i < PolygonPointCount; ++i)
    {
      vec3  C    = ProjectedPolygon[i];
      float Area = 0.5f * Math::Dot(Math::Cross(A - C, B - C), ReferenceFaceNormal);
      if(Area > MaxArea)
      {
        MaxAreaIndex = i;
        MaxArea      = Area;
      }
    }

    vec3 C = ProjectedPolygon[MaxAreaIndex];

    int32_t MinAreaIndex = -1;
    float   MinArea      = FLT_MAX;

    for(int i = 0; i < PolygonPointCount; ++i)
    {
      vec3  Q       = ProjectedPolygon[i];
      float AreaQAB = 0.5f * Math::Dot(Math::Cross(A - Q, B - Q), ReferenceFaceNormal);
      if(AreaQAB < 0.0f)
      {
        if(AreaQAB < MinArea)
        {
          MinAreaIndex = i;
          MinArea      = AreaQAB;
        }
      }

      float AreaQBC = 0.5f * Math::Dot(Math::Cross(B - Q, C - Q), ReferenceFaceNormal);
      if(AreaQBC < 0.0f)
      {
        if(AreaQBC < MinArea)
        {
          MinAreaIndex = i;
          MinArea      = AreaQBC;
        }
      }

      float AreaQCA = 0.5f * Math::Dot(Math::Cross(C - Q, A - Q), ReferenceFaceNormal);
      if(AreaQCA < 0.0f)
      {
        if(AreaQCA < MinArea)
        {
          MinAreaIndex = i;
          MinArea      = AreaQCA;
        }
      }
    }

    vec3    NewPolygon[MAX_CONTACT_POINTS];
    int32_t VertexCount = 4;

    NewPolygon[0] = Polygon[MinSeparationIndex];
    NewPolygon[1] = Polygon[MaxLengthIndex];
    NewPolygon[2] = Polygon[MaxAreaIndex];
    NewPolygon[3] = Polygon[MinAreaIndex];

    for(int i = 0; i < VertexCount; ++i)
    {
      Polygon[i] = NewPolygon[i];
    }
    return VertexCount;
  }
  return PolygonPointCount;
}

void
CreateFaceContact(sat_contact_manifold* Manifold, face_query QueryA, const mat4 TransformA,
                  const hull* HullA, const mat4 TransformB, const hull* HullB)
{
  // Local space of HullB
  mat4 Transform = Math::MulMat4(Math::InvMat4(TransformB), TransformA);

  vec3 Centroid;
  vec3 Normal;
  TransformedFaceParameters(&Centroid, &Normal, &HullA->Faces[QueryA.Index], Transform);

  half_edge* ReferenceFaceEdge = HullA->Faces[QueryA.Index].Edge;

  int32_t Index         = 0;
  float   MinDotProduct = Math::Dot(Normal, HullB->Faces[0].Normal);
  for(int i = 1; i < HullB->FaceCount; ++i)
  {
    float DotProduct = Math::Dot(Normal, HullB->Faces[i].Normal);
    if(DotProduct < MinDotProduct)
    {
      Index         = i;
      MinDotProduct = DotProduct;
    }
  }

  half_edge* IncidentFaceEdge = HullB->Faces[Index].Edge;

  if(g_VisualizeContactManifold)
  {
    half_edge* a = ReferenceFaceEdge;

    do
    {
      Debug::PushLine(TransformVector(a->Tail->Position, TransformA),
                      TransformVector(a->Next->Tail->Position, TransformA), { 0, 0, 1, 1 });
      a = a->Next;
    } while(a != ReferenceFaceEdge);

    half_edge* b = IncidentFaceEdge;

    do
    {
      Debug::PushLine(TransformVector(b->Tail->Position, TransformB),
                      TransformVector(b->Next->Tail->Position, TransformB), { 1, 0, 0, 1 });
      b = b->Next;
    } while(b != IncidentFaceEdge);
  }

  vec3    Polygon[MAX_CONTACT_POINTS];
  int32_t PolygonPointCount = 0;

  half_edge* i = IncidentFaceEdge;

  do
  {
    Polygon[PolygonPointCount++] = i->Tail->Position;
    i                            = i->Next;
  } while(i != IncidentFaceEdge);

  PolygonPointCount =
    ClipPolygonToFace(Polygon, PolygonPointCount, &HullA->Faces[QueryA.Index], Transform);
  PolygonPointCount = ReducePolygon(Polygon, PolygonPointCount, Centroid, Normal);

  mat4 NormalMatrix = Math::Transposed4(Math::InvMat4(TransformB));
  Manifold->Normal =
    Math::Normalized(Math::Vec4ToVec3(Math::MulMat4Vec4(NormalMatrix, Math::Vec4(Normal, 0))));
  Manifold->PointCount = 0;

  for(int i = 0; i < PolygonPointCount; ++i)
  {
    float Penetration = PointToPlaneDistance(Polygon[i], Centroid, Normal);
    assert(Penetration < 0.0f);

    Manifold->Points[Manifold->PointCount].Position    = TransformVector(Polygon[i], TransformB);
    Manifold->Points[Manifold->PointCount].Penetration = Penetration;
    ++Manifold->PointCount;
  }
}

float
Clamp(float N, float Min, float Max)
{
  if(N < Min)
  {
    return Min;
  }
  if(N > Max)
  {
    return Max;
  }
  return N;
}

void
ClosestPointsEdgeEdge(vec3* ClosestA, vec3* ClosestB, vec3 EdgeAStart, vec3 EdgeAEnd,
                      vec3 EdgeBStart, vec3 EdgeBEnd)
{
  float S;
  float T;

  vec3 EdgeA = EdgeAEnd - EdgeAStart;
  vec3 EdgeB = EdgeBEnd - EdgeBStart;
  vec3 R     = EdgeAStart - EdgeBStart;

  float A = Math::Dot(EdgeA, EdgeA); // Squared length of EdgeA
  float E = Math::Dot(EdgeB, EdgeB); // Squared length of EdgeB
  float F = Math::Dot(EdgeB, R);

  if(A <= FLT_EPSILON && E <= FLT_EPSILON)
  {
    S = T     = 0.0f;
    *ClosestA = EdgeAStart;
    *ClosestB = EdgeBStart;
    return;
  }
  if(A <= FLT_EPSILON)
  {
    S = 0.0f;
    T = F / E;
    T = Clamp(T, 0.0f, 1.0f);
  }
  else
  {
    float C = Math::Dot(EdgeA, R);
    if(E <= FLT_EPSILON)
    {
      T = 0.0f;
      S = Clamp(-C / A, 0.0f, 1.0f);
    }
    else
    {
      float B           = Math::Dot(EdgeA, EdgeB);
      float Denominator = A * E - B * B;

      if(Denominator != 0.0f)
      {
        S = Clamp((B * F - C * E) / Denominator, 0.0f, 1.0f);
      }
      else
      {
        S = 0.0f;
      }

      T = (B * S + F) / E;

      if(T < 0.0f)
      {
        T = 0.0f;
        S = Clamp(-C / A, 0.0f, 1.0f);
      }
      else if(T > 1.0f)
      {
        T = 1.0f;
        S = Clamp((B - C) / A, 0.0f, 1.0f);
      }
    }
  }

  *ClosestA = EdgeAStart + EdgeA * S;
  *ClosestB = EdgeBStart + EdgeB * T;
}

void
CreateEdgeContact(sat_contact_manifold* Manifold, edge_query EdgeQuery, mat4 TransformA,
                  const hull* HullA, mat4 TransformB, const hull* HullB)
{
  // Local space of HullB
  mat4 Transform = Math::MulMat4(Math::InvMat4(TransformB), TransformA);

  vec3 ClosestA;
  vec3 ClosestB;

  const half_edge* EdgeA = &HullA->Edges[EdgeQuery.IndexA];
  const half_edge* EdgeB = &HullB->Edges[EdgeQuery.IndexB];

  vec3 EdgeATail = TransformVector(EdgeA->Tail->Position, Transform);
  vec3 EdgeAHead = TransformVector(EdgeA->Next->Tail->Position, Transform);

  ClosestPointsEdgeEdge(&ClosestA, &ClosestB, EdgeATail, EdgeAHead, EdgeB->Tail->Position,
                        EdgeB->Next->Tail->Position);

  vec3 Midpoint = (ClosestA + ClosestB) / 2.0f;

  Manifold->PointCount            = 1;
  Manifold->Points[0].Penetration = EdgeQuery.Separation;
  Manifold->Normal                = Math::Normalized(
    Math::Cross(EdgeAHead - EdgeATail, EdgeB->Next->Tail->Position - EdgeB->Tail->Position));
  if(Math::Dot(Manifold->Normal, EdgeATail - TransformVector(HullA->Centroid, Transform)) < 0.0f)
  {
    Manifold->Normal = -Manifold->Normal;
  }
  // Manifold->Normal =
  // TransformVector(Manifold->Normal, TransformB) - TransformVector({}, TransformB);

  mat4 NormalMatrix = Math::Transposed4(Math::InvMat4(TransformB));
  Manifold->Normal  = Math::Normalized(
    Math::Vec4ToVec3(Math::MulMat4Vec4(NormalMatrix, Math::Vec4(Manifold->Normal, 0))));

  Manifold->Points[0].Position = TransformVector(Midpoint, TransformB);
#if 0
#define DEBUG_QUEIRES 0
#if DEBUG_QUERIES
#endif
#endif
  if(g_VisualizeContactManifold)
  {
    Debug::PushLine(TransformVector(EdgeA->Tail->Position, TransformA),
                    TransformVector(EdgeA->Next->Tail->Position, TransformA), { 0, 0, 1, 1 });
    Debug::PushLine(TransformVector(EdgeB->Tail->Position, TransformB),
                    TransformVector(EdgeB->Next->Tail->Position, TransformB), { 1, 0, 0, 1 });
  }
}

bool
SAT(sat_contact_manifold* Manifold, const mat4 TransformA, const hull* HullA, const mat4 TransformB,
    const hull* HullB)
{
  TIMED_BLOCK(SAT);
  const float EDGE_THRESHOLD = 0.0001f; // FLT_EPSILON;
  const float FACE_THRESHOLD = 0.1f;    // FLT_EPSILON;

  const face_query FaceQueryA = QueryFaceDirections(TransformA, HullA, TransformB, HullB);
  if(FaceQueryA.Separation > 0.0f)
  {
    return false;
  }

  face_query FaceQueryB = QueryFaceDirections(TransformB, HullB, TransformA, HullA);
  if(FaceQueryB.Separation > 0.0f)
  {
    return false;
  }

  edge_query EdgeQuery = QueryEdgeDirections(TransformA, HullA, TransformB, HullB);
  if(EdgeQuery.Separation > 0.0f)
  {
    return false;
  }

  // Change if order to be more efficent
  if(EdgeQuery.Separation < MaxFloat(FaceQueryA.Separation, FaceQueryB.Separation) + EDGE_THRESHOLD)
  {
    if(FaceQueryB.Separation + FACE_THRESHOLD < FaceQueryA.Separation)
    {
      // printf("FaceA Manifold\n");
      CreateFaceContact(Manifold, FaceQueryA, TransformA, HullA, TransformB, HullB);
      Manifold->NormalFromA = true;
    }
    else
    {
      // printf("FaceB Manifold\n");
      CreateFaceContact(Manifold, FaceQueryB, TransformB, HullB, TransformA, HullA);
      Manifold->NormalFromA = false;
    }
  }
  else
  {
    // printf("Edge Manifold\n");
    CreateEdgeContact(Manifold, EdgeQuery, TransformA, HullA, TransformB, HullB);
    Manifold->NormalFromA = true;
  }

  if(g_VisualizeContactPoints)
  {
    for(int i = 0; i < Manifold->PointCount; ++i)
    {
      Debug::PushWireframeSphere(Manifold->Points[i].Position, 0.05f, { 1, 1, 1, 1 });
    }
  }

  return true;
}
