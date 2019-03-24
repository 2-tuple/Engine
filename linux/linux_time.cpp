#include <time.h>

namespace Platform
{
  void
  InitPerformanceFrequency()
  {
  }

  int64_t
  GetCurrentCounter()
  {
    struct timespec CurrentTime;
    clock_gettime(CLOCK_MONOTONIC_RAW, &CurrentTime);
    int64_t Result = CurrentTime.tv_sec * 1000000 + CurrentTime.tv_nsec * 0.001f;
    return Result;
  }

  float
  GetTimeInSeconds()
  {
    struct timespec CurrentTime;
    clock_gettime(CLOCK_MONOTONIC_RAW, &CurrentTime);
    float Result = (float)CurrentTime.tv_sec + (float)CurrentTime.tv_nsec / 1e9f;
    return Result;
  }

  float
  GetTimeInSeconds(int64_t Start, int64_t End)
  {
      assert(End >= Start);
      return (float)(End - Start) / 1000000.0f;
  }
}
