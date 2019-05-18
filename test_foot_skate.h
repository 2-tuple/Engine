#pragma once

#include "anim.h"
#include "blend_stack.h"
#include "stack_alloc.h"
#include "motion_matching.h"

struct foot_skate_data_row
{
  float t;
  float dt;

  float LeftFootHeight;
  float RightFootHeight;

  float LeftFootXVel;
  float LeftFootYVel;
  float LeftFootZVel;
  float RightFootXVel;
  float RightFootYVel;
  float RightFootZVel;

  int32_t AnimIsMirrored;
  float   LocalAnimTime;
  int32_t AnimCount;
  int32_t AnimIndex;
};

#define MAX_TEST_BONE_COUNT 5

struct foot_skate_test
{
  fixed_stack<int32_t, MAX_TEST_BONE_COUNT> TestBoneIndices;

  rid   AnimationRID;
  float TopMargin;
  float BottomMargin;
  float ElapsedTime;
  float RemainingTime;
};

foot_skate_data_row MeasureFootSkate(foot_skate_test* Test, Anim::animation_controller* AnimPlayer,
                                     const mm_controller_data*         MMController,
                                     const Anim::skeleton_mirror_info* MirrorInfo,
                                     const blend_stack* BlendStack, const mat4& EntityModelMatrix,
                                     transform LocalDeltaRoot, float t, float dt);
foot_skate_data_row MeasureFootSkate(Memory::stack_allocator* TempAlloc, foot_skate_test* Test,
                                     const Anim::skeleton* Skeleton, const Anim::animation* Anim,
                                     float t, float dt);
