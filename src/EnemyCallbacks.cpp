#include "inttypes.hpp"

namespace th07
{

struct EnemyCombatTemplate
{
    u8 bytes[0xD4];
};

struct EnemyCallbackEntry
{
    i16 subId;
    i16 unknown;
};

struct EnemyCallbackOverlay
{
    u8 unknown0000[0x6E4];
    u8 eclContext[0x2398];
    i32 stackDepth;
    u8 unknown2A80[4];
    i32 deathCallbackSub;
    u8 unknown2A88[0x120];
    f32 bulletRankSpeedLow;
    f32 bulletRankSpeedHigh;
    i16 bulletRankAmount1Low;
    i16 bulletRankAmount1High;
    i16 bulletRankAmount2Low;
    i16 bulletRankAmount2High;
    i32 life;
    u8 unknown2BBC[0x18];
    EnemyCombatTemplate combatTemplate;
    i32 unknown2CA8;
    u8 unknown2CAC[0x17C];
    u8 unknownSlotFlags : 7;
    u8 isSlotOccupied : 1;
    u8 isInteractable : 1;
    u8 unknownFlags1 : 5;
    u8 isBoss : 1;
    u8 unknownFlags2 : 1;
    u8 unknown2E2A[0x92];
    i32 lifeCallbackThresholds[4];
    EnemyCallbackEntry lifeCallbacks[4];
    i32 timerCallbackThreshold;
    i32 unknown2EE0;
    i32 timerCallbackSub;
    u8 unknown2EE8[0x2060];

    i32 HandleLifeCallback();
};

typedef char EnemyCallbackOverlaySizeMustBe4F48[
    sizeof(EnemyCallbackOverlay) == 0x4F48 ? 1 : -1];

struct EclManagerCallbackOverlay
{
    void CallEclSub(void *context, i16 subId);
};

extern EclManagerCallbackOverlay g_EclManagerCallbacks;
extern EnemyCallbackOverlay g_EnemyCallbackPool[480];
extern EnemyCombatTemplate g_EnemyCombatTemplate;

#pragma var_order(enemy, callbackIndex, clearIndex, this)
i32 EnemyCallbackOverlay::HandleLifeCallback()
{
    EnemyCallbackOverlay *enemy;
    i32 callbackIndex;
    i32 clearIndex;

    enemy = g_EnemyCallbackPool;
    for (callbackIndex = 0; callbackIndex < 4; ++callbackIndex)
    {
        if (lifeCallbackThresholds[callbackIndex] < 0)
            continue;
        if (life < lifeCallbackThresholds[callbackIndex])
        {
            life = lifeCallbackThresholds[callbackIndex];
            g_EclManagerCallbacks.CallEclSub(eclContext,
                                             lifeCallbacks[callbackIndex].subId);
            lifeCallbackThresholds[callbackIndex] = -1;
            timerCallbackThreshold = -1;
            timerCallbackSub = -1;
            bulletRankSpeedLow = -0.5f;
            bulletRankSpeedHigh = 0.5f;
            bulletRankAmount1Low = 0;
            bulletRankAmount1High = 0;
            bulletRankAmount2Low = 0;
            bulletRankAmount2High = 0;
            stackDepth = 0;
            combatTemplate = g_EnemyCombatTemplate;
            unknown2CA8 = 0;

            for (clearIndex = 0; clearIndex < 480; ++clearIndex, ++enemy)
            {
                if (!enemy->isSlotOccupied)
                    continue;
                if (enemy->isBoss)
                    continue;

                enemy->life = 0;
                if (!enemy->isInteractable && enemy->deathCallbackSub >= 0)
                {
                    g_EclManagerCallbacks.CallEclSub(enemy->eclContext,
                                                     (i16)enemy->deathCallbackSub);
                    enemy->deathCallbackSub = -1;
                }
            }
            return 1;
        }
    }
    return 0;
}

} // namespace th07
