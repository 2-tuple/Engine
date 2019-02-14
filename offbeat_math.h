#pragma once

#define PI 3.1415926535

inline r32
AbsoluteValue(r32 A)
{
    return A < 0.0f ? -A : A;
}

inline r32
Square(r32 A)
{
    return A * A;
}

inline r32
Lerp(r32 A, r32 t, r32 B)
{
    return (1.0f - t) * A + t * B;
}

inline r32
Clamp(r32 N, r32 Min, r32 Max)
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

inline r32
Clamp01(r32 Value)
{
    return Clamp(Value, 0.0f, 1.0f);
}

inline r32
Clamp01MapToRange(r32 Min, r32 t, r32 Max)
{
    r32 Result = 0.0f;

    r32 Range = Max - Min;
    if(Range != 0.0f)
    {
        Result = Clamp01((t - Min) / Range);
    }
    
    return Result;
}
