#include "inttypes.hpp"

#include <math.h>
#include <string.h>
#include <d3dx8math.h>

#pragma intrinsic(atan2)

namespace th07
{

/*
 * Private, target-pinned overlays for EnemyManager::OnUpdate.  These are kept
 * out of the shared headers until the complete Enemy/ANM layouts are proven.
 * Every named offset below is observed in TH07 1.00b at 0x00420620; TH06 only
 * corroborates the high-level phase ordering.
 */

struct EnemyManagerUpdateVec3
{
    f32 x;
    f32 y;
    f32 z;

    __forceinline EnemyManagerUpdateVec3 operator-(const EnemyManagerUpdateVec3 &other) const
    {
        EnemyManagerUpdateVec3 result;
        result.x = x - other.x;
        result.y = y - other.y;
        result.z = z - other.z;
        return result;
    }
};


struct EnemyManagerUpdateSprite
{
    u8 unknown00[0x2C];
    f32 height;
    f32 width;
};

struct EnemyManagerUpdateAnmVm
{
    u8 unknown000[0x1B8];
    u32 color;
    u32 unknown1BC;
    u32 flags;
    u8 unknown1C4[0x14];
    i16 scriptIndex;
    u8 unknown1DA[0xA];
    EnemyManagerUpdateSprite *sprite;
    u8 unknown1E8[0x64];
};
typedef char EnemyManagerUpdateAnmVm_size[(sizeof(EnemyManagerUpdateAnmVm) == 0x24C) ? 1 : -1];

struct EnemyManagerUpdateTimer
{
    i32 previous;
    union
    {
        i32 subFrameBits;
        f32 subFrame;
    };
    i32 current;

    i32 HasTicked()
    {
        return (current != previous) ? 1 : 0;
    }

    void Decrement(i32 amount);
};

struct EnemyManagerUpdateSlotFlagBits
{
    u8 unknown : 7;
    u8 active : 1;
};

struct EnemyManagerUpdateCombatFlagBits
{
    u8 interactable : 1;
    u8 collisionEnabled : 1;
    u8 damageable : 1;
    u8 noSprite : 1;
    u8 damageCollision : 1;
    u8 unknown5 : 1;
    u8 boss : 1;
    u8 unknown7 : 1;
};

struct EnemyManagerUpdateDeathFlagBits
{
    u8 mode : 3;
    u8 hasBeenInBounds : 1;
    u8 unknown4 : 1;
    u8 unknown5 : 1;
    u8 unknown6 : 1;
    u8 allowOffscreen : 1;
};

struct EnemyManagerUpdateControlFlagBits
{
    u8 skipMovement : 1;
    u8 skipFollow : 1;
    u8 skipCombat : 1;
    u8 pauseTimer : 1;
    u8 unknown4 : 1;
    u8 unknown5 : 1;
    u8 unknown6 : 1;
    u8 unknown7 : 1;
};

struct EnemyManagerUpdateDamageModeBits
{
    u32 rankFloor : 1;
    u32 unknown : 31;
};

struct EnemyManagerUpdateFlagOverlay
{
    u8 unknown0000[0x2E28];
    EnemyManagerUpdateSlotFlagBits slot;
    EnemyManagerUpdateCombatFlagBits combat;
    EnemyManagerUpdateDeathFlagBits death;
    EnemyManagerUpdateControlFlagBits control;
};

struct EnemyManagerUpdateEnemy
{
    u8 raw[0x4F48];

    EnemyManagerUpdateAnmVm *PrimaryVm()
    {
        return reinterpret_cast<EnemyManagerUpdateAnmVm *>(raw);
    }

    EnemyManagerUpdateAnmVm *ExtraVm(i32 index)
    {
        return reinterpret_cast<EnemyManagerUpdateAnmVm *>(raw + 0x24C + 0x24C * index);
    }

    EnemyManagerUpdateVec3 *Position()
    {
        return reinterpret_cast<EnemyManagerUpdateVec3 *>(raw + 0x2B0C);
    }

    EnemyManagerUpdateVec3 *Hitbox()
    {
        return reinterpret_cast<EnemyManagerUpdateVec3 *>(raw + 0x2B3C);
    }

    EnemyManagerUpdateVec3 *SecondaryHitbox()
    {
        return reinterpret_cast<EnemyManagerUpdateVec3 *>(raw + 0x2B48);
    }

    i32 &Life() { return *reinterpret_cast<i32 *>(raw + 0x2BB8); }
    i32 &MaxLife() { return *reinterpret_cast<i32 *>(raw + 0x2BBC); }
    i32 &Score() { return *reinterpret_cast<i32 *>(raw + 0x2BC0); }
    u32 &DamageModeFlags() { return *reinterpret_cast<u32 *>(raw + 0x2BCC); }
    EnemyManagerUpdateDamageModeBits *DamageModeBits()
    {
        return reinterpret_cast<EnemyManagerUpdateDamageModeBits *>(raw + 0x2BCC);
    }
    EnemyManagerUpdateTimer *BossTimer() { return reinterpret_cast<EnemyManagerUpdateTimer *>(raw + 0x2BC4); }
    u32 &DisplayColor() { return *reinterpret_cast<u32 *>(raw + 0x2BD0); }
    i32 &DeathCallbackSub() { return *reinterpret_cast<i32 *>(raw + 0x2A84); }
    i32 &ItemDrop() { return *reinterpret_cast<i32 *>(raw + 0x2E10); }
    i8 &DeathAnm1() { return *reinterpret_cast<i8 *>(raw + 0x2E14); }
    u8 &DeathAnm2() { return *reinterpret_cast<u8 *>(raw + 0x2E15); }
    u8 &BossSlot() { return *reinterpret_cast<u8 *>(raw + 0x2E17); }
    u8 &DamageFlashTimer() { return *reinterpret_cast<u8 *>(raw + 0x2E18); }
    u8 &SlotFlags() { return *reinterpret_cast<u8 *>(raw + 0x2E28); }
    i32 IsActive() { return reinterpret_cast<EnemyManagerUpdateFlagOverlay *>(this)->slot.active; }
    void ClearActive() { reinterpret_cast<EnemyManagerUpdateFlagOverlay *>(this)->slot.active = 0; }
    void SetNoSprite() { reinterpret_cast<EnemyManagerUpdateFlagOverlay *>(this)->combat.noSprite = 1; }
    void SetHasBeenInBounds()
    {
        reinterpret_cast<EnemyManagerUpdateFlagOverlay *>(this)->death.hasBeenInBounds = 1;
    }
    u8 &CombatFlags() { return *reinterpret_cast<u8 *>(raw + 0x2E29); }
    u8 &DeathFlags() { return *reinterpret_cast<u8 *>(raw + 0x2E2A); }
    u8 &UpdateFlags() { return *reinterpret_cast<u8 *>(raw + 0x2E2B); }
    EnemyManagerUpdateCombatFlagBits *CombatBits()
    {
        return &reinterpret_cast<EnemyManagerUpdateFlagOverlay *>(this)->combat;
    }
    EnemyManagerUpdateDeathFlagBits *DeathBits()
    {
        return &reinterpret_cast<EnemyManagerUpdateFlagOverlay *>(this)->death;
    }
    EnemyManagerUpdateControlFlagBits *ControlBits()
    {
        return &reinterpret_cast<EnemyManagerUpdateFlagOverlay *>(this)->control;
    }
    u8 &DrawGroup() { return *reinterpret_cast<u8 *>(raw + 0x2E2F); }
    i32 &LastDamage() { return *reinterpret_cast<i32 *>(raw + 0x2E4C); }
    void *&FollowTarget() { return *reinterpret_cast<void **>(raw + 0x2EB0); }
    i32 *LifeCallbacks() { return reinterpret_cast<i32 *>(raw + 0x2EBC); }
    i32 &TimerCallbackThreshold() { return *reinterpret_cast<i32 *>(raw + 0x2EDC); }
    i32 &TimerCallbackSub() { return *reinterpret_cast<i32 *>(raw + 0x2EE4); }
    u8 &TrailFlags() { return *reinterpret_cast<u8 *>(raw + 0x4F30); }
    i16 &TrailHistoryCount() { return *reinterpret_cast<i16 *>(raw + 0x4F32); }
    i16 &TrailSampleCount() { return *reinterpret_cast<i16 *>(raw + 0x4F34); }
    i32 &FreezeTimer() { return *reinterpret_cast<i32 *>(raw + 0x4F40); }
    EnemyManagerUpdateEnemy *&DrawNext()
    {
        return *reinterpret_cast<EnemyManagerUpdateEnemy **>(raw + 0x4F44);
    }

    void Move();
    void ClampPosition();
    void Despawn();
    i32 HandleLifeCallback();
    i32 HandleTimerCallback();
    void UpdateEffects();
    void ReleaseEffects();
    void CheckPlayerCollision(EnemyManagerUpdateVec3 *center, EnemyManagerUpdateVec3 *size);
};
typedef char EnemyManagerUpdateEnemy_size[(sizeof(EnemyManagerUpdateEnemy) == 0x4F48) ? 1 : -1];

struct EnemyManagerUpdateTimelineLane
{
    EnemyManagerUpdateTimer timer;
    void *instruction;
    void Run();
};

struct EnemyManagerUpdateEclFile
{
    i16 unknown00;
    i16 timelineCount;
    void *timeline[1];
};

struct EnemyManagerUpdateEclManager
{
    EnemyManagerUpdateEclFile *file;

    i32 RunEcl(EnemyManagerUpdateEnemy *enemy);
    void CallEclSub(void *context, i16 subId);
};

struct EnemyManagerUpdateAnmManager
{
    i32 ExecuteScript(EnemyManagerUpdateAnmVm *vm);
};

struct EnemyManagerUpdatePlayer
{
    i32 CalcDamageToEnemy(EnemyManagerUpdateVec3 *position, EnemyManagerUpdateVec3 *hitbox, i32 *bombHit);
};

struct EnemyManagerUpdateGameManager
{
    u8 unknown000[4];
    i32 score;
    u8 unknown008[0x54];
    f32 livesRemaining;

    __forceinline void AddScore(i32 points)
    {
        score += points / 10;
    }
};

struct EnemyManagerUpdateGrazeState
{
    void IncreaseSubrank(i32 amount);
    void ApplyRank(i32 amount);
    i32 PeriodicCheck();
    i32 IsInBounds(f32 x, f32 y, f32 width, f32 height);
};

struct EnemyManagerUpdateGui
{
    i32 IsMessageActive();
    void ShowBonus(i32 score);
};

struct EnemyManagerUpdateRewardManager
{
    i32 ConvertBulletBonus(i32 base, i32 bullets);
};

struct EnemyManagerUpdateTimerManager
{
    void Advance(i32 *current, i32 *subFrameBits);
};

struct EnemyManagerUpdateEffectManager
{
    void SpawnParticles(i32 animation, EnemyManagerUpdateVec3 *position, i32 count, i32 color);
};

struct EnemyManagerUpdateItemManager
{
    void SpawnItem(EnemyManagerUpdateVec3 *position, i32 item, i32 bombHit);
};

struct EnemyManagerUpdateBulletManager
{
    i32 DespawnBullets(i32 maxScore, i32 awardScore);
};

struct EnemyManagerUpdateSoundPlayer
{
    void PlaySoundByIdx(i32 sound, i32 unused);
};

extern EnemyManagerUpdateEclManager g_EnemyManagerUpdateEclManager;
extern EnemyManagerUpdateAnmManager *g_EnemyManagerUpdateAnmManager;
extern EnemyManagerUpdatePlayer g_EnemyManagerUpdatePlayer;
extern EnemyManagerUpdateGameManager *g_EnemyManagerUpdateGameManager;
extern EnemyManagerUpdateGrazeState g_EnemyManagerUpdateGrazeState;
extern EnemyManagerUpdateGui g_EnemyManagerUpdateGui;
extern EnemyManagerUpdateRewardManager g_EnemyManagerUpdateRewardManager;
extern EnemyManagerUpdateTimerManager g_EnemyManagerUpdateTimerManager;
extern EnemyManagerUpdateEffectManager g_EnemyManagerUpdateEffectManager;
extern EnemyManagerUpdateItemManager g_EnemyManagerUpdateItemManager;
extern EnemyManagerUpdateBulletManager g_EnemyManagerUpdateBulletManager;
extern EnemyManagerUpdateSoundPlayer g_EnemyManagerUpdateSoundPlayer;
extern "C" i32 __cdecl EnemyManagerUpdateFtol();

extern i32 g_EnemyManagerUpdateDifficulty;
extern i32 g_EnemyManagerUpdateAccumulator;
extern i32 g_EnemyManagerUpdateFrameStop;
extern i8 g_EnemyManagerUpdatePauseByte;
extern i8 g_EnemyManagerUpdateSpecialDamage;
extern u8 g_EnemyManagerUpdatePracticeFlag;
extern u8 g_EnemyManagerUpdateStageState;
extern u8 g_EnemyManagerUpdateBossPresent;
extern f32 g_EnemyManagerUpdateBossHealth;
extern f32 g_EnemyManagerUpdateReal2_5;
extern i32 g_EnemyManagerUpdateSpellActive;
extern i32 g_EnemyManagerUpdateSpellState;
extern u8 *g_EnemyManagerUpdatePlayerFlags;
extern f32 g_EnemyManagerUpdateLastHitX;
extern f32 g_EnemyManagerUpdateLastHitY;
extern f32 g_EnemyManagerUpdateLastHitZ;
extern f32 g_EnemyManagerUpdateLastHit2X;
extern f32 g_EnemyManagerUpdateLastHit2Y;
extern f32 g_EnemyManagerUpdateLastHit2Z;
extern f32 g_EnemyManagerUpdateReferenceX;
extern f32 g_EnemyManagerUpdateReferenceY;
extern f32 g_EnemyManagerUpdateReferenceZ;
extern u8 g_EnemyManagerUpdateShotType;
extern const f32 g_EnemyManagerUpdateOne;
extern const f32 g_EnemyManagerUpdateSixteen;
extern u8 g_EnemyManagerUpdateBossUi[4][0x24C];
extern i32 g_EnemyManagerUpdateBossUiFlags[4];
extern u8 g_EnemyManagerUpdateRandomItemTable[32];
extern u8 g_EnemyManagerUpdateCombatTemplate[0xD4];

struct EnemyManagerUpdateOverlay
{
    u8 raw[1];

    EnemyManagerUpdateEnemy *Enemies()
    {
        return reinterpret_cast<EnemyManagerUpdateEnemy *>(raw + 0x4F50);
    }

    i32 &EnemyCount() { return *reinterpret_cast<i32 *>(raw + 0x9545BC); }
    i16 &RandomSpawnIndex() { return *reinterpret_cast<i16 *>(raw + 0x9545B8); }
    i16 &RandomTableIndex() { return *reinterpret_cast<i16 *>(raw + 0x9545BA); }
    i32 &SpellActive() { return *reinterpret_cast<i32 *>(raw + 0x9545C8); }
    i32 &SpellUsedBomb() { return *reinterpret_cast<i32 *>(raw + 0x9545DC); }
    EnemyManagerUpdateTimelineLane *TimelineLanes()
    {
        return reinterpret_cast<EnemyManagerUpdateTimelineLane *>(raw + 0x9545F4);
    }
    EnemyManagerUpdateTimer *Timer()
    {
        return reinterpret_cast<EnemyManagerUpdateTimer *>(raw + 0x9546F4);
    }
    EnemyManagerUpdateEnemy **DrawHeads()
    {
        return reinterpret_cast<EnemyManagerUpdateEnemy **>(raw + 0x954700);
    }

    i32 OnUpdate();
};

static __forceinline void AdvanceEnemyTimer(EnemyManagerUpdateTimer *timer)
{
    timer->previous = timer->current;
    g_EnemyManagerUpdateTimerManager.Advance(&timer->current, &timer->subFrameBits);
}

static __forceinline void PushTrailSample(EnemyManagerUpdateEnemy *enemy)
{
    i32 index;

    if (enemy->TrailFlags())
    {
        for (index = enemy->TrailHistoryCount() - 1; index > 0; --index)
        {
            *reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2F78 + 0x1C * index) =
                *reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2F78 + 0x1C * (index - 1));
            *reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2F84 + 0x1C * index) =
                *reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2F84 + 0x1C * (index - 1));

            *reinterpret_cast<u32 *>(enemy->raw + 0x2F90 + 0x1C * index) =
                *reinterpret_cast<u32 *>(enemy->raw + 0x2F90 + 0x1C * (index - 1));
        }

        *reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2F78) = *enemy->Position();
        *reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2F84) =
            *reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2B18);
        *reinterpret_cast<u32 *>(enemy->raw + 0x2F90) = *reinterpret_cast<u32 *>(enemy->raw + 0x2B54);
    }
}

static __forceinline void InterpolateFollowTarget(EnemyManagerUpdateEnemy *enemy)
{
    D3DXVECTOR3 *targetPosition;
    D3DXVECTOR3 *enemyPosition;

    if (enemy->FollowTarget() && !enemy->ControlBits()->skipFollow)
    {
        targetPosition = reinterpret_cast<D3DXVECTOR3 *>(
            reinterpret_cast<u8 *>(enemy->FollowTarget()) + 0x24C);
        enemyPosition = reinterpret_cast<D3DXVECTOR3 *>(enemy->Position());
        D3DXVECTOR3 delta = *enemyPosition - *targetPosition;
        D3DXVECTOR3 step = delta / 16.0f;
        D3DXVECTOR3 *resultTarget = reinterpret_cast<D3DXVECTOR3 *>(
            reinterpret_cast<u8 *>(enemy->FollowTarget()) + 0x24C);
        D3DXVECTOR3 result = *resultTarget + step;
        *reinterpret_cast<D3DXVECTOR3 *>(reinterpret_cast<u8 *>(enemy->FollowTarget()) + 0x24C) = result;
    }
}

static __forceinline i32 EnemyIsInBounds(EnemyManagerUpdateEnemy *enemy)
{
    return g_EnemyManagerUpdateGrazeState.IsInBounds(
        enemy->Position()->x, enemy->Position()->y,
        enemy->PrimaryVm()->sprite->width, enemy->PrimaryVm()->sprite->height);
}

static __forceinline i32 EnemyTrailIsInBounds(EnemyManagerUpdateEnemy *enemy)
{
    return g_EnemyManagerUpdateGrazeState.IsInBounds(
        reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2F78 +
            0x1C * (enemy->TrailHistoryCount() - 1))->x,
        reinterpret_cast<EnemyManagerUpdateVec3 *>(enemy->raw + 0x2F78 +
            0x1C * (enemy->TrailHistoryCount() - 1))->y,
        enemy->PrimaryVm()->sprite->width, enemy->PrimaryVm()->sprite->height);
}

static __forceinline void ResetEnemyCombatState(EnemyManagerUpdateEnemy *enemy)
{
    i32 i;
    u32 *destination;
    u32 *source;

    *reinterpret_cast<f32 *>(enemy->raw + 0x2BA8) = -0.5f;
    *reinterpret_cast<f32 *>(enemy->raw + 0x2BAC) = 0.5f;
    *reinterpret_cast<i16 *>(enemy->raw + 0x2BB0) = 0;
    *reinterpret_cast<i16 *>(enemy->raw + 0x2BB2) = 0;
    *reinterpret_cast<i16 *>(enemy->raw + 0x2BB4) = 0;
    *reinterpret_cast<i16 *>(enemy->raw + 0x2BB6) = 0;
    *reinterpret_cast<i32 *>(enemy->raw + 0x2A7C) = 0;
    for (i = 0; i < 4; ++i)
        enemy->LifeCallbacks()[i] = -1;
    enemy->TimerCallbackThreshold() = -1;
    enemy->TimerCallbackSub() = -1;

    destination = reinterpret_cast<u32 *>(enemy->raw + 0x2BD4);
    source = reinterpret_cast<u32 *>(g_EnemyManagerUpdateCombatTemplate);
    memcpy(destination, source, 0xD4);
    *reinterpret_cast<i32 *>(enemy->raw + 0x2CA8) = 0;
}

static __forceinline void StorePrimaryHit(EnemyManagerUpdateVec3 *position)
{
    g_EnemyManagerUpdateLastHitX = position->x;
    g_EnemyManagerUpdateLastHitY = position->y;
    g_EnemyManagerUpdateLastHitZ = position->z;
}

static __forceinline void StoreSecondaryHit(EnemyManagerUpdateVec3 *position)
{
    g_EnemyManagerUpdateLastHit2X = position->x;
    g_EnemyManagerUpdateLastHit2Y = position->y;
    g_EnemyManagerUpdateLastHit2Z = position->z;
}

static __forceinline f32 EnemyAimAngle(EnemyManagerUpdateVec3 *position)
{
    return (f32)atan2(position->y - g_EnemyManagerUpdateReferenceY,
                      position->x - g_EnemyManagerUpdateReferenceX);
}

static __forceinline i32 IsSecondaryAimAngle(f32 angle)
{
    return angle >= -2.0943952f && angle <= -1.0471976f;
}

static __forceinline void TrackLastEnemyHit(EnemyManagerUpdateEnemy *enemy)
{
    EnemyManagerUpdateVec3 *position;
    EnemyManagerUpdateVec3 currentDelta;
    EnemyManagerUpdateVec3 previousDelta;
    f32 angle;

    if (enemy->CombatBits()->boss)
    {
        previousDelta = *reinterpret_cast<EnemyManagerUpdateVec3 *>(&g_EnemyManagerUpdateLastHitX) -
                        *reinterpret_cast<EnemyManagerUpdateVec3 *>(&g_EnemyManagerUpdateReferenceX);
        position = enemy->Position();
        currentDelta = *position -
                       *reinterpret_cast<EnemyManagerUpdateVec3 *>(&g_EnemyManagerUpdateReferenceX);
        if (!g_EnemyManagerUpdateSpellState || fabs(previousDelta.x) > fabs(currentDelta.x))
            StorePrimaryHit(position);

        if (g_EnemyManagerUpdateShotType == 2)
        {
            previousDelta = *reinterpret_cast<EnemyManagerUpdateVec3 *>(&g_EnemyManagerUpdateLastHit2X) -
                            *reinterpret_cast<EnemyManagerUpdateVec3 *>(&g_EnemyManagerUpdateReferenceX);
            angle = EnemyAimAngle(position);
            if (IsSecondaryAimAngle(angle) &&
                (!g_EnemyManagerUpdateSpellState || fabs(previousDelta.x) > fabs(currentDelta.x)))
            {
                StoreSecondaryHit(position);
                g_EnemyManagerUpdateSpellState = 1;
            }
        }
        else
        {
            g_EnemyManagerUpdateSpellState = 1;
        }
    }

    if (!g_EnemyManagerUpdateSpellState)
    {
        position = enemy->Position();
        if (g_EnemyManagerUpdateLastHitY < position->y)
            StorePrimaryHit(position);
        if (g_EnemyManagerUpdateShotType == 2 && g_EnemyManagerUpdateLastHit2Y < -900.0f)
        {
            angle = EnemyAimAngle(position);
            if (IsSecondaryAimAngle(angle))
                StoreSecondaryHit(position);
        }
    }
}

#pragma var_order(bombHit, difficultyScale, enemyIndex, damage, extraDamage, vmIndex, trailIndex, oldLife, enemy)
i32 EnemyManagerUpdateOverlay::OnUpdate()
{
    EnemyManagerUpdateEnemy *enemy;
    D3DXVECTOR3 secondaryHitbox;
    i32 damageOccurred;
    i32 trailIndex;
    i32 vmIndex;
    i32 extraDamage;
    i32 damage;
    i32 enemyIndex;
    i32 difficultyScale;
    i32 bombHit;
    i32 timerCurrent;
    void *timelineInstruction;

    bombHit = 0;
    difficultyScale = (g_EnemyManagerUpdateDifficulty >= 5) ?
        10 : g_EnemyManagerUpdateDifficulty * 2;

    if (!g_EnemyManagerUpdateGui.IsMessageActive())
    {
        i32 interval = 2400;
        interval -= (i32)g_EnemyManagerUpdateGameManager->livesRemaining * 4 * 60;
        if (Timer()->HasTicked())
        {
            timerCurrent = Timer()->current;
            if (timerCurrent % interval == 0)
                g_EnemyManagerUpdateGrazeState.IncreaseSubrank(100);
        }
        ++g_EnemyManagerUpdateAccumulator;
    }

    for (enemyIndex = 0; enemyIndex < 4; ++enemyIndex)
        DrawHeads()[enemyIndex] = 0;

    for (enemyIndex = 0; enemyIndex < g_EnemyManagerUpdateEclManager.file->timelineCount; ++enemyIndex)
    {
        if (!*reinterpret_cast<void **>(raw + 0x954600 + 0x10 * enemyIndex))
        {
            timelineInstruction = g_EnemyManagerUpdateEclManager.file->timeline[enemyIndex];
            *reinterpret_cast<void **>(raw + 0x954600 + 0x10 * enemyIndex) = timelineInstruction;
        }
        reinterpret_cast<EnemyManagerUpdateTimelineLane *>(raw + 0x9545F4 + 0x10 * enemyIndex)->Run();
    }

    enemy = Enemies();
    EnemyCount() = 0;
    for (enemyIndex = 0; enemyIndex < 480; ++enemyIndex, ++enemy)
    {
        if (!enemy->IsActive())
            continue;

        *reinterpret_cast<i32 *>(raw + 0x9545BC) =
            *reinterpret_cast<i32 *>(raw + 0x9545BC) + 1;
        if (enemy->ControlBits()->pauseTimer &&
            (g_EnemyManagerUpdateFrameStop || g_EnemyManagerUpdatePauseByte))
        {
            enemy->BossTimer()->Decrement(1);
        }
        else
        {
        run_enemy_ecl:
            if (g_EnemyManagerUpdateEclManager.RunEcl(enemy) == -1)
            {
                enemy->ClearActive();
                enemy->Despawn();
                continue;
            }

            if (!enemy->ControlBits()->skipMovement)
            {
                enemy->ClampPosition();
                enemy->Move();
                enemy->ClampPosition();
            }

            InterpolateFollowTarget(enemy);
            PushTrailSample(enemy);

            if (!enemy->PrimaryVm()->sprite)
                enemy->SetNoSprite();
            if (!enemy->CombatBits()->noSprite && !enemy->DeathBits()->hasBeenInBounds &&
                EnemyIsInBounds(enemy))
                enemy->SetHasBeenInBounds();

            if (enemy->DeathBits()->hasBeenInBounds == 1)
            {
                if (!((enemy->TrailFlags() || EnemyIsInBounds(enemy)) &&
                      (!enemy->TrailFlags() || EnemyIsInBounds(enemy) ||
                       EnemyTrailIsInBounds(enemy))) &&
                    !enemy->DeathBits()->allowOffscreen)
                {
                    enemy->ClearActive();
                    enemy->Despawn();
                    continue;
                }
            }

            if (enemy->HandleLifeCallback())
                goto run_enemy_ecl;
            if (enemy->TimerCallbackThreshold() >= 0 && enemy->HandleTimerCallback())
                goto run_enemy_ecl;

            enemy->PrimaryVm()->color = enemy->DisplayColor();
            g_EnemyManagerUpdateAnmManager->ExecuteScript(enemy->PrimaryVm());
            enemy->DisplayColor() = enemy->PrimaryVm()->color;
            for (vmIndex = 0; vmIndex < 2; ++vmIndex)
            {
                if (enemy->ExtraVm(vmIndex)->scriptIndex >= 0 &&
                    g_EnemyManagerUpdateAnmManager->ExecuteScript(enemy->ExtraVm(vmIndex)))
                    enemy->ExtraVm(vmIndex)->scriptIndex = -1;
            }

            bombHit = 0;
            damageOccurred = 0;
            if (!enemy->CombatBits()->noSprite && !enemy->ControlBits()->skipCombat)
            {
                if (enemy->CombatBits()->interactable && enemy->CombatBits()->collisionEnabled)
                {
                    enemy->CheckPlayerCollision(enemy->Position(), enemy->Hitbox());
                    if (enemy->TrailFlags())
                    {
                        secondaryHitbox = *reinterpret_cast<D3DXVECTOR3 *>(enemy->Hitbox());
                        for (trailIndex = 1; trailIndex < enemy->TrailSampleCount(); trailIndex += 6)
                        {
                            if (enemy->TrailFlags() & 2)
                            {
                                secondaryHitbox =
                                    *reinterpret_cast<D3DXVECTOR3 *>(enemy->Hitbox()) -
                                    *reinterpret_cast<D3DXVECTOR3 *>(enemy->Hitbox()) *
                                    (f32)trailIndex / (f32)enemy->TrailSampleCount();
                            }
                            enemy->CheckPlayerCollision(
                                reinterpret_cast<EnemyManagerUpdateVec3 *>(
                                    enemy->raw + 0x2F78 + 0x1C * trailIndex),
                                reinterpret_cast<EnemyManagerUpdateVec3 *>(&secondaryHitbox));
                        }
                    }
                }

                enemy->LastDamage() = 0;
                if (enemy->CombatBits()->interactable && enemy->CombatBits()->damageCollision)
                {
                    damage = g_EnemyManagerUpdatePlayer.CalcDamageToEnemy(enemy->Position(), enemy->Hitbox(), &bombHit);
                    if (enemy->SecondaryHitbox()->x > 0.0f)
                    {
                        extraDamage = g_EnemyManagerUpdatePlayer.CalcDamageToEnemy(enemy->Position(), enemy->SecondaryHitbox(), &bombHit);
                        if (!bombHit)
                            damage = damage + extraDamage / 2.5f;
                    }

                    if (damage > 0)
                    {
                        if ((enemy->CombatBits()->boss || !g_EnemyManagerUpdateSpecialDamage) &&
                            !g_EnemyManagerUpdateFrameStop)
                        {
                            i32 rankAmount;
                            if (enemy->CombatBits()->boss && !g_EnemyManagerUpdateSpecialDamage)
                                rankAmount = 10 * (damage / (10 - difficultyScale / 3));
                            else
                                rankAmount = 10 * (damage / (30 - difficultyScale));
                            if (rankAmount > 70)
                                rankAmount = 70;
                            u32 rankFloorFlags;
                            if (!rankAmount &&
                                (!g_EnemyManagerUpdateSpecialDamage ||
                                 ((rankFloorFlags = enemy->DamageModeFlags()),
                                  (rankFloorFlags & 1))))
                                rankAmount = 10;
                            switch (g_EnemyManagerUpdatePracticeFlag)
                            {
                            case 0:
                                if (rankAmount == 20 || rankAmount == 30)
                                {
                                    u32 rankAdjustFlags = enemy->DamageModeFlags();
                                    if (rankAdjustFlags & 1)
                                        rankAmount -= 10;
                                }
                                if (g_EnemyManagerUpdateDifficulty >= 5 && g_EnemyManagerUpdateDifficulty <= 6 &&
                                    !enemy->CombatBits()->boss)
                                    damage /= 2;
                                if (g_EnemyManagerUpdateDifficulty == 4 && !enemy->CombatBits()->boss)
                                    damage -= damage / 16 + damage / 4;
                                break;
                            }
                            if (rankAmount)
                                g_EnemyManagerUpdateGrazeState.ApplyRank(rankAmount);
                        }

                        if (damage >= 70)
                            damage = 70;
                        __asm
                        {
                            mov ecx, g_EnemyManagerUpdateGameManager
                            mov eax, damage
                            cdq
                            mov esi, 5
                            idiv esi
                            imul eax, eax, 10
                            cdq
                            mov esi, 10
                            idiv esi
                            add eax, [ecx + 4]
                            mov edx, g_EnemyManagerUpdateGameManager
                            mov [edx + 4], eax
                        }
                        __asm
                        {
                            mov eax, enemy
                            mov cl, BYTE PTR[eax + 0x2e29]
                            shr cl, 2
                            and cl, 1
                            movzx edx, cl
                            test edx, edx
                            je damage_update_done
                            mov eax, this
                            cmp DWORD PTR[eax + 0x9545c8], 0
                            je damage_spell_done
                            cmp bombHit, 0
                            jne damage_bomb_hit
                            cmp damage, 7
                            jle damage_nonzero_one
                            mov eax, damage
                            cdq
                            mov ecx, 7
                            idiv ecx
                            mov damage, eax
                            jmp damage_spell_done
                        damage_nonzero_one:
                            cmp damage, 0
                            je damage_spell_done
                            mov damage, 1
                            jmp damage_spell_done
                        damage_bomb_hit:
                            mov edx, this
                            cmp DWORD PTR[edx + 0x9545dc], 0
                            je damage_zero
                            cmp damage, 2
                            jle damage_bomb_nonzero_one
                            fild damage
                            fdiv DWORD PTR[g_EnemyManagerUpdateReal2_5]
                            call EnemyManagerUpdateFtol
                            mov damage, eax
                            jmp damage_spell_done
                        damage_bomb_nonzero_one:
                            cmp damage, 0
                            je damage_spell_done
                            mov damage, 1
                            jmp damage_spell_done
                        damage_zero:
                            mov damage, 0
                        damage_spell_done:
                            mov eax, enemy
                            xor ecx, ecx
                            cmp DWORD PTR[eax + 0x4f40], 0
                            setg cl
                            test ecx, ecx
                            je damage_apply
                            mov edx, enemy
                            mov al, BYTE PTR[edx + 0x2e29]
                            shr al, 6
                            and al, 1
                            movzx ecx, al
                            test ecx, ecx
                            je damage_force_zero
                            mov eax, damage
                            cdq
                            mov ecx, 9
                            idiv ecx
                            mov damage, eax
                            jmp damage_apply
                        damage_force_zero:
                            mov damage, 0
                        damage_apply:
                            mov edx, enemy
                            mov eax, DWORD PTR[edx + 0x2ba4]
                            sub eax, damage
                            mov ecx, enemy
                            mov DWORD PTR[ecx + 0x2ba4], eax
                            mov edx, enemy
                            mov eax, damage
                            mov DWORD PTR[edx + 0x2bb8], eax
                        damage_update_done:
                        }
                        damageOccurred = 1;
                    }
                }

                TrackLastEnemyHit(enemy);

                if (enemy->Life() <= 0 && enemy->CombatBits()->interactable)
                {
                    for (vmIndex = 0; vmIndex < 4; ++vmIndex)
                        enemy->LifeCallbacks()[vmIndex] = -1;
                    enemy->TimerCallbackThreshold() = -1;
                    enemy->TimerCallbackSub() = -1;

                    switch (enemy->DeathFlags() & 7)
                    {
                    case 3:
                        enemy->Life() = 1;
                        enemy->CombatBits()->damageable = 0;
                        enemy->DeathFlags() &= 0xF8;
                        g_EnemyManagerUpdateBossPresent = 0;
                        *reinterpret_cast<u16 *>(g_EnemyManagerUpdatePlayerFlags + 0xD6) |= 0x20;
                        if (enemy->DeathAnm1() >= 0)
                        {
                            g_EnemyManagerUpdateEffectManager.SpawnParticles(enemy->DeathAnm1(), enemy->Position(), 1, -1);
                            g_EnemyManagerUpdateEffectManager.SpawnParticles(enemy->DeathAnm1(), enemy->Position(), 1, -1);
                            g_EnemyManagerUpdateEffectManager.SpawnParticles(enemy->DeathAnm1(), enemy->Position(), 1, -1);
                        }
                        break;
                    case 1:
                        g_EnemyManagerUpdateGameManager->score += enemy->Score() / 10;
                        enemy->CombatBits()->interactable = 0;
                    case 0:
                        if ((enemy->DeathFlags() & 7) == 0)
                        {
                            g_EnemyManagerUpdateGameManager->score += enemy->Score() / 10;
                            enemy->ClearActive();
                        }
                        if (enemy->CombatBits()->boss)
                        {
                            g_EnemyManagerUpdateBossPresent = 0;
                            enemy->ReleaseEffects();
                        }
                    case 2:
                        if (enemy->ItemDrop() >= 0)
                        {
                            g_EnemyManagerUpdateEffectManager.SpawnParticles(enemy->DeathAnm2() + 4, enemy->Position(), 3, -1);
                            g_EnemyManagerUpdateItemManager.SpawnItem(enemy->Position(), enemy->ItemDrop(), bombHit);
                        }
                        else if (enemy->ItemDrop() == -1)
                        {
                            if ((u16)RandomSpawnIndex() % 3 == 0)
                            {
                                g_EnemyManagerUpdateEffectManager.SpawnParticles(enemy->DeathAnm2() + 4, enemy->Position(), 6, -1);
                                g_EnemyManagerUpdateItemManager.SpawnItem(enemy->Position(),
                                    g_EnemyManagerUpdateRandomItemTable[(u16)RandomTableIndex()], bombHit);
                                if ((u16)++RandomTableIndex() >= 32)
                                    RandomTableIndex() = 0;
                            }
                            ++RandomSpawnIndex();
                        }
                        if (enemy->CombatBits()->boss && !g_EnemyManagerUpdateSpellActive)
                        {
                            i32 bullets = g_EnemyManagerUpdateBulletManager.DespawnBullets(8000, 1);
                            i32 score = g_EnemyManagerUpdateRewardManager.ConvertBulletBonus(8000, bullets);
                            if (score)
                            {
                                g_EnemyManagerUpdateGameManager->score += score / 10;
                                g_EnemyManagerUpdateGui.ShowBonus(score);
                            }
                        }
                        enemy->Life() = 0;
                        *reinterpret_cast<u16 *>(g_EnemyManagerUpdatePlayerFlags + 0xD6) |= 0x20;
                        break;
                    }

                    g_EnemyManagerUpdateSoundPlayer.PlaySoundByIdx(enemyIndex % 2 + 2, 0);
                    if (enemy->DeathAnm1() >= 0)
                    {
                        g_EnemyManagerUpdateEffectManager.SpawnParticles(enemy->DeathAnm1(), enemy->Position(), 1, -1);
                        g_EnemyManagerUpdateEffectManager.SpawnParticles(enemy->DeathAnm2() + 4, enemy->Position(), 4, -1);
                    }
                    if (enemy->DeathCallbackSub() >= 0)
                    {
                        ResetEnemyCombatState(enemy);
                        g_EnemyManagerUpdateEclManager.CallEclSub(enemy->raw + 0x6E4,
                            (i16)enemy->DeathCallbackSub());
                        enemy->DeathCallbackSub() = -1;
                    }
                }

            }
        }

        if (enemy->DamageFlashTimer())
        {
            --enemy->DamageFlashTimer();
            enemy->PrimaryVm()->flags &= ~0x10000;
        }
        else if (damageOccurred)
        {
            g_EnemyManagerUpdateSoundPlayer.PlaySoundByIdx(20, 0);
            enemy->PrimaryVm()->flags |= 0x10000;
            enemy->DamageFlashTimer() = 1;
        }
        else
        {
            enemy->PrimaryVm()->flags &= ~0x10000;
        }

        if (enemy->CombatBits()->boss)
        {
            EnemyManagerUpdateVec3 bossUiPosition;
            if (!g_EnemyManagerUpdateGui.IsMessageActive() && !enemy->BossSlot())
                g_EnemyManagerUpdateBossHealth = (f32)enemy->Life() / (f32)enemy->MaxLife();

            if (enemy->BossSlot() < 4)
            {
                bossUiPosition.x = enemy->CombatBits()->noSprite ? -999.0f : enemy->Position()->x + 32.0f;
                bossUiPosition.y = 472.0f;
                bossUiPosition.z = 0.0f;
                *reinterpret_cast<EnemyManagerUpdateVec3 *>(g_EnemyManagerUpdateBossUi[enemy->BossSlot()]) = bossUiPosition;
                g_EnemyManagerUpdateBossUiFlags[enemy->BossSlot()] = (enemy->PrimaryVm()->flags >> 16) & 1;
            }
        }

        enemy->UpdateEffects();
        if (!g_EnemyManagerUpdateStageState)
            AdvanceEnemyTimer(enemy->BossTimer());
        if (enemy->FreezeTimer() > 0)
            reinterpret_cast<EnemyManagerUpdateTimer *>(enemy->raw + 0x4F38)->Decrement(1);

        if (!enemy->CombatBits()->noSprite && enemy->IsActive())
        {
            enemy->DrawNext() = DrawHeads()[enemy->DrawGroup()];
            DrawHeads()[enemy->DrawGroup()] = enemy;
        }
    }

    if (Timer()->current % 200 == 0 && g_EnemyManagerUpdateGrazeState.PeriodicCheck())
        return 4;

    AdvanceEnemyTimer(Timer());
    return 1;
}

} // namespace th07
