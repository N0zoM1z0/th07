#include "inttypes.hpp"

namespace th07
{

extern f32 __stdcall AddNormalizeAngle(f32 angle, f32 delta);
extern u8 g_TargetBossPresent49FC14;
extern i32 g_PlayerCollisionFlags;

struct EnemyUpdateVec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct EnemyUpdateEffect
{
    u8 unknown000[0x1C0];
    u32 unknownFlag0 : 1;
    u32 visible : 1;
    u32 unknownFlags : 30;
    u8 unknown1C4[0xC4];
    EnemyUpdateVec3 position;
    u8 unknown294[0x1C];
    f32 distance;
    f32 angle;
    u8 unknown2A4[0x16];
    u8 releaseRequested;
};

struct EnemyUpdateOverlay
{
    u8 unknown0000[0x2B0C];
    EnemyUpdateVec3 position;
    u8 unknown2B18[0x310];
    u8 unknownSlotFlags : 7;
    u8 isSlotOccupied : 1;
    u8 isInteractable : 1;
    u8 unknownFlagBits : 2;
    u8 hideEffects : 1;
    u8 unknownFlagBitsHigh : 2;
    u8 isBoss : 1;
    u8 shouldClampPosition : 1;
    u8 deathMode : 3;
    u8 unknownDeathModeBits : 5;
    u8 unknown2E2B[0x11];
    f32 clampMinX;
    f32 clampMinY;
    f32 clampMaxX;
    f32 clampMaxY;
    u8 unknown2E4C[4];
    EnemyUpdateEffect *effects[25];
    i32 effectCount;
    f32 effectDistance;

    void Despawn();
    void ReleaseEffects();
    void ClampPosition();
    void UpdateEffects();
};

struct EnemyUpdatePlayerCollisionState
{
    u8 unknown000[0xD6];
    u16 flags;
};

extern EnemyUpdateOverlay *g_TargetSpellBosses12FE098[8];

void EnemyUpdateOverlay::Despawn()
{
    if (deathMode == 0)
        isSlotOccupied = 0;
    else
        isInteractable = 0;

    if (isBoss) {
        if (unknown2B18[0x2FF] < 4)
            g_TargetBossPresent49FC14 = 0;
    }

    if (effectCount)
        ReleaseEffects();

    if (isBoss)
        g_TargetSpellBosses12FE098[unknown2B18[0x2FF]] = 0;

    reinterpret_cast<EnemyUpdatePlayerCollisionState *>(g_PlayerCollisionFlags)->flags |= 0x20;
}

#pragma var_order(i, this)
void EnemyUpdateOverlay::ReleaseEffects()
{
    i32 i;

    for (i = 0; i < effectCount; ++i)
    {
        if (!effects[i])
            continue;

        effects[i]->releaseRequested = 1;
        effects[i] = 0;
    }

    effectCount = 0;
}

void EnemyUpdateOverlay::ClampPosition()
{
    if (shouldClampPosition)
    {
        if (position.x < clampMinX)
            position.x = clampMinX;
        else if (position.x > clampMaxX)
            position.x = clampMaxX;

        if (position.y < clampMinY)
            position.y = clampMinY;
        else if (position.y > clampMaxY)
            position.y = clampMaxY;
    }
}

#pragma var_order(effect, i, this)
void EnemyUpdateOverlay::UpdateEffects()
{
    EnemyUpdateEffect *effect;
    i32 i;

    for (i = 0; i < effectCount; ++i)
    {
        effect = effects[i];
        if (!effect)
            continue;

        effect->visible = !hideEffects;
        effect->position = position;
        if (effect->distance < effectDistance)
            effect->distance += 0.3f;
        effect->angle = AddNormalizeAngle(effect->angle, 0.031415928f);
    }
}

} // namespace th07
