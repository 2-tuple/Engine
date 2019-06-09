#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <SDL2/SDL.h>

static bool g_PerformanceFrequencyInit = false;
static int64_t g_PerformanceFrequency;

namespace Platform
{
  void
  InitPerformanceFrequency()
  {
    if(!g_PerformanceFrequencyInit){
      LARGE_INTEGER PerformanceFrequencyResult;
      g_PerformanceFrequencyInit = QueryPerformanceFrequency(&PerformanceFrequencyResult);
      g_PerformanceFrequency = PerformanceFrequencyResult.QuadPart;
    }
  }

  int64_t
  GetCurrentCounter()
  {
    LARGE_INTEGER CurrentPerformanceCounter;
    QueryPerformanceCounter(&CurrentPerformanceCounter);
    return CurrentPerformanceCounter.QuadPart;
  }

  float
  GetTimeInSeconds()
  {
    InitPerformanceFrequency();

    LARGE_INTEGER CurrentPerformanceCounter;
    QueryPerformanceCounter(&CurrentPerformanceCounter);
    float Result = (float)CurrentPerformanceCounter.QuadPart / (float)g_PerformanceFrequency;
    return Result;
  }

  float
  GetTimeInSeconds(int64_t Start, int64_t End)
  {
      assert(End >= Start);
      return (float)(End - Start) / (float)g_PerformanceFrequency;
  }
}
