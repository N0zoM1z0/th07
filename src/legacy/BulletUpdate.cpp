#include "BulletManager.hpp"

namespace th07
{

struct BulletUpdateVec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct BulletUpdateSprite
{
    u8 unknown00[0x2C];
    f32 height;
    f32 width;
};

// This is the target-observed part of an ANM VM used by the Bullet update
// loop.  The full shared ANM layout remains outside this lane.
struct BulletUpdateAnmVm
{
    u8 unknown00[8];
    f32 rotationZ;
    u8 unknown0C[0x0C];
    f32 scaleX;
    f32 scaleY;
    u8 unknown20[0x198];
    u32 color;
    u8 unknown1BC[4];
    u32 flags;
    u8 unknown1C4[0x1C];
    i32 scriptActive;
    BulletUpdateSprite *sprite;
    u8 unknown1E8[0x24C - 0x1E8];

};
typedef char BulletUpdateAnmVm_size[(sizeof(BulletUpdateAnmVm) == 0x24C) ? 1 : -1];

struct BulletUpdateAnmManager
{
    i32 ExecuteScript(BulletUpdateAnmVm *vm);
};
extern BulletUpdateAnmManager *g_BulletUpdateAnmManager;

struct BulletUpdateTimer
{
    i32 previous;
    union
    {
        i32 subFrameBits;
        f32 subFrame;
    };
    i32 current;

    void Decrement(i32 amount);
};
typedef char BulletUpdateTimer_size[(sizeof(BulletUpdateTimer) == 0xC) ? 1 : -1];

struct BulletUpdateBullet
{
    BulletUpdateAnmVm animation[5];
    BulletUpdateVec3 grazeSize;       // +0xB7C
    u8 unknownB88[2];
    u8 drawListIndex;                 // +0xB8A
    u8 unknownB8B;
    BulletUpdateVec3 position;        // +0xB8C
    BulletUpdateVec3 velocity;        // +0xB98
    u8 unknownBA4[0x14];
    f32 speed;                        // +0xBB8
    f32 angle;                        // +0xBBC
    u8 unknownBC0[8];
    BulletUpdateTimer lifetime;       // +0xBC8
    BulletUpdateTimer collisionTimer; // +0xBD4
    u8 unknownBE0[0x10];
    i32 skipBoundsCountdown;          // +0xBF0
    u16 movementFlags;                // +0xBF4
    u16 collisionFlags;               // +0xBF6
    u8 unknownBF8[4];
    u16 state;                        // +0xBFC
    u16 offscreenCounter;             // +0xBFE
    u8 unknownC00;
    u8 grazeState;                    // +0xC01
    u8 unknownC02[2];
    BulletUpdateBullet *drawNext;     // +0xC04
    u8 unknownC08[0xD68 - 0xC08];

    void UpdateSpeedRamp();
    void UpdateAcceleration();
    void UpdateRotation();
    void UpdateDirectionChange();
    void UpdateDirectionSet();
    void UpdateAimAtPlayer();
    void UpdateBounce();
    void ApplyMovementCommands();
    void Clear();
};
typedef char BulletUpdateBullet_size[(sizeof(BulletUpdateBullet) == 0xD68) ? 1 : -1];

struct BulletUpdateLaser
{
    BulletUpdateAnmVm primaryAnimation;
    BulletUpdateAnmVm secondaryAnimation;
    BulletUpdateVec3 position;        // +0x498
    f32 angle;                        // +0x4A4
    f32 startOffset;                  // +0x4A8
    f32 endOffset;                    // +0x4AC
    f32 maximumLength;                // +0x4B0
    f32 width;                        // +0x4B4
    f32 hitboxThickness;              // +0x4B8
    f32 speed;                        // +0x4BC
    i32 startTime;                    // +0x4C0
    i32 hitboxStartTime;              // +0x4C4
    i32 duration;                     // +0x4C8
    i32 despawnDuration;              // +0x4CC
    i32 hitboxEndDelay;               // +0x4D0
    i32 isInUse;                      // +0x4D4
    BulletUpdateTimer timer;          // +0x4D8
    u16 flags;                        // +0x4E4
    i16 color;                        // +0x4E6
    u8 state;                         // +0x4E8
    u8 unknown4E9[3];
};
typedef char BulletUpdateLaser_size[(sizeof(BulletUpdateLaser) == 0x4EC) ? 1 : -1];

struct BulletUpdateItemManager
{
    void OnUpdate();
    void Spawn(BulletUpdateVec3 *position, i32 itemType, i32 count);
};
extern BulletUpdateItemManager g_ItemManager;

struct BulletUpdatePlayer
{
    i32 CheckGraze(BulletUpdateVec3 *center, BulletUpdateVec3 *size);
    i32 CalcKillBoxCollision(BulletUpdateVec3 *center, BulletUpdateVec3 *size);
    i32 CalcLaserHitbox(BulletUpdateVec3 *center, BulletUpdateVec3 *size, BulletUpdateVec3 *position, f32 angle,
                        i32 canGraze);
};
extern BulletUpdatePlayer g_Player;

struct BulletUpdateTimerManager
{
    void Advance(i32 *current, i32 *subFrame);
};
extern BulletUpdateTimerManager g_TimerManager;

extern i8 g_BulletUpdatePaused;
extern f32 g_FrameMultiplier;
extern i32 g_BulletPointItem;

struct BulletUpdateGameManager
{
    i32 IsInBounds(f32 x, f32 y, f32 width, f32 height);
};
extern BulletUpdateGameManager g_BulletUpdateGameManager;

extern f32 __stdcall AddNormalizeAngle(f32 first, f32 second);

static __forceinline void AdvanceTimer(BulletUpdateTimer *timer)
{
    timer->previous = timer->current;
    g_TimerManager.Advance(&timer->current, &timer->subFrameBits);
}

static __forceinline void ResetTimer(BulletUpdateTimer *timer)
{
    timer->current = 0;
    timer->subFrame = 0;
    timer->previous = -999;
}

#pragma var_order(lifetimeTimer, collisionTimer)
void BulletUpdateBullet::Clear()
{
    BulletUpdateTimer *lifetimeTimer;
    BulletUpdateTimer *collisionTimer;

    state = 0;
    lifetimeTimer = &lifetime;
    ResetTimer(lifetimeTimer);
    collisionTimer = &this->collisionTimer;
    ResetTimer(collisionTimer);
}

static __forceinline f32 TimerAsFramesFloat(BulletUpdateTimer *timer)
{
    return (f32)timer->current + timer->subFrame;
}

static __forceinline f32 Reciprocal(f32 value)
{
    return 1.0f / value;
}

#pragma var_order(collisionState, i, hitboxThickness, bulletSchedulerIndex, hitboxSize, bullet, fadeAlpha, laser,     \
                  hitboxCenter, rampFrames, spawnFastOffset, spawnNormalOffset, spawnSlowOffset, despawnOffset,      \
                  lifetimeTimer, bulletVelocity, bulletPosition, vectorCtorThis, vectorCtorX, vectorCtorY,           \
                  vectorCtorZ, collisionTimerCurrent, spawnFastScale, spawnFastProduct, spawnFastVelocity,          \
                  spawnFastPosition, spawnNormalScale, spawnNormalProduct, spawnNormalVelocity,                     \
                  spawnNormalPosition, spawnSlowScale, spawnSlowProduct, spawnSlowVelocity, spawnSlowPosition,      \
                  despawnScale, despawnProduct, despawnVelocity, despawnPosition, vectorDivideThis,                  \
                  vectorDivideArgument, lifetimeAdvanceTimer, collisionAdvanceTimer, normalizedAngle,               \
                  startFadeTimer, rampTimerCurrent, rampTimer,                                                       \
                  startHitboxTimerCurrent, startResetTimer, activeHitboxTimerCurrent, activeResetTimer,              \
                  despawnFadeTimer, despawnTimer, despawnHitboxTimerCurrent, laserTimer, managerTimer)
int __fastcall BulletManager::OnUpdate(BulletManager *manager)
{
    BulletUpdateBullet *bullet;
    BulletUpdateLaser *laser;
    BulletUpdateVec3 hitboxCenter;
    BulletUpdateVec3 hitboxSize;
    BulletUpdateTimer *lifetimeTimer;
    BulletUpdateTimer *managerTimer;
    BulletUpdateVec3 *bulletVelocity;
    BulletUpdateVec3 *bulletPosition;
    BulletUpdateVec3 *spawnFastVelocity;
    BulletUpdateVec3 *spawnFastPosition;
    BulletUpdateVec3 spawnFastProduct;
    BulletUpdateVec3 spawnFastOffset;
    BulletUpdateVec3 *spawnNormalVelocity;
    BulletUpdateVec3 *spawnNormalPosition;
    BulletUpdateVec3 spawnNormalProduct;
    BulletUpdateVec3 spawnNormalOffset;
    BulletUpdateVec3 *spawnSlowVelocity;
    BulletUpdateVec3 *spawnSlowPosition;
    BulletUpdateVec3 spawnSlowProduct;
    BulletUpdateVec3 spawnSlowOffset;
    BulletUpdateVec3 *despawnVelocity;
    BulletUpdateVec3 *despawnPosition;
    BulletUpdateVec3 despawnProduct;
    BulletUpdateVec3 despawnOffset;
    BulletUpdateVec3 *vectorCtorThis;
    f32 vectorCtorX;
    f32 vectorCtorY;
    f32 vectorCtorZ;
    const BulletUpdateVec3 *vectorDivideThis;
    f32 vectorDivideArgument;
    BulletUpdateTimer *lifetimeAdvanceTimer;
    BulletUpdateTimer *collisionAdvanceTimer;
    f32 spawnFastScale;
    f32 spawnNormalScale;
    f32 spawnSlowScale;
    f32 despawnScale;
    f32 hitboxThickness;
    i32 fadeAlpha;
    i32 bulletSchedulerIndex;
    i32 i;
    i32 collisionState;
    i32 collisionTimerCurrent;
    i32 rampFrames;
    i32 rampTimerCurrent;
    i32 startHitboxTimerCurrent;
    i32 activeHitboxTimerCurrent;
    i32 despawnHitboxTimerCurrent;
    BulletUpdateTimer *startFadeTimer;
    BulletUpdateTimer *rampTimer;
    BulletUpdateTimer *startResetTimer;
    BulletUpdateTimer *activeResetTimer;
    BulletUpdateTimer *despawnFadeTimer;
    BulletUpdateTimer *despawnTimer;
    BulletUpdateTimer *laserTimer;
    f32 normalizedAngle;

    bulletSchedulerIndex = 0;
    bullet = reinterpret_cast<BulletUpdateBullet *>(reinterpret_cast<u8 *>(manager) + 0xB8C0);

    if (g_BulletUpdatePaused)
    {
        return 1;
    }

    g_ItemManager.OnUpdate();

    // +0x37A128 is the target's active count; +0x37A144 is its six-list draw scheduler.
    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(manager) + 0x37A128) = 0;
    reinterpret_cast<BulletUpdateBullet **>(manager)[911446] = 0;
    reinterpret_cast<BulletUpdateBullet **>(manager)[911445] = 0;
    reinterpret_cast<BulletUpdateBullet **>(manager)[911444] = 0;
    reinterpret_cast<BulletUpdateBullet **>(manager)[911443] = 0;
    reinterpret_cast<BulletUpdateBullet **>(manager)[911442] = 0;
    reinterpret_cast<BulletUpdateBullet **>(manager)[911441] = 0;

    for (i = 0; i < 1024; i++)
    {
        if (!bullet->state)
        {
            goto advance_bullet;
        }
        else
        {
            ++*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(manager) + 0x37A128);
            switch (bullet->state)
            {
        reset_fired_bullet:
            bullet->state = 1;
            lifetimeTimer = &bullet->lifetime;
            ResetTimer(lifetimeTimer);
        case 1:
        update_fired_bullet:
            bullet->ApplyMovementCommands();
            if (bullet->movementFlags)
            {
                if (bullet->movementFlags & 1)
                {
                    bullet->UpdateSpeedRamp();
                }
                if (bullet->movementFlags & 0x10)
                {
                    bullet->UpdateAcceleration();
                }
                if (bullet->movementFlags & 0x20)
                {
                    bullet->UpdateRotation();
                }
                if (bullet->movementFlags & 0x40)
                {
                    bullet->UpdateDirectionChange();
                }
                if (bullet->movementFlags & 0x100)
                {
                    bullet->UpdateDirectionSet();
                }
                if (bullet->movementFlags & 0x80)
                {
                    bullet->UpdateAimAtPlayer();
                }
                if (bullet->movementFlags & 0xC00)
                {
                    bullet->UpdateBounce();
                }
            }

            if (bullet->skipBoundsCountdown)
            {
                --bullet->skipBoundsCountdown;
            }
            bulletVelocity = &bullet->velocity;
            bulletPosition = &bullet->position;
            bulletPosition->x += bulletVelocity->x;
            bulletPosition->y += bulletVelocity->y;
            bulletPosition->z += bulletVelocity->z;

            if (!bullet->skipBoundsCountdown)
            {
                if (!g_BulletUpdateGameManager.IsInBounds(bullet->position.x, bullet->position.y,
                                                          bullet->animation[0].sprite->width,
                                                          bullet->animation[0].sprite->height))
                {
                    if (bullet->movementFlags & 0xDC0)
                    {
                        if (++bullet->offscreenCounter >= 0x80)
                        {
                            bullet->Clear();
                            goto advance_bullet;
                        }
                    }
                    else
                    {
                        if (!bullet->offscreenCounter)
                        {
                            bullet->Clear();
                            goto advance_bullet;
                        }
                        --bullet->offscreenCounter;
                    }
                }
                else
                {
                    bullet->offscreenCounter = 0;
                }
            }

            if (!bullet->grazeState &&
                (collisionTimerCurrent = bullet->collisionTimer.current) >= 16)
            {
                collisionState = g_Player.CheckGraze(&bullet->position, &bullet->grazeSize);
                if (collisionState == 1)
                {
                    bullet->grazeState = 1;
                    goto check_bullet_kill_box;
                }
                if (collisionState == 2 && !(bullet->collisionFlags & 0x1000))
                {
                    bullet->state = 5;
                    g_ItemManager.Spawn(&bullet->position, g_BulletPointItem, 1);
                }
            }
            else
            {
            check_bullet_kill_box:
                collisionState = g_Player.CalcKillBoxCollision(&bullet->position, &bullet->grazeSize);
                if (collisionState && (collisionState != 2 || !(bullet->collisionFlags & 0x1000)))
                {
                    bullet->state = 5;
                    if (collisionState == 2)
                    {
                        g_ItemManager.Spawn(&bullet->position, g_BulletPointItem, 1);
                    }
                }
            }

            if (bullet->animation[0].scriptActive)
            {
                g_BulletUpdateAnmManager->ExecuteScript(&bullet->animation[0]);
            }
            break;
        case 2:
            bullet->collisionTimer.Decrement(1);
            spawnFastVelocity = &bullet->velocity;
            spawnFastScale = Reciprocal(2.0f);
            spawnFastProduct.z = spawnFastScale * spawnFastVelocity->z;
            spawnFastProduct.y = spawnFastScale * spawnFastVelocity->y;
            spawnFastProduct.x = spawnFastScale * spawnFastVelocity->x;
            spawnFastOffset = spawnFastProduct;
            spawnFastPosition = &bullet->position;
            spawnFastPosition->x += spawnFastOffset.x;
            spawnFastPosition->y += spawnFastOffset.y;
            spawnFastPosition->z += spawnFastOffset.z;
            if (!g_BulletUpdateAnmManager->ExecuteScript(&bullet->animation[1]))
            {
                break;
            }
            goto reset_fired_bullet;
        case 3:
            bullet->collisionTimer.Decrement(1);
            spawnNormalVelocity = &bullet->velocity;
            spawnNormalScale = Reciprocal(2.5f);
            spawnNormalProduct.z = spawnNormalScale * spawnNormalVelocity->z;
            spawnNormalProduct.y = spawnNormalScale * spawnNormalVelocity->y;
            spawnNormalProduct.x = spawnNormalScale * spawnNormalVelocity->x;
            spawnNormalOffset = spawnNormalProduct;
            spawnNormalPosition = &bullet->position;
            spawnNormalPosition->x += spawnNormalOffset.x;
            spawnNormalPosition->y += spawnNormalOffset.y;
            spawnNormalPosition->z += spawnNormalOffset.z;
            if (!g_BulletUpdateAnmManager->ExecuteScript(&bullet->animation[2]))
            {
                break;
            }
            goto reset_fired_bullet;
        case 4:
            bullet->collisionTimer.Decrement(1);
            spawnSlowVelocity = &bullet->velocity;
            spawnSlowScale = Reciprocal(3.0f);
            spawnSlowProduct.z = spawnSlowScale * spawnSlowVelocity->z;
            spawnSlowProduct.y = spawnSlowScale * spawnSlowVelocity->y;
            spawnSlowProduct.x = spawnSlowScale * spawnSlowVelocity->x;
            spawnSlowOffset = spawnSlowProduct;
            spawnSlowPosition = &bullet->position;
            spawnSlowPosition->x += spawnSlowOffset.x;
            spawnSlowPosition->y += spawnSlowOffset.y;
            spawnSlowPosition->z += spawnSlowOffset.z;
            if (!g_BulletUpdateAnmManager->ExecuteScript(&bullet->animation[3]))
            {
                break;
            }
            goto reset_fired_bullet;
        case 5:
            despawnVelocity = &bullet->velocity;
            despawnScale = Reciprocal(2.0f);
            despawnProduct.z = despawnScale * despawnVelocity->z;
            despawnProduct.y = despawnScale * despawnVelocity->y;
            despawnProduct.x = despawnScale * despawnVelocity->x;
            despawnOffset = despawnProduct;
            despawnPosition = &bullet->position;
            despawnPosition->x += despawnOffset.x;
            despawnPosition->y += despawnOffset.y;
            despawnPosition->z += despawnOffset.z;
            if (g_BulletUpdateAnmManager->ExecuteScript(&bullet->animation[4]))
            {
                bullet->Clear();
                goto advance_bullet;
            }
            break;
            }

            lifetimeAdvanceTimer = &bullet->lifetime;
            AdvanceTimer(lifetimeAdvanceTimer);
            collisionAdvanceTimer = &bullet->collisionTimer;
            AdvanceTimer(collisionAdvanceTimer);
            bullet->drawNext = reinterpret_cast<BulletUpdateBullet **>(manager)[911441 + bullet->drawListIndex];
            reinterpret_cast<BulletUpdateBullet **>(manager)[911441 + bullet->drawListIndex] = bullet;
        }

    advance_bullet:
        if (--bulletSchedulerIndex < 0)
        {
            bulletSchedulerIndex = 1023;
            bullet += 1024;
        }
        --bullet;
    }

    laser = reinterpret_cast<BulletUpdateLaser *>(reinterpret_cast<u8 *>(manager) + 0x366628);
    for (i = 0; i < 64; i++, laser++)
    {
        if (!laser->isInUse)
        {
            continue;
        }

        laser->endOffset += g_FrameMultiplier * laser->speed;
        if (laser->maximumLength < laser->endOffset - laser->startOffset)
        {
            laser->startOffset = laser->endOffset - laser->maximumLength;
        }
        if (laser->startOffset < 0.0f)
        {
            laser->startOffset = 0.0f;
        }

        hitboxSize.y = laser->width / 2.0f;
        hitboxSize.x = laser->endOffset - laser->startOffset;
        hitboxCenter.x = (laser->endOffset - laser->startOffset) / 2.0f + laser->startOffset + laser->position.x;
        hitboxCenter.y = laser->position.y;
        laser->primaryAnimation.scaleX = laser->width / laser->primaryAnimation.sprite->width;
        hitboxThickness = laser->endOffset - laser->startOffset;
        laser->primaryAnimation.scaleY = hitboxThickness / laser->primaryAnimation.sprite->height;
        normalizedAngle = AddNormalizeAngle(1.5707964f + laser->angle, 0.0f);
        laser->primaryAnimation.rotationZ = normalizedAngle;
        laser->primaryAnimation.flags |= 4;

        switch (laser->state)
        {
        case 0:
            if (laser->flags & 1)
            {
                startFadeTimer = &laser->timer;
                fadeAlpha =
                    (i32)(TimerAsFramesFloat(startFadeTimer) * 255.0f / laser->startTime);
                if (fadeAlpha > 255)
                {
                    fadeAlpha = 255;
                }
                laser->primaryAnimation.color = fadeAlpha << 24;
            }
            else
            {
                rampFrames = laser->startTime > 30 ? 30 : laser->startTime;
                rampTimerCurrent = laser->timer.current;
                if (laser->startTime - rampFrames < rampTimerCurrent)
                {
                    rampTimer = &laser->timer;
                    hitboxThickness =
                        TimerAsFramesFloat(rampTimer) * laser->width / laser->startTime;
                }
                else
                {
                    hitboxThickness = 1.2f;
                }
                laser->hitboxThickness = hitboxThickness;
                laser->primaryAnimation.scaleX = hitboxThickness / 16.0f;
                hitboxSize.x = hitboxThickness / 2.0f;
            }
            if ((laser->timer.current >= laser->hitboxStartTime) != 0)
            {
                startHitboxTimerCurrent = laser->timer.current;
                g_Player.CalcLaserHitbox(&hitboxCenter, &hitboxSize, &laser->position, laser->angle,
                                         startHitboxTimerCurrent % 12 == 0);
            }
            if ((laser->timer.current < laser->startTime) != 0)
            {
                break;
            }
            startResetTimer = &laser->timer;
            ResetTimer(startResetTimer);
            ++laser->state;
            laser->hitboxThickness = laser->width;
            // fall through
        case 1:
            activeHitboxTimerCurrent = laser->timer.current;
            g_Player.CalcLaserHitbox(&hitboxCenter, &hitboxSize, &laser->position, laser->angle,
                                     activeHitboxTimerCurrent % 12 == 0);
            if ((laser->timer.current < laser->duration) != 0)
            {
                break;
            }
            activeResetTimer = &laser->timer;
            ResetTimer(activeResetTimer);
            ++laser->state;
            if (!laser->despawnDuration)
            {
                laser->isInUse = 0;
                continue;
            }
            // fall through
        case 2:
            if (laser->flags & 1)
            {
                despawnFadeTimer = &laser->timer;
                fadeAlpha =
                    (i32)(TimerAsFramesFloat(despawnFadeTimer) * 255.0f / laser->startTime);
                if (fadeAlpha > 255)
                {
                    fadeAlpha = 255;
                }
                laser->primaryAnimation.color = fadeAlpha << 24;
            }
            else if (laser->despawnDuration > 0)
            {
                despawnTimer = &laser->timer;
                hitboxThickness = laser->width -
                                  TimerAsFramesFloat(despawnTimer) * laser->width /
                                      laser->despawnDuration;
                laser->primaryAnimation.scaleX = hitboxThickness / 16.0f;
                hitboxSize.x = hitboxThickness / 2.0f;
            }
            if ((laser->timer.current < laser->hitboxEndDelay) != 0)
            {
                despawnHitboxTimerCurrent = laser->timer.current;
                g_Player.CalcLaserHitbox(&hitboxCenter, &hitboxSize, &laser->position, laser->angle,
                                         despawnHitboxTimerCurrent % 12 == 0);
            }
            if ((laser->timer.current < laser->despawnDuration) != 0)
            {
                break;
            }
            else
            {
                laser->isInUse = 0;
                continue;
            }
            break;
        }

        if (laser->startOffset >= 640.0f)
        {
            laser->isInUse = 0;
        }
        laserTimer = &laser->timer;
        AdvanceTimer(laserTimer);
        g_BulletUpdateAnmManager->ExecuteScript(&laser->primaryAnimation);
    }

    if (*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(manager) + 0x37A12C))
    {
        --*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(manager) + 0x37A12C);
    }
    managerTimer = reinterpret_cast<BulletUpdateTimer *>(reinterpret_cast<u8 *>(manager) + 0x37A130);
    AdvanceTimer(managerTimer);
    ++*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(manager) + 0x37A13C);
    return 1;
}

} // namespace th07
