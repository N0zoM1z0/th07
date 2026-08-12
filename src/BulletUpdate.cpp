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
    u8 unknown1C4[0x20];
    BulletUpdateSprite *sprite;
    u8 unknown1E8[0x24C - 0x1E8];

    i32 ExecuteScript();
};
typedef char BulletUpdateAnmVm_size[(sizeof(BulletUpdateAnmVm) == 0x24C) ? 1 : -1];

struct BulletUpdateTimer
{
    i32 previous;
    i32 subFrame;
    i32 current;
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
    u8 unknownBD4[8];
    i32 collisionDelay;               // +0xBDC
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
    f32 speed;                        // +0x4B8
    i32 startTime;                    // +0x4BC
    i32 hitboxStartTime;              // +0x4C0
    i32 duration;                     // +0x4C4
    i32 despawnDuration;              // +0x4C8
    i32 hitboxEndDelay;               // +0x4CC
    u8 unknown4D0[4];
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

extern u8 g_BulletUpdatePaused;
extern f32 g_FrameMultiplier;
extern i32 g_BulletPointItem;

extern i32 __stdcall BulletIsInBounds(f32 x, f32 y, f32 width, f32 height);

static __forceinline void AdvanceTimer(BulletUpdateTimer *timer)
{
    timer->previous = timer->current;
    g_TimerManager.Advance(&timer->current, &timer->subFrame);
}

static __forceinline void ResetTimer(BulletUpdateTimer *timer)
{
    timer->previous = -999;
    timer->subFrame = 0;
    timer->current = 0;
}

static __forceinline void UpdateLaser(BulletUpdateLaser *laser)
{
    BulletUpdateVec3 hitboxCenter;
    BulletUpdateVec3 hitboxSize;
    i32 fadeAlpha;
    i32 rampFrames;
    f32 hitboxThickness;
    f32 visibleLength;

    if (!laser->isInUse)
    {
        return;
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

    visibleLength = laser->endOffset - laser->startOffset;
    hitboxSize.x = visibleLength;
    hitboxSize.y = laser->width / 2.0f;
    hitboxCenter.x = visibleLength / 2.0f + laser->startOffset + laser->position.x;
    hitboxCenter.y = laser->position.y;
    hitboxCenter.z = 0.0f;
    laser->primaryAnimation.scaleX = laser->width / laser->primaryAnimation.sprite->width;
    laser->primaryAnimation.scaleY = visibleLength / laser->primaryAnimation.sprite->height;
    laser->primaryAnimation.rotationZ = laser->angle + 1.5707964f;
    laser->primaryAnimation.flags |= 4;

    switch (laser->state)
    {
    case 0:
        if (laser->flags & 1)
        {
            fadeAlpha = (i32)((laser->timer.current + laser->timer.subFrame) * 255.0f / laser->startTime);
            if (fadeAlpha > 255)
            {
                fadeAlpha = 255;
            }
            laser->primaryAnimation.color = fadeAlpha << 24;
        }
        else
        {
            rampFrames = laser->startTime <= 30 ? laser->startTime : 30;
            if (laser->startTime - rampFrames < laser->timer.current)
            {
                hitboxThickness =
                    (laser->timer.current + laser->timer.subFrame) * laser->width / laser->startTime;
            }
            else
            {
                hitboxThickness = 1.2f;
            }
            laser->primaryAnimation.scaleX = hitboxThickness / 16.0f;
            hitboxSize.x = hitboxThickness / 2.0f;
        }
        if (laser->timer.current >= laser->hitboxStartTime)
        {
            g_Player.CalcLaserHitbox(&hitboxCenter, &hitboxSize, &laser->position, laser->angle,
                                     laser->timer.current % 12 == 0);
        }
        if (laser->timer.current < laser->startTime)
        {
            break;
        }
        ResetTimer(&laser->timer);
        ++laser->state;
        // fall through
    case 1:
        g_Player.CalcLaserHitbox(&hitboxCenter, &hitboxSize, &laser->position, laser->angle,
                                 laser->timer.current % 12 == 0);
        if (laser->timer.current < laser->duration)
        {
            break;
        }
        ResetTimer(&laser->timer);
        ++laser->state;
        if (!laser->despawnDuration)
        {
            laser->isInUse = 0;
            return;
        }
        // fall through
    case 2:
        if (laser->flags & 1)
        {
            fadeAlpha = (i32)((laser->timer.current + laser->timer.subFrame) * 255.0f / laser->startTime);
            if (fadeAlpha > 255)
            {
                fadeAlpha = 255;
            }
            laser->primaryAnimation.color = fadeAlpha << 24;
        }
        else if (laser->despawnDuration > 0)
        {
            hitboxThickness = laser->width -
                              (laser->timer.current + laser->timer.subFrame) * laser->width / laser->despawnDuration;
            laser->primaryAnimation.scaleX = hitboxThickness / 16.0f;
            hitboxSize.x = hitboxThickness / 2.0f;
        }
        if (laser->timer.current < laser->hitboxEndDelay)
        {
            g_Player.CalcLaserHitbox(&hitboxCenter, &hitboxSize, &laser->position, laser->angle,
                                     laser->timer.current % 12 == 0);
        }
        if (laser->timer.current >= laser->despawnDuration)
        {
            laser->isInUse = 0;
            return;
        }
        break;
    }

    if (laser->startOffset >= 640.0f)
    {
        laser->isInUse = 0;
    }
    AdvanceTimer(&laser->timer);
    laser->primaryAnimation.ExecuteScript();
}

#pragma var_order(collisionState, i, visibleLength, hitboxSize, bullet, laser, hitboxCenter)
int __fastcall BulletManager::OnUpdate(BulletManager *manager)
{
    BulletUpdateBullet *bullet;
    BulletUpdateLaser *laser;
    i32 i;

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

    bullet = reinterpret_cast<BulletUpdateBullet *>(reinterpret_cast<u8 *>(manager) + 0xB8C0);
    for (i = 0; i < 1024; i++, bullet++)
    {
        BulletUpdateAnmVm *animation;
        i32 collisionState;

        if (!bullet->state)
        {
            continue;
        }

        ++*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(manager) + 0x37A128);
        switch (bullet->state)
        {
        case 2:
            bullet->position.x += bullet->velocity.x / 2.0f;
            bullet->position.y += bullet->velocity.y / 2.0f;
            bullet->position.z += bullet->velocity.z / 2.0f;
            if (!bullet->animation[1].ExecuteScript())
            {
                break;
            }
            bullet->state = 1;
            ResetTimer(&bullet->lifetime);
            // fall through
        case 1:
        update_fired_bullet:
            bullet->ApplyMovementCommands();
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

            if (bullet->skipBoundsCountdown)
            {
                --bullet->skipBoundsCountdown;
            }
            bullet->position.x += bullet->velocity.x;
            bullet->position.y += bullet->velocity.y;
            bullet->position.z += bullet->velocity.z;

            animation = &bullet->animation[0];
            if (!bullet->skipBoundsCountdown &&
                !BulletIsInBounds(bullet->position.x, bullet->position.y, animation->sprite->width,
                                  animation->sprite->height))
            {
                if (!(bullet->movementFlags & 0xDC0))
                {
                    if (!bullet->offscreenCounter)
                    {
                        bullet->Clear();
                        continue;
                    }
                    --bullet->offscreenCounter;
                }
                else if (++bullet->offscreenCounter >= 0x80)
                {
                    bullet->Clear();
                    continue;
                }
            }
            else
            {
                bullet->offscreenCounter = 0;
            }

            if (!bullet->grazeState && bullet->collisionDelay >= 16)
            {
                collisionState = g_Player.CheckGraze(&bullet->position, &bullet->grazeSize);
                if (collisionState == 1)
                {
                    bullet->grazeState = 1;
                }
                if (collisionState == 2 && !(bullet->collisionFlags & 0x1000))
                {
                    bullet->state = 5;
                    g_ItemManager.Spawn(&bullet->position, g_BulletPointItem, 1);
                }
            }
            if (bullet->grazeState)
            {
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
            if (bullet->animation[0].ExecuteScript())
            {
                bullet->Clear();
                continue;
            }
            break;
        case 3:
            bullet->position.x += bullet->velocity.x / 2.5f;
            bullet->position.y += bullet->velocity.y / 2.5f;
            bullet->position.z += bullet->velocity.z / 2.5f;
            if (bullet->animation[2].ExecuteScript())
            {
                bullet->state = 1;
                ResetTimer(&bullet->lifetime);
                goto update_fired_bullet;
            }
            break;
        case 4:
            bullet->position.x += bullet->velocity.x / 3.0f;
            bullet->position.y += bullet->velocity.y / 3.0f;
            bullet->position.z += bullet->velocity.z / 3.0f;
            if (bullet->animation[3].ExecuteScript())
            {
                bullet->state = 1;
                ResetTimer(&bullet->lifetime);
                goto update_fired_bullet;
            }
            break;
        case 5:
            bullet->position.x += bullet->velocity.x / 2.0f;
            bullet->position.y += bullet->velocity.y / 2.0f;
            bullet->position.z += bullet->velocity.z / 2.0f;
            if (bullet->animation[4].ExecuteScript())
            {
                bullet->Clear();
                continue;
            }
            break;
        }

        AdvanceTimer(&bullet->lifetime);
        bullet->drawNext = reinterpret_cast<BulletUpdateBullet **>(manager)[911441 + bullet->drawListIndex];
        reinterpret_cast<BulletUpdateBullet **>(manager)[911441 + bullet->drawListIndex] = bullet;
    }

    laser = reinterpret_cast<BulletUpdateLaser *>(reinterpret_cast<u8 *>(manager) + 0x366628);
    for (i = 0; i < 64; i++, laser++)
    {
        UpdateLaser(laser);
    }

    AdvanceTimer(reinterpret_cast<BulletUpdateTimer *>(reinterpret_cast<u8 *>(manager) + 0x37A134));
    return 1;
}

} // namespace th07
