#pragma once

#include "stack_alloc.h"
#include "resource_manager.h"
#include "basic_data_structures.h"

#define MM_POINT_COUNT 3
#define MM_COMPARISON_BONE_COUNT 2
#define MM_ANIM_CAPACITY 35

struct mm_fixed_params
{
  fixed_stack<int32_t, MM_COMPARISON_BONE_COUNT> ComparisonBoneIndices;
};

struct mm_dynamic_params
{
  float TrajectoryTimeHorizon;
  float Responsiveness;
  float BelndInTime;
  float MinTimeOffsetThreshold;
};

struct mm_matching_params
{
  fixed_stack<rid, MM_ANIM_CAPACITY> AnimRIDs;

  mm_fixed_params   FixedParams;
  mm_dynamic_params DynamicParams;
};

struct mm_frame_info
{
  vec3 TrajectoryPs[MM_POINT_COUNT];
  // float Directions[MM_POINT_COUNT];
  vec3 BonePs[MM_COMPARISON_BONE_COUNT];
  vec3 BoneVs[MM_COMPARISON_BONE_COUNT];
};

struct int32_range
{
  int32_t Start;
  int32_t End;
};

struct mm_controller_data
{
  mm_matching_params Params;

  fixed_stack<int32_range, MM_ANIM_CAPACITY> AnimFrameRanges;
  array_handle<mm_frame_info>                FrameInfos;
};

mm_frame_info      GetCurrentFrameGoal(const Anim::animation_controller* Controller, vec3 Velocity,
                                       mm_matching_params Params);
mm_controller_data PrecomputeRuntimeMMData(Memory::stack_allocator*    TempAlloc,
                                           Resource::resource_manager* Resources,
                                           mm_matching_params          Params,
                                           const Anim::skeleton*       Skeleton);
float              MotionMatch(int32_t* OutAnimIndex, int32_t* OutStartFrameIndex,
                               const mm_controller_data* MMData, mm_frame_info Goal);
