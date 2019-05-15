#pragma once

#include <math.h>
#include <xmmintrin.h>

#define PI 3.1415926535f

#define ObMin(A, B) ((A) < (B) ? (A) : (B))
#define ObMax(A, B) ((A) > (B) ? (A) : (B))

// NOTE(rytis): Linear types

union ov2
{
    struct
    {
        f32 x, y;
    };

    struct
    {
        f32 u, v;
    };

    f32 E[2];
};

union ov3
{
    struct
    {
        f32 x, y, z;
    };

    f32 E[3];
};

union ov4
{
    union
    {
        struct
        {
            f32 x, y, z, w;
        };

        struct
        {
            ov3 xyz;
            // f32 w;
        };
    };

    struct
    {
        f32 r, g, b, a;
    };

    f32 E[4];
};

// NOTE(rytis): Matrices are ROW MAJOR (E[ROW][COLUMN])!!!
union om3
{
    struct
    {
        f32 _11, _12, _13;
        f32 _21, _22, _23;
        f32 _31, _32, _33;
    };

    f32 E[3][3];
};

union om3x4
{
    struct
    {
        f32 _11, _12, _13, _14;
        f32 _21, _22, _23, _24;
        f32 _31, _32, _33, _34;
    };

    f32 E[3][4];
};

union om4
{
    struct
    {
        f32 _11, _12, _13, _14;
        f32 _21, _22, _23, _24;
        f32 _31, _32, _33, _34;
        f32 _41, _42, _43, _44;
    };

    f32 E[4][4];
};

// NOTE(rytis): Intrinsic operations

inline f32
ObAbsoluteValue(f32 A)
{
    return A < 0.0f ? -A : A;
}

inline f32
ObSquare(f32 A)
{
    return A * A;
}

inline f32
ObSquareRoot(f32 Value)
{
    f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Value)));
    return Result;
}

inline f32
ObSin(f32 Angle)
{
    return sinf(Angle);
}

inline f32
ObCos(f32 Angle)
{
    return cosf(Angle);
}

inline s32
TruncateF32ToS32(f32 F32)
{
    return (s32)F32;
}

inline u32
TruncateF32ToU32(f32 F32)
{
    return (u32)F32;
}

// NOTE(rytis): Scalar operations

inline f32
ObLerp(f32 A, f32 t, f32 B)
{
    return (1.0f - t) * A + t * B;
}

inline f32
ObClamp(f32 Min, f32 N, f32 Max)
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

inline f32
ObClamp01(f32 Value)
{
    return ObClamp(0.0f, Value, 1.0f);
}

inline f32
ObClamp01MapToRange(f32 Min, f32 t, f32 Max)
{
    f32 Result = 0.0f;

    f32 Range = Max - Min;
    if(Range != 0.0f)
    {
        Result = ObClamp01((t - Min) / Range);
    }
    
    return Result;
}

inline f32
ObFloor(f32 Value)
{
    return (f32)TruncateF32ToS32(Value);
}

inline f32
ObRound(f32 Value)
{
    return (f32)TruncateF32ToS32(Value + 0.5f);
}

inline f32
ObCeil(f32 Value)
{
    return (f32)TruncateF32ToS32(Value + 1.0f);
}

// NOTE(rytis): v3 operations

inline ov3
operator*(f32 A, ov3 B)
{
    ov3 Result;
    Result.x = A * B.x;
    Result.y = A * B.y;
    Result.z = A * B.z;
    return Result;
}

inline ov3
operator*(ov3 B, f32 A)
{
    ov3 Result = A * B;
    return Result;
}

inline ov3&
operator*=(ov3& B, f32 A)
{
    B = A * B;
    return B;
}

inline ov3
operator/(f32 A, ov3 B)
{
    ov3 Result = (1.0f / A) * B;
    return Result;
}

inline ov3
operator/(ov3 B, f32 A)
{
    ov3 Result = A / B;
    return Result;
}

inline ov3&
operator/=(ov3& B, f32 A)
{
    B = B / A;
    return B;
}

inline ov3
operator-(ov3 A)
{
    ov3 Result;
    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    return Result;
}

inline ov3
operator+(ov3 A, ov3 B)
{
    ov3 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    return Result;
}

inline ov3&
operator+=(ov3& A, ov3 B)
{
    A = A + B;
    return A;
}

inline ov3
operator-(ov3 A, ov3 B)
{
    ov3 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    return Result;
}

inline ov3&
operator-=(ov3& A, ov3 B)
{
    A = A - B;
    return A;
}

inline f32
ObInner(ov3 A, ov3 B)
{
    f32 Result = A.x * B.x + A.y * B.y + A.z * B.z;
    return Result;
}

inline f32
ObLengthSq(ov3 A)
{
    f32 Result = ObInner(A, A);
    return Result;
}

inline f32
ObLength(ov3 A)
{
    f32 Result = ObSquareRoot(ObLengthSq(A));
    return Result;
}

inline ov3
ObNormalize(ov3 A)
{
    ov3 Result = A * (1.0f / ObLength(A));
    return Result;
}

inline ov3
ObNOZ(ov3 A)
{
    ov3 Result = {};

    f32 LengthSq = ObLengthSq(A);
    if(LengthSq > ObSquare(0.0001f))
    {
        Result = ObNormalize(A);
    }

    return Result;
}

inline ov3
ObCross(ov3 A, ov3 B)
{
    ov3 Result;
    Result.x = A.y * B.z - A.z * B.y;
    Result.y = A.z * B.x - A.x * B.z;
    Result.z = A.x * B.y - A.y * B.x;
    return Result;
}

inline ov3
ObClamp01(ov3 A)
{
    ov3 Result;
    Result.x = ObClamp01(A.x);
    Result.y = ObClamp01(A.y);
    Result.z = ObClamp01(A.z);
    return Result;
}

inline ov3
ObLerp(ov3 A, f32 t, ov3 B)
{
    ov3 Result = (1.0f - t) * A + t * B;
    return Result;
}

// NOTE(rytis): v4 operations

inline ov4
operator*(f32 A, ov4 B)
{
    ov4 Result;
    Result.x = A * B.x;
    Result.y = A * B.y;
    Result.z = A * B.z;
    Result.w = A * B.w;
    return Result;
}

inline ov4
operator*(ov4 B, f32 A)
{
    ov4 Result = A * B;
    return Result;
}

inline ov4&
operator*=(ov4& B, f32 A)
{
    B = A * B;
    return B;
}

inline ov4
operator/(f32 A, ov4 B)
{
    ov4 Result = (1.0f / A) * B;
    return Result;
}

inline ov4
operator/(ov4 B, f32 A)
{
    ov4 Result = A / B;
    return Result;
}

inline ov4&
operator/=(ov4& B, f32 A)
{
    B = B / A;
    return B;
}

inline ov4
operator-(ov4 A)
{
    ov4 Result;
    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    Result.w = -A.w;
    return Result;
}

inline ov4
operator+(ov4 A, ov4 B)
{
    ov4 Result;
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    Result.w = A.w + B.w;
    return Result;
}

inline ov4&
operator+=(ov4& A, ov4 B)
{
    A = A + B;
    return A;
}

inline ov4
operator-(ov4 A, ov4 B)
{
    ov4 Result;
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    Result.w = A.w - B.w;
    return Result;
}

inline ov4&
operator-=(ov4& A, ov4 B)
{
    A = A - B;
    return A;
}

inline f32
ObInner(ov4 A, ov4 B)
{
    f32 Result = A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
    return Result;
}

inline f32
ObLengthSq(ov4 A)
{
    f32 Result = ObInner(A, A);
    return Result;
}

inline f32
ObLength(ov4 A)
{
    f32 Result = ObSquareRoot(ObLengthSq(A));
    return Result;
}

inline ov4
ObNormalize(ov4 A)
{
    ov4 Result = A * (1.0f / ObLength(A));
    return Result;
}

inline ov4
ObNOZ(ov4 A)
{
    ov4 Result = {};

    f32 LengthSq = ObLengthSq(A);
    if(LengthSq > ObSquare(0.0001f))
    {
        Result = ObNormalize(A);
    }

    return Result;
}

inline ov4
ObClamp01(ov4 A)
{
    ov4 Result;
    Result.x = ObClamp01(A.x);
    Result.y = ObClamp01(A.y);
    Result.z = ObClamp01(A.z);
    Result.w = ObClamp01(A.w);
    return Result;
}

inline ov4
ObLerp(ov4 A, f32 t, ov4 B)
{
    ov4 Result = (1.0f - t) * A + t * B;
    return Result;
}

// NOTE(rytis): m3 operations

static ov3
ObTransform(om3 A, ov3 B)
{
    ov3 Result;
    Result.x = B.x * A._11 + B.y * A._12 + B.z * A._13;
    Result.y = B.x * A._21 + B.y * A._22 + B.z * A._23;
    Result.z = B.x * A._31 + B.y * A._32 + B.z * A._33;
    return Result;
}

inline ov3
operator*(om3 A, ov3 B)
{
    ov3 Result = ObTransform(A, B);
    return Result;
}

inline om3
ObIdentity3(f32 Scale = 1.0f)
{
    om3 Result = {};
    Result._11 = Scale;
    Result._22 = Scale;
    Result._33 = Scale;
    return Result;
}

inline f32
ObDeterminant(om3 M)
{
    return M._11 * (M._22 * M._33 - M._23 * M._32) -
           M._12 * (M._21 * M._33 - M._23 * M._31) +
           M._13 * (M._21 * M._32 - M._22 * M._31);
}

// NOTE(rytis): Credit goes to Inigo Quilez:
// http://www.iquilezles.org/www/articles/noacos/noacos.htm
static om3
ObRotationAlign(ov3 Start, ov3 Destination)
{
    if((ObLength(Start) == 0.0f) || (ObLength(Destination) == 0.0f))
    {
        return ObIdentity3();
    }

    // IMPORTANT(rytis): We assume that Start and Destination are normalized already.
#if 0
    Start = ObNormalize(Start);
    Destination = ObNormalize(Destination);
#endif

    ov3 Axis = ObCross(Start, Destination);
    f32 CosTheta = ObInner(Start, Destination);
    f32 K = 1.0f / (1.0f + CosTheta);

    om3 Result = {};

    Result._11 = Axis.x * Axis.x * K + CosTheta;
    Result._12 = Axis.y * Axis.x * K - Axis.z;
    Result._13 = Axis.z * Axis.x * K + Axis.y;

    Result._21 = Axis.x * Axis.y * K + Axis.z;
    Result._22 = Axis.y * Axis.y * K + CosTheta;
    Result._23 = Axis.z * Axis.y * K - Axis.x;

    Result._31 = Axis.x * Axis.z * K - Axis.y;
    Result._32 = Axis.y * Axis.z * K + Axis.x;
    Result._33 = Axis.z * Axis.z * K + CosTheta;

    return Result;
}

// NOTE(rytis): m4 operations

inline om4
ObIdentity4(f32 Scale = 1.0f)
{
    om4 Result = {};
    Result._11 = Scale;
    Result._22 = Scale;
    Result._33 = Scale;
    Result._44 = Scale;
    return Result;
}

inline f32
ObDeterminant(om4 M)
{
    return M._11 * (
                   M._22 * (M._33 * M._44 - M._34 * M._43) -
                   M._23 * (M._32 * M._44 - M._34 * M._42) +
                   M._24 * (M._32 * M._43 - M._33 * M._42)
                   ) -
           M._12 * (
                   M._21 * (M._33 * M._44 - M._34 * M._43) -
                   M._23 * (M._31 * M._44 - M._34 * M._41) +
                   M._24 * (M._31 * M._43 - M._33 * M._41)
                   ) +
           M._13 * (
                   M._21 * (M._32 * M._44 - M._34 * M._42) -
                   M._22 * (M._31 * M._44 - M._34 * M._41) +
                   M._24 * (M._31 * M._42 - M._32 * M._41)
                   ) -
           M._14 * (
                   M._21 * (M._32 * M._43 - M._33 * M._42) -
                   M._22 * (M._31 * M._43 - M._33 * M._41) +
                   M._23 * (M._31 * M._42 - M._32 * M._41)
                   );
}

static om4
ObInverse(om4 M)
{
    f32 Determinant = ObDeterminant(M);
    if(Determinant == 0.0f)
    {
        return ObIdentity4();
    }

    f32 ID = 1.0f / Determinant;

    om4 Result = {};

    Result._11 = ID * (
                 M._22 * (M._33 * M._44 - M._34 * M._43) -
                 M._23 * (M._32 * M._44 - M._34 * M._42) +
                 M._24 * (M._32 * M._43 - M._33 * M._42));
    Result._12 = -ID * (
                 M._12 * (M._33 * M._44 - M._34 * M._43) -
                 M._13 * (M._32 * M._44 - M._34 * M._42) +
                 M._14 * (M._32 * M._43 - M._33 * M._42));
    Result._13 = ID * (
                 M._12 * (M._23 * M._44 - M._24 * M._43) -
                 M._13 * (M._22 * M._44 - M._24 * M._42) +
                 M._14 * (M._22 * M._43 - M._23 * M._42));
    Result._14 = -ID * (
                 M._12 * (M._23 * M._34 - M._24 * M._33) -
                 M._13 * (M._22 * M._34 - M._24 * M._32) +
                 M._14 * (M._22 * M._33 - M._23 * M._32));

    Result._21 = -ID * (
                 M._21 * (M._33 * M._44 - M._34 * M._43) -
                 M._23 * (M._31 * M._44 - M._34 * M._41) +
                 M._24 * (M._31 * M._43 - M._33 * M._41));
    Result._22 = ID * (
                 M._11 * (M._33 * M._44 - M._34 * M._43) -
                 M._13 * (M._31 * M._44 - M._34 * M._41) +
                 M._14 * (M._31 * M._43 - M._33 * M._41));
    Result._23 = -ID * (
                 M._11 * (M._23 * M._44 - M._24 * M._43) -
                 M._13 * (M._21 * M._44 - M._24 * M._41) +
                 M._14 * (M._21 * M._43 - M._23 * M._41));
    Result._24 = ID * (
                 M._11 * (M._23 * M._34 - M._24 * M._33) -
                 M._13 * (M._21 * M._34 - M._24 * M._31) +
                 M._14 * (M._21 * M._33 - M._23 * M._31));

    Result._31 = ID * (
                 M._21 * (M._32 * M._44 - M._34 * M._42) -
                 M._22 * (M._31 * M._44 - M._34 * M._41) +
                 M._24 * (M._31 * M._42 - M._32 * M._41));
    Result._32 = -ID * (
                 M._11 * (M._32 * M._44 - M._34 * M._42) -
                 M._12 * (M._31 * M._44 - M._34 * M._41) +
                 M._14 * (M._31 * M._42 - M._32 * M._41));
    Result._33 = ID * (
                 M._11 * (M._22 * M._44 - M._24 * M._42) -
                 M._12 * (M._21 * M._44 - M._24 * M._41) +
                 M._14 * (M._21 * M._42 - M._22 * M._41));
    Result._34 = -ID * (
                 M._11 * (M._22 * M._34 - M._24 * M._32) -
                 M._12 * (M._21 * M._34 - M._24 * M._31) +
                 M._14 * (M._21 * M._32 - M._22 * M._31));

    Result._41 = -ID * (
                 M._21 * (M._32 * M._43 - M._33 * M._42) -
                 M._22 * (M._31 * M._43 - M._33 * M._41) +
                 M._23 * (M._31 * M._42 - M._32 * M._41));
    Result._42 = ID * (
                 M._11 * (M._32 * M._43 - M._33 * M._42) -
                 M._12 * (M._31 * M._43 - M._33 * M._41) +
                 M._13 * (M._31 * M._42 - M._32 * M._41));
    Result._43 = -ID * (
                 M._11 * (M._22 * M._43 - M._23 * M._42) -
                 M._12 * (M._21 * M._43 - M._23 * M._41) +
                 M._13 * (M._21 * M._42 - M._22 * M._41));
    Result._44 = ID * (
                 M._11 * (M._22 * M._33 - M._23 * M._32) -
                 M._12 * (M._21 * M._33 - M._23 * M._31) +
                 M._13 * (M._21 * M._32 - M._22 * M._31));

    return Result;
}
