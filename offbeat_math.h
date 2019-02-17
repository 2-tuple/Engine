#pragma once

#define PI 3.1415926535f

inline f32
AbsoluteValue(f32 A)
{
    return A < 0.0f ? -A : A;
}

inline f32
Square(f32 A)
{
    return A * A;
}

inline f32
Lerp(f32 A, f32 t, f32 B)
{
    return (1.0f - t) * A + t * B;
}

inline f32
Clamp(f32 N, f32 Min, f32 Max)
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
Clamp01(f32 Value)
{
    return Clamp(Value, 0.0f, 1.0f);
}

inline f32
Clamp01MapToRange(f32 Min, f32 t, f32 Max)
{
    f32 Result = 0.0f;

    f32 Range = Max - Min;
    if(Range != 0.0f)
    {
        Result = Clamp01((t - Min) / Range);
    }
    
    return Result;
}
