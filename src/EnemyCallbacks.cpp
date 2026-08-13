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

struct EnemyCallbackTimer
{
    i32 previous;
    i32 subFrameBits;
    i32 current;
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
    u8 unknown2BBC[8];
    EnemyCallbackTimer bossTimer;
    u8 unknown2BD0[4];
    EnemyCombatTemplate combatTemplate;
    i32 unknown2CA8;
    u8 unknown2CAC[0x16B];
    u8 bossSlot;
    u8 unknown2E18[0x10];
    u8 unknownSlotFlags : 7;
    u8 isSlotOccupied : 1;
    u8 isInteractable : 1;
    u8 unknownFlags1 : 5;
    u8 isBoss : 1;
    u8 unknownFlags2 : 1;
    u8 unknown2E2AFlags : 6;
    u8 isTimeoutSpell : 1;
    u8 unknown2E2AFlag7 : 1;
    u8 unknown2E2B[0x91];
    i32 lifeCallbackThresholds[4];
    EnemyCallbackEntry lifeCallbacks[4];
    i32 timerCallbackThreshold;
    i32 timerCallbackSub;
    i32 unknown2EE4;
    u8 unknown2EE8[0x2060];

    i32 HandleLifeCallback();
    i32 HandleTimerCallback();
};

typedef char EnemyCallbackOverlaySizeMustBe4F48[
    sizeof(EnemyCallbackOverlay) == 0x4F48 ? 1 : -1];

struct EclManagerCallbackOverlay
{
    void CallEclSub(void *context, i16 subId);
};

struct BulletManagerCallbackOverlay
{
    void ClearBullets(i32 mode);
};

struct GameManagerCallbackOverlay
{
    u8 unknown000[0x88];
    i32 rankBase;
};

extern EclManagerCallbackOverlay g_EclManagerCallbacks;
extern EnemyCallbackOverlay g_EnemyCallbackPool[480];
extern EnemyCombatTemplate g_EnemyCombatTemplate;
extern BulletManagerCallbackOverlay g_BulletManagerCallbacks;
extern GameManagerCallbackOverlay *g_GameManagerCallbacks;
extern i32 g_SpellcardSeconds;
extern i32 g_SpellCaptureEligible;
extern i32 g_SpellActive;
extern i32 g_SpellBaseScore;
extern i32 g_RankValue;
extern const f32 g_EnemyRankFactor;

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
            unknown2EE4 = -1;
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

#pragma optimize("p", off)
#pragma var_order(callbackIndex, maxThreshold, selectedIndex, rankDecrease, enemy, clearIndex, bossTimerCurrent, spellSeconds, timer, this)
i32 EnemyCallbackOverlay::HandleTimerCallback()
{
    i32 callbackIndex;
    i32 maxThreshold;
    i32 selectedIndex;
    i32 rankDecrease;
    EnemyCallbackOverlay *enemy;
    i32 clearIndex;
    i32 bossTimerCurrent;
    i32 spellSeconds;
    EnemyCallbackTimer *timer;

    if (isBoss && bossSlot == 0)
    {
        bossTimerCurrent = bossTimer.current;
        spellSeconds = (timerCallbackThreshold - bossTimerCurrent) / 60;
        g_SpellcardSeconds = spellSeconds;
    }

    if ((bossTimer.current >= timerCallbackThreshold) ? 1 : 0)
    {
        maxThreshold = 0;
        for (callbackIndex = 0; callbackIndex < 4; ++callbackIndex)
        {
            if (lifeCallbackThresholds[callbackIndex] < 0)
                continue;
            if (maxThreshold < lifeCallbackThresholds[callbackIndex])
            {
                maxThreshold = lifeCallbackThresholds[callbackIndex];
                selectedIndex = callbackIndex;
            }
        }

        if (maxThreshold > 0)
        {
            life = lifeCallbackThresholds[selectedIndex];
            lifeCallbackThresholds[selectedIndex] = -1;
        }

        g_EclManagerCallbacks.CallEclSub(eclContext, (i16)timerCallbackSub);
        timerCallbackThreshold = -1;
        timerCallbackSub = deathCallbackSub;
        timer = &bossTimer;
        timer->current = 0;
        timer->subFrameBits = 0;
        timer->previous = -999;

        if (!isTimeoutSpell)
        {
            g_SpellBaseScore = 0;
            g_SpellCaptureEligible = 0;
            if (g_SpellActive)
                ++g_SpellActive;
            g_BulletManagerCallbacks.ClearBullets(10);

            rankDecrease = (g_RankValue - g_GameManagerCallbacks->rankBase) * g_EnemyRankFactor;
            rankDecrease -= rankDecrease % 10;
            g_RankValue -= rankDecrease;
        }

        enemy = g_EnemyCallbackPool;
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

        unknown2EE4 = -1;
        combatTemplate = g_EnemyCombatTemplate;
        unknown2CA8 = 0;
        bulletRankSpeedLow = -0.5f;
        bulletRankSpeedHigh = 0.5f;
        bulletRankAmount1Low = 0;
        bulletRankAmount1High = 0;
        bulletRankAmount2Low = 0;
        bulletRankAmount2High = 0;
        stackDepth = 0;
        return 1;
    }
    return 0;
}
#pragma optimize("p", on)

} // namespace th07
