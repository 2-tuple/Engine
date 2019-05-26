#ifdef OFFBEAT_DO_TESTS

#include <float.h>
#include <stdio.h>
#include <string.h>

#define CLEAR    "\x1B[0m"
#define BLACK    "\x1B[30m"
#define RED      "\x1B[31m"
#define GREEN    "\x1B[32m"
#define YELLOW   "\x1B[33m"
#define BLUE     "\x1B[34m"
#define MAGENTA  "\x1B[35m"
#define CYAN     "\x1B[36m"
#define WHITE    "\x1B[37m"
#define BBLACK   "\x1B[90m"
#define BRED     "\x1B[91m"
#define BGREEN   "\x1B[92m"
#define BYELLOW  "\x1B[93m"
#define BBLUE    "\x1B[94m"
#define BMAGENTA "\x1B[95m"
#define BCYAN    "\x1B[96m"
#define BWHITE   "\x1B[97m"

struct test_string
{
    char S[300];
};

static u32 GTotalTestCount;
static u32 GTotalPassedCount;
static u32 GCurrentTestCount;
static u32 GPassedTestCount;

static u32 GFailedTestCount;
static test_string GFailedTests[100];

#define OffbeatTest(FunctionPointer, ExpectedResult, ...) OffbeatTest_(#FunctionPointer, #__VA_ARGS__, ExpectedResult, FunctionPointer(##__VA_ARGS__))
void
OffbeatTest_(char* FunctionName, char* Arguments, f32 ExpectedResult, f32 ActualResult)
{
    f32 Diff = ExpectedResult - ActualResult;
    if(Diff < 0) { Diff = -Diff; }
    if(Diff < 0.000001f)
    {
        ++GPassedTestCount;
        ++GTotalPassedCount;
        ++GCurrentTestCount;
        ++GTotalTestCount;
    }
    else
    {
        ++GCurrentTestCount;
        ++GTotalTestCount;
        sprintf(GFailedTests[GFailedTestCount++].S, "\tTest %uC|%uT: %s(%s):\n\t\tResult   = %.10f.\n\t\tExpected = %.10f.\n\t\tDiff     = %.10f.\n", GCurrentTestCount, GTotalTestCount, FunctionName, Arguments, ActualResult, ExpectedResult, Diff);
    }
}

void
OffbeatTest_(char* FunctionName, char* Arguments, ov3 ExpectedResult, ov3 ActualResult)
{
    ov3 Diff = ExpectedResult - ActualResult;
    if(Diff.x < 0) { Diff.x = -Diff.x; }
    if(Diff.y < 0) { Diff.y = -Diff.y; }
    if(Diff.z < 0) { Diff.z = -Diff.z; }
    if((Diff.x < 0.000001f) &&
       (Diff.y < 0.000001f) &&
       (Diff.z < 0.000001f))
    {
        ++GPassedTestCount;
        ++GTotalPassedCount;
        ++GCurrentTestCount;
        ++GTotalTestCount;
    }
    else
    {
        ++GCurrentTestCount;
        ++GTotalTestCount;
        sprintf(GFailedTests[GFailedTestCount++].S, "\tTest %uC|%uT: %s(%s):\n\t\tResult   = ov3{%.10f, %.10f, %.10f}.\n\t\tExpected = ov3{%.10f, %.10f, %.10f}.\n\t\tDiff     = ov3{%.10f, %.10f, %.10f}.\n", GCurrentTestCount, GTotalTestCount, FunctionName, Arguments, ActualResult.x, ActualResult.y, ActualResult.z, ExpectedResult.x, ExpectedResult.y, ExpectedResult.z, Diff.x, Diff.y, Diff.z);
    }
}

void
OffbeatTest_(char* FunctionName, char* Arguments, ov4 ExpectedResult, ov4 ActualResult)
{
    ov4 Diff = ExpectedResult - ActualResult;
    if(Diff.x < 0) { Diff.x = -Diff.x; }
    if(Diff.y < 0) { Diff.y = -Diff.y; }
    if(Diff.z < 0) { Diff.z = -Diff.z; }
    if(Diff.w < 0) { Diff.w = -Diff.w; }
    if((Diff.x < 0.000001f) &&
       (Diff.y < 0.000001f) &&
       (Diff.z < 0.000001f) &&
       (Diff.w < 0.000001f))
    {
        ++GPassedTestCount;
        ++GTotalPassedCount;
        ++GCurrentTestCount;
        ++GTotalTestCount;
    }
    else
    {
        ++GCurrentTestCount;
        ++GTotalTestCount;
        sprintf(GFailedTests[GFailedTestCount++].S, "\tTest %uC|%uT: %s(%s):\n\t\tResult   = ov4{%.10f, %.10f, %.10f, %.10f}.\n\t\tExpected = ov4{%.10f, %.10f, %.10f, %.10f}.\n\t\tDiff     = ov4{%.10f, %.10f, %.10f, %.10f}.\n", GCurrentTestCount, GTotalTestCount, FunctionName, Arguments, ActualResult.x, ActualResult.y, ActualResult.z, ActualResult.w, ExpectedResult.x, ExpectedResult.y, ExpectedResult.z, ExpectedResult.w, Diff.x, Diff.y, Diff.z, Diff.w);
    }
}

#define OffbeatPrintResults(FunctionPointer) OffbeatPrintResults_(#FunctionPointer)
void
OffbeatPrintResults_(char* FunctionName)
{
    char Dots[] = "..................................................";
    s32 MaxLength = 40;
    s32 FunctionNameLength = strlen(FunctionName);
    s32 Diff = MaxLength - FunctionNameLength;
    if(Diff < 0)
    {
        Diff = 0;
    }
    Dots[Diff] = 0;

    printf("\t%s%s%s%u/%u%s passed.\n", FunctionName, Dots,
                                        (GPassedTestCount == GCurrentTestCount) ? GREEN : RED,
                                        GPassedTestCount, GCurrentTestCount,
                                        CLEAR);

    GPassedTestCount = GCurrentTestCount = 0;
}

void
OffbeatPrintFailedTests()
{
    printf("\n%svvvvvvvvvvFAILED TESTSvvvvvvvvvv%s\n", RED, CLEAR);
    for(u32 i = 0; i < GFailedTestCount; ++i)
    {
        printf("%s", GFailedTests[i].S);
    }
    printf("%s^^^^^^^^^^FAILED TESTS^^^^^^^^^^%s\n", RED, CLEAR);
}

static void
OffbeatRunTests()
{
    printf("\n%s==========UNIT TESTING==========%s\n", BCYAN, CLEAR);

    OffbeatTest(ObAbsoluteValue, 1.0f, -1.0f);
    OffbeatTest(ObAbsoluteValue, 2.0f, 2.0f);
    OffbeatTest(ObAbsoluteValue, 0.0f, 0.0f);
    OffbeatPrintResults(ObAbsoluteValue);

    OffbeatTest(ObSquare, 100.0f, -10.0f);
    OffbeatTest(ObSquare, 4.0f, 2.0f);
    OffbeatTest(ObSquare, 0.0f, 0.0f);
    OffbeatPrintResults(ObSquare);

    OffbeatTest(ObSquareRoot, 10.0f, 100.0f);
    OffbeatTest(ObSquareRoot, 2.0f, 4.0f);
    OffbeatTest(ObSquareRoot, 0.0f, 0.0f);
    OffbeatPrintResults(ObSquareRoot);

    OffbeatTest(ObSin, 1.0f, 0.5f * PI);
    OffbeatTest(ObSin, -1.0f, -0.5f * PI);
    OffbeatTest(ObSin, 0.0f, 0.0f);
    OffbeatPrintResults(ObSin);

    OffbeatTest(ObCos, 1.0f, 0.0f);
    OffbeatTest(ObCos, -1.0f, -1.0f * PI);
    OffbeatTest(ObCos, 0.0f, 0.5f * PI);
    OffbeatPrintResults(ObCos);

    // F32
    OffbeatTest(ObLerp, 1.0f, 0.0f, 1.0f, 1.0f);
    OffbeatTest(ObLerp, -1.0f, -2.0f, 0.5f, 0.0f);
    OffbeatTest(ObLerp, 0.0f, -10.0f, 0.5f, 10.0f);
    OffbeatTest(ObLerp, 2.0f, 0.0f, 0.2f, 10.0f);
    OffbeatTest(ObLerp, 2.0f, 0.0f, 0.25f, 8.0f);
    OffbeatTest(ObLerp, 2.0f, 10.0f, 0.8f, 0.0f);
    // OV3
    OffbeatTest(ObLerp, (ov3{0.8f, 1.6f, 0.4f}), ov3{0.0f, 0.0f, 0.0f}, 0.8f, ov3{1.0f, 2.0f, 0.5f});
    OffbeatTest(ObLerp, (ov3{10.0f, 0.0f, -5.0f}), ov3{0.0f, -5.0f, 0.0f}, 0.5f, ov3{20.0f, 5.0f, -10.0f});
    OffbeatTest(ObLerp, (ov3{20.0f, -3.0f, -15.0f}), ov3{0.0f, -5.0f, 0.0f}, 1.0f, ov3{20.0f, -3.0f, -15.0f});
    // OV4
    OffbeatTest(ObLerp, (ov4{0.8f, 1.6f, 0.4f, -2.0f}), ov4{0.0f, 0.0f, 0.0f, -1.0f}, 0.8f, ov4{1.0f, 2.0f, 0.5f, -2.25f});
    OffbeatTest(ObLerp, (ov4{10.0f, 0.0f, -5.0f, 0.0f}), ov4{0.0f, -5.0f, 0.0f, 100.0f}, 0.5f, ov4{20.0f, 5.0f, -10.0f, -100.0f});
    OffbeatTest(ObLerp, (ov4{20.0f, -3.0f, -15.0f, 44.0f}), ov4{0.0f, -5.0f, 0.0f, 44.0f}, 1.0f, ov4{20.0f, -3.0f, -15.0f, 44.0f});
    OffbeatPrintResults(ObLerp);

    OffbeatTest(ObClamp, 10.0f, 0.0f, 100.0f, 10.0f);
    OffbeatTest(ObClamp, 10.0f, 10.0f, 5.0f, 20.0f);
    OffbeatTest(ObClamp, -1.0f, -5.0f, 0.0f, -1.0f);
    OffbeatTest(ObClamp, -1.0f, -1.0f, -5.0f, 0.0f);
    OffbeatTest(ObClamp, 200.0f, 100.0f, 200.0f, 300.0f);
    OffbeatPrintResults(ObClamp);

    // F32
    OffbeatTest(ObClamp01, 1.0f, 10.0f);
    OffbeatTest(ObClamp01, 0.0f, -10.0f);
    OffbeatTest(ObClamp01, 0.3f, 0.3f);
    OffbeatTest(ObClamp01, 0.7f, 0.7f);
    // OV3
    OffbeatTest(ObClamp01, (ov3{1.0f, 0.0f, 0.0f}), ov3{10.0f, 0.0f, -5.0f});
    OffbeatTest(ObClamp01, (ov3{0.0f, 1.0f, 1.0f}), ov3{-10.0f, 55.0f, 1.0001f});
    OffbeatTest(ObClamp01, (ov3{0.3f, 0.0f, 1.0f}), ov3{0.3f, -100.0f, 5.0f});
    OffbeatTest(ObClamp01, (ov3{0.5f, 0.8f, 0.11f}), ov3{0.5f, 0.8f, 0.11f});
    // OV4
    OffbeatTest(ObClamp01, (ov4{1.0f, 0.0f, 0.0f, 0.0f}), ov4{10.0f, 0.0f, -5.0f, -1.0f});
    OffbeatTest(ObClamp01, (ov4{0.0f, 1.0f, 1.0f, 0.25f}), ov4{-10.0f, 55.0f, 1.0001f, 0.25f});
    OffbeatTest(ObClamp01, (ov4{0.3f, 0.0f, 1.0f, 1.0f}), ov4{0.3f, -100.0f, 5.0f, 80.0f});
    OffbeatTest(ObClamp01, (ov4{0.5f, 0.8f, 0.11f, 0.7f}), ov4{0.5f, 0.8f, 0.11f, 0.7f});
    OffbeatPrintResults(ObClamp01);

    OffbeatTest(ObClamp01MapToRange, 1.0f, 0.0f, 100.0f, 10.0f);
    OffbeatTest(ObClamp01MapToRange, 0.0f, 10.0f, -5.0f, 20.0f);
    OffbeatTest(ObClamp01MapToRange, 1.0f, -5.0f, 1.0f, -1.0f);
    OffbeatTest(ObClamp01MapToRange, 0.0f, -1.0f, -5.0f, 0.0f);
    OffbeatTest(ObClamp01MapToRange, 0.5f, 100.0f, 200.0f, 300.0f);
    OffbeatPrintResults(ObClamp01MapToRange);

    OffbeatTest(ObFloor, 0.0f, 0.3f);
    OffbeatTest(ObFloor, 0.0f, 0.7f);
    OffbeatTest(ObFloor, 1.0f, 1.99f);
    OffbeatTest(ObFloor, -1.0f, -1.99f);
    OffbeatTest(ObFloor, 10.0f, 10.001f);
    OffbeatTest(ObFloor, 22.0f, 22.222f);
    OffbeatPrintResults(ObFloor);

    OffbeatTest(ObRound, 0.0f, 0.3f);
    OffbeatTest(ObRound, 1.0f, 0.7f);
    OffbeatTest(ObRound, 2.0f, 1.99f);
    OffbeatTest(ObRound, -1.0f, -1.99f);
    OffbeatTest(ObRound, 10.0f, 10.001f);
    OffbeatTest(ObRound, 22.0f, 22.222f);
    OffbeatPrintResults(ObRound);

    OffbeatTest(ObCeil, 1.0f, 0.3f);
    OffbeatTest(ObCeil, 1.0f, 0.7f);
    OffbeatTest(ObCeil, 2.0f, 1.99f);
    OffbeatTest(ObCeil, 0.0f, -1.99f);
    OffbeatTest(ObCeil, 11.0f, 10.001f);
    OffbeatTest(ObCeil, 23.0f, 22.222f);
    OffbeatPrintResults(ObCeil);

    // OV3
    OffbeatTest(ObInner, 1.25f, ov3{1.0f, 0.5f, 0.0f}, ov3{1.0f, 0.5f, 0.0f});
    OffbeatTest(ObInner, -11.25f, ov3{3.0f, 0.0f, 1.5f}, ov3{-3.0f, 0.0f, -1.5f});
    OffbeatTest(ObInner, 0.0f, ov3{1.0f, 1.0f, 0.0f}, ov3{-1.0f, 1.0f, 0.0f});
    OffbeatTest(ObInner, -5.0f, ov3{2.0f, 1.0f, 0.0f}, ov3{-3.0f, 1.0f, 2.0f});
    // OV4
    OffbeatTest(ObInner, 1.25f, ov4{1.0f, 0.5f, 0.0f, 0.0f}, ov4{1.0f, 0.5f, 0.0f, 0.0f});
    OffbeatTest(ObInner, -11.25f, ov4{3.0f, 0.0f, 1.5f, 0.0f}, ov4{-3.0f, 0.0f, -1.5f, 0.0f});
    OffbeatTest(ObInner, 0.0f, ov4{1.0f, 1.0f, 0.0f, 0.0f}, ov4{-1.0f, 1.0f, 0.0f, 0.0f});
    OffbeatTest(ObInner, -5.0f, ov4{2.0f, 1.0f, 0.0f, 0.0f}, ov4{-3.0f, 1.0f, 2.0f, 0.0f});
    OffbeatTest(ObInner, 1.25f, ov4{1.0f, 0.5f, 0.0f, 1.0f}, ov4{1.0f, 0.5f, 0.0f, 0.0f});
    OffbeatTest(ObInner, -11.25f, ov4{3.0f, 0.0f, 1.5f, 0.0f}, ov4{-3.0f, 0.0f, -1.5f, 1.0f});
    OffbeatTest(ObInner, 0.0f, ov4{1.0f, 1.0f, 0.0f, 0.0f}, ov4{-1.0f, 1.0f, 0.0f, 0.0f});
    OffbeatTest(ObInner, -6.0f, ov4{2.0f, 1.0f, 0.0f, -1.0f}, ov4{-3.0f, 1.0f, 2.0f, 1.0f});
    OffbeatPrintResults(ObInner);

    // OV3
    OffbeatTest(ObLength, 1.0f, ov3{1.0f, 0.0f, 0.0f});
    OffbeatTest(ObLength, 25.0f, ov3{0.0f, 25.0f, 0.0f});
    OffbeatTest(ObLength, 3.354101f, ov3{-3.0f, 0.0f, -1.5f});
    OffbeatTest(ObLength, 1.414213f, ov3{1.0f, -1.0f, 0.0f});
    OffbeatTest(ObLength, 2.236068f, ov3{2.0f, 1.0f, 0.0f});
    // OV4
    OffbeatTest(ObLength, 1.0f, ov4{1.0f, 0.0f, 0.0f, 0.0f});
    OffbeatTest(ObLength, 25.0f, ov4{0.0f, 25.0f, 0.0f, 0.0f});
    OffbeatTest(ObLength, 6.020797f, ov4{-3.0f, 0.0f, -1.5f, 5.0f});
    OffbeatTest(ObLength, 1.732050f, ov4{1.0f, -1.0f, 0.0f, -1.0f});
    OffbeatTest(ObLength, 3.741657f, ov4{2.0f, 1.0f, 0.0f, 3.0f});
    OffbeatPrintResults(ObLength);

    // OV3
    OffbeatTest(ObNormalize, (ov3{1.0f, 0.0f, 0.0f}), ov3{100.0f, 0.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov3{0.0f, 1.0f, 0.0f}), ov3{0.0f, 25.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov3{0.0f, 0.0f, 1.0f}), ov3{0.0f, 0.0f, 0.5f});
    OffbeatTest(ObNormalize, (ov3{-1.0f, 0.0f, 0.0f}), ov3{-0.1f, 0.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov3{0.0f, -1.0f, 0.0f}), ov3{0.0f, -50.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov3{0.0f, 0.0f, -1.0f}), ov3{0.0f, 0.0f, -1.0f});
    // OV4
    OffbeatTest(ObNormalize, (ov4{1.0f, 0.0f, 0.0f, 0.0f}), ov4{100.0f, 0.0f, 0.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov4{0.0f, 1.0f, 0.0f, 0.0f}), ov4{0.0f, 25.0f, 0.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov4{0.0f, 0.0f, 1.0f, 0.0f}), ov4{0.0f, 0.0f, 0.5f, 0.0f});
    OffbeatTest(ObNormalize, (ov4{0.0f, 0.0f, 0.0f, 1.0f}), ov4{0.0f, 0.0f, 0.0f, 0.001f});
    OffbeatTest(ObNormalize, (ov4{-1.0f, 0.0f, 0.0f, 0.0f}), ov4{-0.1f, 0.0f, 0.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov4{0.0f, -1.0f, 0.0f, 0.0f}), ov4{0.0f, -50.0f, 0.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov4{0.0f, 0.0f, -1.0f, 0.0f}), ov4{0.0f, 0.0f, -1.0f, 0.0f});
    OffbeatTest(ObNormalize, (ov4{0.0f, 0.0f, 0.0f, -1.0f}), ov4{0.0f, 0.0f, 0.0f, -0.01f});
    OffbeatPrintResults(ObNormalize);

    // OV3
    OffbeatTest(ObNOZ, (ov3{1.0f, 0.0f, 0.0f}), ov3{100.0f, 0.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov3{0.0f, 1.0f, 0.0f}), ov3{0.0f, 25.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov3{0.0f, 0.0f, 1.0f}), ov3{0.0f, 0.0f, 0.5f});
    OffbeatTest(ObNOZ, (ov3{-1.0f, 0.0f, 0.0f}), ov3{-0.1f, 0.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov3{0.0f, -1.0f, 0.0f}), ov3{0.0f, -50.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov3{0.0f, 0.0f, -1.0f}), ov3{0.0f, 0.0f, -1.0f});
    OffbeatTest(ObNOZ, (ov3{0.0f, 0.0f, 0.0f}), ov3{0.0f, 0.0f, -0.0001f});
    OffbeatTest(ObNOZ, (ov3{0.0f, 0.0f, 0.0f}), ov3{0.0f, 0.0f, -0.0f});
    OffbeatTest(ObNOZ, (ov3{0.0f, 1.0f, 0.0f}), ov3{0.0f, 0.01f, 0.0f});
    // OV4
    OffbeatTest(ObNOZ, (ov4{1.0f, 0.0f, 0.0f, 0.0f}), ov4{100.0f, 0.0f, 0.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov4{0.0f, 1.0f, 0.0f, 0.0f}), ov4{0.0f, 25.0f, 0.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov4{0.0f, 0.0f, 1.0f, 0.0f}), ov4{0.0f, 0.0f, 0.5f, 0.0f});
    OffbeatTest(ObNOZ, (ov4{0.0f, 0.0f, 0.0f, 1.0f}), ov4{0.0f, 0.0f, 0.0f, 0.001f});
    OffbeatTest(ObNOZ, (ov4{-1.0f, 0.0f, 0.0f, 0.0f}), ov4{-0.1f, 0.0f, 0.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov4{0.0f, -1.0f, 0.0f, 0.0f}), ov4{0.0f, -50.0f, 0.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov4{0.0f, 0.0f, -1.0f, 0.0f}), ov4{0.0f, 0.0f, -1.0f, 0.0f});
    OffbeatTest(ObNOZ, (ov4{0.0f, 0.0f, 0.0f, -1.0f}), ov4{0.0f, 0.0f, 0.0f, -0.01f});
    OffbeatTest(ObNOZ, (ov4{0.0f, 0.0f, 0.0f, 0.0f}), ov4{0.0f, 0.0f, -0.0001f, 0.0f});
    OffbeatTest(ObNOZ, (ov4{0.0f, 0.0f, 0.0f, 0.0f}), ov4{0.0f, 0.0f, -0.0f, 0.0f});
    OffbeatPrintResults(ObNOZ);

    OffbeatTest(ObCross, (ov3{0.0f, 0.0f, 1.0f}), ov3{1.0f, 0.0f, 0.0f}, ov3{0.0f, 1.0f, 0.0f});
    OffbeatTest(ObCross, (ov3{1.0f, 0.0f, 0.0f}), ov3{0.0f, 1.0f, 0.0f}, ov3{0.0f, 0.0f, 1.0f});
    OffbeatTest(ObCross, (ov3{0.0f, 1.0f, 0.0f}), ov3{0.0f, 0.0f, 1.0f}, ov3{1.0f, 0.0f, 0.0f});
    OffbeatTest(ObCross, (ov3{0.0f, 0.0f, 1.0f}), ov3{-1.0f, 0.0f, 0.0f}, ov3{0.0f, -1.0f, 0.0f});
    OffbeatTest(ObCross, (ov3{1.0f, 0.0f, 0.0f}), ov3{0.0f, -1.0f, 0.0f}, ov3{0.0f, 0.0f, -1.0f});
    OffbeatTest(ObCross, (ov3{0.0f, 1.0f, 0.0f}), ov3{0.0f, 0.0f, -1.0f}, ov3{-1.0f, 0.0f, 0.0f});
    OffbeatTest(ObCross, (ov3{0.0f, 0.0f, -1.0f}), ov3{-1.0f, 0.0f, 0.0f}, ov3{0.0f, 1.0f, 0.0f});
    OffbeatTest(ObCross, (ov3{-1.0f, 0.0f, 0.0f}), ov3{0.0f, -1.0f, 0.0f}, ov3{0.0f, 0.0f, 1.0f});
    OffbeatTest(ObCross, (ov3{0.0f, -1.0f, 0.0f}), ov3{0.0f, 0.0f, -1.0f}, ov3{1.0f, 0.0f, 0.0f});
    OffbeatPrintResults(ObCross);

    ov3 Start, Destination;
    Start       = ObNormalize(ov3{1.0f, 0.0f, 0.0f});
    Destination = ObNormalize(ov3{0.0f, 1.0f, 1.0f});
    OffbeatTest(ObTransform, (Destination), ObRotationAlign(Start, Destination), Start);
    Start       = ObNormalize(ov3{1.0f, 0.0f, 1.0f});
    Destination = ObNormalize(ov3{0.0f, -1.0f, 1.0f});
    OffbeatTest(ObTransform, (Destination), ObRotationAlign(Start, Destination), Start);
    Start       = ObNormalize(ov3{1.0f, 33.0f, 0.0f});
    Destination = ObNormalize(ov3{0.0f, -24.0f, 1.0f});
    OffbeatTest(ObTransform, (Destination), ObRotationAlign(Start, Destination), Start);
    Start       = ObNormalize(ov3{0.0f, 0.0f, 4.33f});
    Destination = ObNormalize(ov3{-7.0f, 1.0f, 1.0f});
    OffbeatTest(ObTransform, (Destination), ObRotationAlign(Start, Destination), Start);
    Start       = ov3{0.0f, 0.0f, 0.0f};
    Destination = ov3{0.0f, 1.0f, 1.0f};
    OffbeatTest(ObTransform, (Destination), ObRotationAlign(Start, Destination), Destination);
    Start       = ov3{0.0f, 0.0f, 0.0f};
    Destination = ov3{0.0f, 1.0f, 1.0f};
    OffbeatTest(ObTransform, (Start), ObRotationAlign(Start, Destination), Start);
    OffbeatPrintResults(ObTransform);

    // OM3
    OffbeatTest(ObDeterminant, 1.0f, ObIdentity3());
    OffbeatTest(ObDeterminant, 8.0f, ObIdentity3(2.0f));
    OffbeatTest(ObDeterminant, -8.0f, ObIdentity3(-2.0f));
    OffbeatTest(ObDeterminant, 27.0f, ObIdentity3(3.0f));
    OffbeatTest(ObDeterminant, -27.0f, ObIdentity3(-3.0f));
    // OM4
    OffbeatTest(ObDeterminant, 1.0f, ObIdentity4());
    OffbeatTest(ObDeterminant, 16.0f, ObIdentity4(2.0f));
    OffbeatTest(ObDeterminant, 16.0f, ObIdentity4(-2.0f));
    OffbeatTest(ObDeterminant, 81.0f, ObIdentity4(3.0f));
    OffbeatTest(ObDeterminant, 81.0f, ObIdentity4(-3.0f));
    OffbeatPrintResults(ObDeterminant);

    ob_expr Expression;
    ob_particle Particle;
    // 1
    Expression = {};
    Expression.Function = OFFBEAT_FunctionConst;
    Expression.Parameter = OFFBEAT_ParameterAge;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.01f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    OffbeatTest(OffbeatEvaluateExpression, (ov4{0.01f, 0.3f, 0.7f, 0.5f}), &Expression, &Particle);
    // 2
    Expression = {};
    Expression.Function = OFFBEAT_FunctionLerp;
    Expression.Parameter = OFFBEAT_ParameterAge;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.0f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.Age = 0.5f;
    OffbeatTest(OffbeatEvaluateExpression, (ov4{2.5f, 1.65f, 0.55f, 0.75f}), &Expression, &Particle);
    // 3
    Expression = {};
    Expression.Function = OFFBEAT_FunctionMaxLerp;
    Expression.Parameter = OFFBEAT_ParameterVelocity;
    Expression.Float = 10.0f;
    Expression.Low = ov4{10.0f, 0.0f, 5.0f, 0.0f};
    Expression.High = ov4{0.0f, 100.0f, -5.0f, 50.0f};
    Particle = {};
    Particle.dP = ov3{7.0f, 0.0f, 0.0f};
    OffbeatTest(OffbeatEvaluateExpression, (ov4{3.0f, 70.0f, -2.0f, 35.0f}), &Expression, &Particle);
    // 4
    Expression = {};
    Expression.Function = OFFBEAT_FunctionTriangle;
    Expression.Parameter = OFFBEAT_ParameterAge;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.0f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.Age = 0.25f;
    OffbeatTest(OffbeatEvaluateExpression, (ov4{2.5f, 1.65f, 0.55f, 0.75f}), &Expression, &Particle);
    // 5
    Expression = {};
    Expression.Function = OFFBEAT_FunctionTwoTriangles;
    Expression.Parameter = OFFBEAT_ParameterRandom;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.01f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.Random = 0.25f;
    OffbeatTest(OffbeatEvaluateExpression, (ov4{0.01f, 0.3f, 0.7f, 0.5f}), &Expression, &Particle);
    // 6
    Expression = {};
    Expression.Function = OFFBEAT_FunctionFourTriangles;
    Expression.Parameter = OFFBEAT_ParameterID;
    Expression.Float = 0.0f;
    Expression.Uint = 0;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.01f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.ID = 1.0f;
    OffbeatTest(OffbeatEvaluateExpression, (ov4{5.0f, 3.0f, 0.4f, 1.0f}), &Expression, &Particle);
    // 7
    Expression = {};
    Expression.Function = OFFBEAT_FunctionStep;
    Expression.Parameter = OFFBEAT_ParameterVelocity;
    Expression.Float = 10.0f;
    Expression.Uint = 5;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.0f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.dP = ov3{3.5f, 0.0f, 0.0f};
    OffbeatTest(OffbeatEvaluateExpression, (ov4{4.0f, 2.46f, 0.46f, 0.9f}), &Expression, &Particle);
    // 8
    Expression = {};
    Expression.Function = OFFBEAT_FunctionStep;
    Expression.Parameter = OFFBEAT_ParameterVelocity;
    Expression.Float = 10.0f;
    Expression.Uint = 5;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.0f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.dP = ov3{3.0f, 0.0f, 0.0f};
    OffbeatTest(OffbeatEvaluateExpression, (ov4{4.0f, 2.46f, 0.46f, 0.9f}), &Expression, &Particle);
    // 9
    Expression = {};
    Expression.Function = OFFBEAT_FunctionPeriodic;
    Expression.Parameter = OFFBEAT_ParameterAge;
    Expression.Float = 1.0f;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.0f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.Age = 0.5f;
    OffbeatTest(OffbeatEvaluateExpression, (ov4{2.5f, 1.65f, 0.55f, 0.75f}), &Expression, &Particle);
    // 10
    Expression = {};
    Expression.Function = OFFBEAT_FunctionPeriodic;
    Expression.Parameter = OFFBEAT_ParameterAge;
    Expression.Float = 1.0f;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.01f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.Age = 0.25f;
    OffbeatTest(OffbeatEvaluateExpression, (ov4{0.01f, 0.3f, 0.7f, 0.5f}), &Expression, &Particle);
    // 11
    Expression = {};
    Expression.Function = OFFBEAT_FunctionPeriodicSquare;
    Expression.Parameter = OFFBEAT_ParameterAge;
    Expression.Float = 2.0f;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.01f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.Age = 1.0f;
    OffbeatTest(OffbeatEvaluateExpression, (ov4{5.0f, 3.0f, 0.4f, 1.0f}), &Expression, &Particle);
    // 12
    Expression = {};
    Expression.Function = OFFBEAT_FunctionPeriodicSquare;
    Expression.Parameter = OFFBEAT_ParameterRandom;
    Expression.Float = 0.0f;
    Expression.Uint = 0;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.01f, 0.3f, 0.7f, 0.5f};
    Particle = {};
    Particle.Random = 0.6223f;
    OffbeatTest(OffbeatEvaluateExpression, (ov4{5.0f, 3.0f, 0.4f, 1.0f}), &Expression, &Particle);
    // 13
    Expression = {};
    Expression.Function = OFFBEAT_FunctionLerp;
    Expression.Parameter = OFFBEAT_ParameterRandom;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.01f, 0.3f, 0.7f, 0.5f};
    OffbeatTest(OffbeatEvaluateExpression, (ov4{5.0f, 3.0f, 0.4f, 1.0f}), &Expression);
    // 13
    Expression = {};
    Expression.Function = OFFBEAT_FunctionPeriodic;
    Expression.Parameter = OFFBEAT_ParameterAge;
    Expression.Low = ov4{5.0f, 3.0f, 0.4f, 1.0f};
    Expression.High = ov4{0.0f, 0.3f, 0.7f, 0.5f};
    OffbeatTest(OffbeatEvaluateExpression, (ov4{2.5f, 1.65f, 0.55f, 0.75f}), &Expression);
    OffbeatPrintResults(OffbeatEvaluateExpression);

    printf("\nTOTAL: %s%u/%u%s passed.\n", (GTotalPassedCount == GTotalTestCount) ? GREEN : RED,
                                           GTotalPassedCount, GTotalTestCount,
                                           CLEAR);
    if(GFailedTestCount)
    {
        OffbeatPrintFailedTests();
    }
    printf("%s==========UNIT TESTING==========%s\n\n", BCYAN, CLEAR);
}

#endif
