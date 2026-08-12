#include "Player.hpp"

#include <math.h>

namespace th07
{
struct PlayerBulletAnimation
{
    void SetAndExecuteScript(i32 script);
};

struct EffectManager
{
    void SpawnParticles(i32 effect, D3DXVECTOR3 *position, i32 count, i32 color);
    void SpawnParticlesColored(i32 effect, D3DXVECTOR3 *position, i32 count, i32 blendMode, i32 color);
};

struct GameManager
{
    i32 unknown00;
    i32 scoreRelated;
    u8 unknown08[0xC];
    i32 grazeInStage;
    i32 grazeInTotal;
    u8 unknown1C[0x74];
    i32 rankBaseline;
};

struct GrazeState
{
    void BeginPlayerDeath();
    void CancelCapture();
};

struct SoundPlayer
{
    i32 PlaySoundByIdx(i32 sound, i32 param);
};

struct PlayerCollisionState
{
    u8 unknown00[0xD6];
    u16 flags;
};

extern EffectManager g_EffectManager;
extern GameManager *g_GameManager;
extern GrazeState g_GrazeState;
extern SoundPlayer g_SoundPlayer;
extern i32 g_PlayerCollisionFlags;
extern i32 g_HideGrazeCounter;
extern i32 g_GuiFlags;
extern i32 g_StageScore;
extern i32 g_RankValue;
extern i32 *g_PlayerAnimationData;

extern void __cdecl IncreaseSubrank(i32 amount);
extern i32 __cdecl AdvanceGrazeDisplay(i32 amount);
extern i32 __cdecl FinishGrazeDisplay(i32 amount);
extern void __fastcall RotatePlayerVector(D3DXVECTOR3 *out, D3DXVECTOR3 *relative, f32 angle);

#define PLAYER_BOXES_OVERLAP(leftA, topA, rightA, bottomA, leftB, topB, rightB, bottomB)                              \
    ((leftA) <= (rightB) && (rightA) >= (leftB) && (topA) <= (bottomB) && (bottomA) >= (topB))

#pragma var_order(bullet, i, enemyBottomRight, bulletBottomRight, enemyTopLeft, damage, bulletTopLeft)
i32 Player::CalcDamageToEnemy(D3DXVECTOR3 *enemyPosition, D3DXVECTOR3 *enemySize, i32 *bombHit)
{
    D3DXVECTOR3 bulletTopLeft;
    i32 damage;
    D3DXVECTOR3 enemyTopLeft;
    i32 i;
    PlayerBullet *bullet;
    D3DXVECTOR3 bulletBottomRight;
    D3DXVECTOR3 enemyBottomRight;
    i32 *lastEnemyHit;

    damage = 0;
    lastEnemyHit = &lastEnemyHitX;
    if (lastEnemyHit[2] == lastEnemyHit[0])
    {
        return 0;
    }

    enemyTopLeft.x = enemyPosition->x - enemySize->x * 0.5f;
    enemyTopLeft.y = enemyPosition->y - enemySize->y * 0.5f;
    enemyBottomRight.x = enemyPosition->x + enemySize->x * 0.5f;
    enemyBottomRight.y = enemyPosition->y + enemySize->y * 0.5f;
    bullet = &bullets[0];
    if (bombHit)
    {
        *bombHit = 0;
    }

    for (i = 0; i < 96; ++i, ++bullet)
    {
        if (bullet->state == 0 || (bullet->state != 1 && bullet->type != 3))
        {
            continue;
        }

        bulletTopLeft.x = bullet->position.x - bullet->size.x * 0.5f;
        bulletTopLeft.y = bullet->position.y - bullet->size.y * 0.5f;
        bulletBottomRight.x = bullet->position.x + bullet->size.x * 0.5f;
        bulletBottomRight.y = bullet->position.y + bullet->size.y * 0.5f;
        if (bulletTopLeft.y > enemyBottomRight.y || bulletTopLeft.x > enemyBottomRight.x ||
            bulletBottomRight.y < enemyTopLeft.y || bulletBottomRight.x < enemyTopLeft.x)
        {
            continue;
        }

        if (bombIsActive)
        {
            i32 reducedDamage = bullet->damage / 3;
            damage += reducedDamage ? reducedDamage : 1;
        }
        else
        {
            damage += bullet->damage;
        }

        if (bullet->type != 4 && bullet->type != 5)
        {
            if (bullet->state == 1)
            {
                PlayerBulletAnimation *animation = reinterpret_cast<PlayerBulletAnimation *>(bullet);
                animation->SetAndExecuteScript(g_PlayerAnimationData[bullet->animationIndex + 32 + 41916]);
                g_EffectManager.SpawnParticles(5, &bullet->position, 1, -1);
                bullet->position.z = 0.1f;
            }
            bullet->state = 2;
            if (bullet->type != 3)
            {
                bullet->velocityX /= 8.0f;
                bullet->velocityY /= 8.0f;
            }
        }

    }

    for (i = 0; i < 112; ++i)
    {
        PlayerDamageRegion *region = &damageRegions[i];
        D3DXVECTOR3 regionTopLeft;
        D3DXVECTOR3 regionBottomRight;

        if (region->size.x <= 0.0f)
        {
            continue;
        }
        regionTopLeft = region->position - region->size / 2.0f;
        regionBottomRight = region->position + region->size / 2.0f;
        if (regionTopLeft.x > enemyBottomRight.x || regionBottomRight.x < enemyTopLeft.x ||
            regionTopLeft.y > enemyBottomRight.y || regionBottomRight.y < enemyTopLeft.y)
        {
            continue;
        }

        damage += region->damage;
        region->accumulatedDamage += region->damage;
        if ((++collisionParticleCounter & 3) == 0)
        {
            g_EffectManager.SpawnParticles(i >= 96 ? 5 : 3, enemyPosition, 1, -1);
        }
        if (bombIsActive && bombHit)
        {
            *bombHit = 1;
        }
    }
    return damage;
}

#pragma var_order(collisionTopLeft, y, x, i, projectileBottomRight, projectileTopLeft, collision, collisionBottomRight)
i32 Player::CheckAuxProjectileCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 projectileTopLeft;
    D3DXVECTOR3 projectileBottomRight;
    D3DXVECTOR3 collisionTopLeft;
    D3DXVECTOR3 collisionBottomRight;
    PlayerAuxCollision *collision;
    i32 i;
    f32 x;
    f32 y;

    collision = &auxCollisions[0];
    projectileTopLeft.x = center->x - size->x / 2.0f;
    projectileTopLeft.y = center->y - size->y / 2.0f;
    projectileBottomRight.x = center->x + size->x / 2.0f;
    projectileBottomRight.y = center->y + size->y / 2.0f;
    for (i = 0; i < 96; ++i, ++collision)
    {
        if (collision->sizeX != 0.0f)
        {
            collisionTopLeft.x = collision->centerX - collision->sizeX / 2.0f;
            collisionTopLeft.y = collision->centerY - collision->sizeY / 2.0f;
            collisionBottomRight.x = collision->centerX + collision->sizeX / 2.0f;
            collisionBottomRight.y = collision->centerY + collision->sizeY / 2.0f;
            if (PLAYER_BOXES_OVERLAP(collisionTopLeft.x, collisionTopLeft.y, collisionBottomRight.x,
                                     collisionBottomRight.y, projectileTopLeft.x, projectileTopLeft.y,
                                     projectileBottomRight.x, projectileBottomRight.y))
            {
                collisionCountdown = collision->collisionValue;
                return 2;
            }
        }
        else
        {
            if (collision->radius != 0.0f)
            {
                x = center->x - collision->centerX;
                y = center->y - collision->centerY;
                if (x * x + y * y < collision->radius * collision->radius)
                {
                    collisionCountdown = collision->collisionValue;
                    return 2;
                }
            }
            else
            {
                continue;
            }
        }
    }
    return 0;
}

#pragma var_order(bulletBottomRight, bulletTopLeft)
i32 Player::CalcKillBoxCollision(D3DXVECTOR3 *bulletCenter, D3DXVECTOR3 *bulletSize)
{
    D3DXVECTOR3 bulletTopLeft;
    D3DXVECTOR3 bulletBottomRight;

    collisionCountdown = 6;
    if (CheckAuxProjectileCollision(bulletCenter, bulletSize))
    {
        return 2;
    }
    bulletTopLeft.x = bulletCenter->x - bulletSize->x / 2.0f;
    bulletTopLeft.y = bulletCenter->y - bulletSize->y / 2.0f;
    bulletBottomRight.x = bulletCenter->x + bulletSize->x / 2.0f;
    bulletBottomRight.y = bulletCenter->y + bulletSize->y / 2.0f;
    if (killBoxTopLeft.x > bulletBottomRight.x || killBoxBottomRight.x < bulletTopLeft.x ||
        killBoxTopLeft.y > bulletBottomRight.y || killBoxBottomRight.y < bulletTopLeft.y)
    {
        return 0;
    }
    reinterpret_cast<PlayerCollisionState *>(g_PlayerCollisionFlags)->flags |= 2;
    if (playerState == 4)
    {
        HandleCollisionDuringBomb(0);
        return 1;
    }
    else
    {
        if (playerState == 0)
        {
            g_GrazeState.BeginPlayerDeath();
            Die();
        }
        return 1;
    }
}

#pragma var_order(bulletBottomRight, bulletTopLeft)
i32 Player::CheckGraze(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 bulletTopLeft;
    D3DXVECTOR3 bulletBottomRight;

    collisionCountdown = 6;
    if (CheckAuxProjectileCollision(center, size))
    {
        return 2;
    }
    bulletTopLeft.x = center->x - size->x / 2.0f - 20.0f;
    bulletTopLeft.y = center->y - size->y / 2.0f - 20.0f;
    bulletBottomRight.x = center->x + size->x / 2.0f + 20.0f;
    bulletBottomRight.y = center->y + size->y / 2.0f + 20.0f;
    if (playerState == 2 || playerState == 1)
    {
        return 0;
    }
    if (grazeBoxTopLeft.x > bulletBottomRight.x || grazeBoxBottomRight.x < bulletTopLeft.x ||
        grazeBoxTopLeft.y > bulletBottomRight.y || grazeBoxBottomRight.y < bulletTopLeft.y)
    {
        return 0;
    }
    ScoreGraze(center);
    return 1;
}

#pragma var_order(itemBottomRight, itemTopLeft)
i32 Player::CalcItemBoxCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 itemTopLeft;
    D3DXVECTOR3 itemBottomRight;

    if (playerState != 0 && playerState != 3 && playerState != 4)
    {
        return 0;
    }
    itemTopLeft = *center - *size / 2.0f;
    itemBottomRight = *center + *size / 2.0f;
    if (itemBoxTopLeft.x > itemBottomRight.x || itemBoxBottomRight.x < itemTopLeft.x ||
        itemBoxTopLeft.y > itemBottomRight.y || itemBoxBottomRight.y < itemTopLeft.y)
    {
        return 0;
    }
    return 1;
}

#pragma var_order(playerRelativeTopLeft, laserBottomRight, laserTopLeft, playerRelativeBottomRight)
i32 Player::CalcLaserHitbox(D3DXVECTOR3 *laserCenter, D3DXVECTOR3 *laserSize, D3DXVECTOR3 *rotation, f32 angle,
                             i32 canGraze)
{
    D3DXVECTOR3 laserTopLeft;
    D3DXVECTOR3 laserBottomRight;
    D3DXVECTOR3 playerRelativeTopLeft;
    D3DXVECTOR3 playerRelativeBottomRight;

    laserTopLeft = positionCenter - *rotation;
    RotatePlayerVector(&laserBottomRight, &laserTopLeft, angle);
    laserBottomRight.z = 0.0f;
    laserTopLeft = laserBottomRight + *rotation;
    playerRelativeTopLeft = laserTopLeft - laserHitboxHalfSize;
    playerRelativeBottomRight = laserTopLeft + laserHitboxHalfSize;
    laserTopLeft = *laserCenter - *laserSize / 2.0f;
    laserBottomRight = *laserCenter + *laserSize / 2.0f;

    if (!(playerRelativeTopLeft.x > laserBottomRight.x || playerRelativeBottomRight.x < laserTopLeft.x ||
          playerRelativeTopLeft.y > laserBottomRight.y || playerRelativeBottomRight.y < laserTopLeft.y))
    {
        goto LASER_COLLISION;
    }
    if (canGraze == 0)
    {
        return 0;
    }
    laserTopLeft.x -= 48.0f;
    laserTopLeft.y -= 48.0f;
    laserBottomRight.x += 48.0f;
    laserBottomRight.y += 48.0f;
    if (playerRelativeTopLeft.x > laserBottomRight.x || playerRelativeBottomRight.x < laserTopLeft.x ||
        playerRelativeTopLeft.y > laserBottomRight.y || playerRelativeBottomRight.y < laserTopLeft.y)
    {
        return 0;
    }
    if (playerState == 2 || playerState == 1)
    {
        return 0;
    }
    ScoreGraze(&positionCenter);
    return 2;

LASER_COLLISION:
    reinterpret_cast<PlayerCollisionState *>(g_PlayerCollisionFlags)->flags |= 2;
    if (playerState == 4)
    {
        HandleCollisionDuringBomb(0);
        return 1;
    }
    if (playerState != 0)
    {
        return 0;
    }
    g_GrazeState.BeginPlayerDeath();
    Die();
    return 1;
}

i32 Player::ScoreGraze(D3DXVECTOR3 *center)
{
    D3DXVECTOR3 particlePosition;

    if (!g_HideGrazeCounter)
    {
        if (g_GameManager->grazeInStage < 9999)
        {
            ++g_GameManager->grazeInStage;
        }
        if (g_GameManager->grazeInTotal < 999999)
        {
            ++g_GameManager->grazeInTotal;
        }
    }
    particlePosition = (positionCenter + *center) / 2.0f;
    g_EffectManager.SpawnParticles(8, &particlePosition, grazeSoundVariant == 1 && grazeVariant == 0 ? 3 : 1,
                                   grazeSoundVariant == 1 && grazeVariant == 0 ? -32640 : -1);
    IncreaseSubrank(6);
    g_GuiFlags = (g_GuiFlags & 0xFFFFFF3F) | 0x80;
    g_SoundPlayer.PlaySoundByIdx(30, 0);
    g_StageScore += 20 * ((g_RankValue - g_GameManager->rankBaseline) / 1500) + 2500;
    g_GameManager->scoreRelated += 2000 / 10;
    if (grazeSoundVariant == 1)
    {
        if (grazeVariant)
        {
            AdvanceGrazeDisplay(30);
            return FinishGrazeDisplay(30);
        }
        AdvanceGrazeDisplay(80);
        return FinishGrazeDisplay(80);
    }
    return reinterpret_cast<i32>(this);
}

i32 Player::Die()
{
    i32 *lastEnemyHit;

    g_GrazeState.CancelCapture();
    g_EffectManager.SpawnParticlesColored(12, &positionCenter, 3, 1, -12566273);
    g_EffectManager.SpawnParticles(6, &positionCenter, 16, -1);
    playerState = 2;
    lastEnemyHit = &lastEnemyHitX;
    lastEnemyHit[2] = 0;
    lastEnemyHit[1] = 0;
    lastEnemyHit[0] = -999;
    return g_SoundPlayer.PlaySoundByIdx(4, 0);
}
#undef PLAYER_BOXES_OVERLAP
} // namespace th07
