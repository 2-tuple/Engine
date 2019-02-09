#pragma once

inline float
Lerp(float A, float t, float B)
{
    return (1.0f - t) * A + t * B;
}

inline float
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

inline float
Clamp01(float Value)
{
    return Clamp(Value, 0.0f, 1.0f);
}

inline float
Clamp01MapToRange(float Min, float t, float Max)
{
    float Result = 0.0f;

    float Range = Max - Min;
    if(Range != 0.0f)
    {
        Result = Clamp01((t - Min) / Range);
    }
    
    return Result;
}
