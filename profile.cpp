#include "profile.h"

#if defined(USE_DEBUG_PROFILING)
frame_endpoints     GLOBAL_FRAME_ENDPOINT_TABLE[PROFILE_MAX_FRAME_COUNT + 1];
timer_frame_summary GLOBAL_TIMER_FRAME_SUMMARY_TABLE[PROFILE_MAX_FRAME_COUNT + 1]
                                                    [ARRAY_COUNT(TIMER_NAME_TABLE)];
timer_event GLOBAL_FRAME_TIMER_EVENT_TABLE[PROFILE_MAX_FRAME_COUNT + 1]
                                          [PROFILE_MAX_TIMER_EVENTS_PER_FRAME];
int GLOBAL_TIMER_FRAME_EVENT_COUNT_TABLE[PROFILE_MAX_FRAME_COUNT + 1];
int g_CurrentProfilerFrameIndex = 0;
int g_SavedCurrentFrameIndex    = 0;
int g_CurrentTimerEventDepth    = 0;
int g_CurrentTimerEventCount    = 0;

timer_event_autoclose_wrapper::timer_event_autoclose_wrapper(int32_t BlockEnumValue)
{
  this->NameTableIndex = BlockEnumValue;
  this->IndexInFrame   = g_CurrentTimerEventCount;
  GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][this->IndexInFrame].StartCycleCount =
    __rdtsc();
  GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][this->IndexInFrame].EventDepth =
    g_CurrentTimerEventDepth;
  GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][this->IndexInFrame].NameTableIndex =
    this->NameTableIndex;
  ++g_CurrentTimerEventCount;
  ++g_CurrentTimerEventDepth;
}

timer_event_autoclose_wrapper::~timer_event_autoclose_wrapper()
{
  GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][this->IndexInFrame].EndCycleCount =
    __rdtsc();
  GLOBAL_TIMER_FRAME_SUMMARY_TABLE[g_CurrentProfilerFrameIndex][this->NameTableIndex].CycleCount +=
    GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][this->IndexInFrame].EndCycleCount -
    GLOBAL_FRAME_TIMER_EVENT_TABLE[g_CurrentProfilerFrameIndex][this->IndexInFrame].StartCycleCount;
  GLOBAL_TIMER_FRAME_SUMMARY_TABLE[g_CurrentProfilerFrameIndex][this->NameTableIndex].Calls++;
  --g_CurrentTimerEventDepth;
}

gpu_timer_event GPU_TIMER_EVENT_TABLE[PROFILE_MAX_FRAME_COUNT + 1][GPU_TIMER_EnumCount];
uint32_t        GPU_QUERY_OBJECT_TABLE[GPU_TIMER_EnumCount];
#endif // USE_DEBUG_PROFILING
