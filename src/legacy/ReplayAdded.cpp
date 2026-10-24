#include "inttypes.hpp"

#include <stdlib.h>
#include <string.h>

void *__cdecl operator new(unsigned int size);

namespace th07 {

extern u8 g_ReplayAddedCharacter;
extern i32 g_ReplayAddedRng0;
extern i32 g_ReplayAddedRng1;
extern i32 g_ReplayAddedConfig;
extern void *g_ReplayAddedGameManager;
extern i32 g_ReplayAddedStage;
extern void *g_ReplayAddedGameState;
extern i32 g_ReplayAddedRank;
extern u16 g_ReplayAddedPointItems;
extern u8 g_ReplayAddedPowerItems;
extern i32 g_ReplayAddedScore0;
extern i32 g_ReplayAddedScore1;
extern i32 g_ReplayAddedScore2;

namespace {

struct ReplayHeader {
    u8 beforeStages[0x1C];
    i32 primaryStageData[7];
    i32 auxiliaryStageData[7];
    u8 afterStages[0x94];
};

struct ReplayGameSnapshot {
    i32 words[14];
};

static __forceinline i32 &HeaderInt(ReplayHeader *header, i32 offset)
{
    return *(i32 *)((u8 *)header + offset);
}

static __forceinline u16 &HeaderWord(ReplayHeader *header, i32 offset)
{
    return *(u16 *)((u8 *)header + offset);
}

static __forceinline u8 &HeaderByte(ReplayHeader *header, i32 offset)
{
    return *(u8 *)((u8 *)header + offset);
}

static __forceinline i32 &StateInt(i32 offset)
{
    return *(i32 *)((u8 *)g_ReplayAddedGameState + offset);
}

static __forceinline f32 StateFloat(i32 offset)
{
    return *(f32 *)((u8 *)g_ReplayAddedGameState + offset);
}

} // namespace

struct ReplayAdded {
    i32 frameId;
    ReplayHeader *replayData;
    u8 unknown008[0x7A];
    u16 currentInputCount;
    i32 *currentInputCursor;
    u8 unknown088[0x1C];
    i32 *auxInputCursor;

    i32 AddedCallback();
};

#pragma var_order(primary, stage, auxiliary, previousStage, allocatedHeader, oldPrimary, oldAuxiliary, primaryBufferSize, auxiliaryBufferSize, this)
i32 ReplayAdded::AddedCallback()
{
    i32 *primary;
    i32 stage;
    i32 *auxiliary;
    i32 *previousStage;
    ReplayHeader *allocatedHeader;
    i32 *oldPrimary;
    i32 *oldAuxiliary;
    i32 primaryBufferSize;
    i32 auxiliaryBufferSize;

    frameId = 0;
    *(i32 *)((u8 *)this + 0x40) = 0;

    if (replayData == 0)
    {
        allocatedHeader = static_cast<ReplayHeader *>(::operator new(sizeof(ReplayHeader)));
        replayData = allocatedHeader;
        memcpy((u8 *)replayData + 0x00, "T7RP", 4);
        HeaderByte(replayData, 0x56) = g_ReplayAddedCharacter;
        HeaderWord(replayData, 0x04) = 0x1100;
        HeaderWord(replayData, 0x6A) = 0x0100;
        HeaderByte(replayData, 0x55) = 0x62;
        memcpy((u8 *)replayData + 0xE0, "0100b", 6);
        HeaderInt(replayData, 0xD8) = g_ReplayAddedRng0;
        HeaderInt(replayData, 0xDC) = g_ReplayAddedRng1;
        HeaderByte(replayData, 0x57) = (u8)g_ReplayAddedConfig;
        memcpy((u8 *)replayData + 0x5E, "NO NAME", 4);

        *reinterpret_cast<ReplayGameSnapshot *>((u8 *)replayData + 0x70) =
            *reinterpret_cast<const ReplayGameSnapshot *>(g_ReplayAddedGameManager);

        for (stage = 0; stage < 7; ++stage)
        {
            replayData->primaryStageData[stage] = 0;
            replayData->auxiliaryStageData[stage] = 0;
        }
    }
    else if (g_ReplayAddedStage - 2 >= 0)
    {
        previousStage = (i32 *)replayData->primaryStageData[g_ReplayAddedStage - 2];
        if (previousStage)
            previousStage[0] = StateInt(0x04);
    }

    stage = g_ReplayAddedStage - 1;
    if (stage >= 7)
        stage = 6;

    if (replayData->primaryStageData[stage])
    {
        oldPrimary = (i32 *)replayData->primaryStageData[stage];
        free(oldPrimary);
    }
    if (replayData->auxiliaryStageData[stage])
    {
        oldAuxiliary = (i32 *)replayData->auxiliaryStageData[stage];
        free(oldAuxiliary);
    }

    primaryBufferSize = 0x70800;
    replayData->primaryStageData[stage] = (i32)malloc(primaryBufferSize);
    auxiliaryBufferSize = 0x70800;
    replayData->auxiliaryStageData[stage] = (i32)malloc(auxiliaryBufferSize);

    primary = (i32 *)replayData->primaryStageData[stage];
    auxiliary = (i32 *)replayData->auxiliaryStageData[stage];
    primary[5] = StateInt(0x18);
    ((u8 *)primary)[0x24] = (u8)*(f32 *)((u8 *)g_ReplayAddedGameState + 0x68);
    ((u8 *)primary)[0x23] = (u8)*(f32 *)((u8 *)g_ReplayAddedGameState + 0x5C);
    ((u8 *)primary)[0x22] = (u8)*(f32 *)((u8 *)g_ReplayAddedGameState + 0x7C);
    ((u8 *)primary)[0x25] = (u8)g_ReplayAddedRank;
    primary[1] = StateInt(0x28);
    *(i16 *)((u8 *)primary + 0x20) = g_ReplayAddedPointItems;
    ((u8 *)primary)[0x26] = g_ReplayAddedPowerItems;
    primary[2] = g_ReplayAddedScore0 - StateInt(0x88);
    primary[3] = g_ReplayAddedScore1 - StateInt(0x88);
    primary[4] = g_ReplayAddedScore2 - StateInt(0x88);
    ((u8 *)primary)[0x27] = *(u8 *)((u8 *)g_ReplayAddedGameState + 0x1C);
    primary[6] = StateInt(0x2C);
    primary[7] = StateInt(0x30);

    currentInputCursor = (i32 *)((u8 *)primary + 0x2C);
    auxInputCursor = auxiliary;
    *(u16 *)currentInputCursor = 0;
    currentInputCount = 0;
    return 0;
}

} // namespace th07
