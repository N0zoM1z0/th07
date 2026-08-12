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

extern EffectManager g_EffectManager;
extern GameManager g_GameManager;
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

#define PLAYER_BOXES_OVERLAP(leftA, topA, rightA, bottomA, leftB, topB, rightB, bottomB)                              \
    ((leftA) <= (rightB) && (rightA) >= (leftB) && (topA) <= (bottomB) && (bottomA) >= (topB))

i32 Player::CalcDamageToEnemy(D3DXVECTOR3 *enemyPosition, D3DXVECTOR3 *enemySize, i32 *bombHit)
{
    i32 damage = 0;
    f32 enemyLeft;
    f32 enemyTop;
    f32 enemyRight;
    f32 enemyBottom;
    i32 i;

    if (lastEnemyHitZ == lastEnemyHitX)
    {
        return 0;
    }

    enemyLeft = enemyPosition->x - enemySize->x * 0.5f;
    enemyTop = enemyPosition->y - enemySize->y * 0.5f;
    enemyRight = enemyPosition->x + enemySize->x * 0.5f;
    enemyBottom = enemyPosition->y + enemySize->y * 0.5f;
    if (bombHit)
    {
        *bombHit = 0;
    }

    for (i = 0; i < 96; ++i)
    {
        PlayerBullet *bullet = &bullets[i];
        f32 bulletLeft;
        f32 bulletTop;
        f32 bulletRight;
        f32 bulletBottom;

        if (bullet->state == 0 || (bullet->state != 1 && bullet->type != 3))
        {
            continue;
        }

        bulletLeft = bullet->position.x - bullet->size.x * 0.5f;
        bulletTop = bullet->position.y - bullet->size.y * 0.5f;
        bulletRight = bullet->position.x + bullet->size.x * 0.5f;
        bulletBottom = bullet->position.y + bullet->size.y * 0.5f;
        if (!PLAYER_BOXES_OVERLAP(bulletLeft, bulletTop, bulletRight, bulletBottom, enemyLeft, enemyTop, enemyRight,
                                  enemyBottom))
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
        f32 regionLeft;
        f32 regionTop;
        f32 regionRight;
        f32 regionBottom;

        if (region->sizeX <= 0.0f)
        {
            continue;
        }
        regionLeft = region->centerX - region->sizeX * 0.5f;
        regionTop = region->centerY - region->sizeY * 0.5f;
        regionRight = region->centerX + region->sizeX * 0.5f;
        regionBottom = region->centerY + region->sizeY * 0.5f;
        if (!PLAYER_BOXES_OVERLAP(regionLeft, regionTop, regionRight, regionBottom, enemyLeft, enemyTop, enemyRight,
                                  enemyBottom))
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

i32 Player::CheckAuxProjectileCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 projectileTopLeft;
    D3DXVECTOR3 projectileBottomRight;
    D3DXVECTOR3 collisionTopLeft;
    D3DXVECTOR3 collisionBottomRight;
    PlayerAuxCollision *collision;
    i32 i;

    projectileTopLeft.x = center->x - size->x * 0.5f;
    projectileTopLeft.y = center->y - size->y * 0.5f;
    projectileBottomRight.x = center->x + size->x * 0.5f;
    projectileBottomRight.y = center->y + size->y * 0.5f;
    collision = &auxCollisions[0];
    for (i = 0; i < 96; ++i, ++collision)
    {
        if (collision->sizeX == 0.0f)
        {
            if (collision->radius != 0.0f)
            {
                f32 x = center->x - collision->centerX;
                f32 y = center->y - collision->centerY;
                if (x * x + y * y >= collision->radius * collision->radius)
                {
                    continue;
                }
                else
                {
                    collisionCountdown = collision->collisionValue;
                    return 2;
                }
            }
        }
        else
        {
            collisionTopLeft.x = collision->centerX - collision->sizeX * 0.5f;
            collisionTopLeft.y = collision->centerY - collision->sizeY * 0.5f;
            collisionBottomRight.x = collision->centerX + collision->sizeX * 0.5f;
            collisionBottomRight.y = collision->centerY + collision->sizeY * 0.5f;
            if (PLAYER_BOXES_OVERLAP(collisionTopLeft.x, collisionTopLeft.y, collisionBottomRight.x,
                                     collisionBottomRight.y, projectileTopLeft.x, projectileTopLeft.y,
                                     projectileBottomRight.x, projectileBottomRight.y))
            {
                collisionCountdown = collision->collisionValue;
                return 2;
            }
        }
    }
    return 0;
}

i32 Player::CalcKillBoxCollision(D3DXVECTOR3 *bulletCenter, D3DXVECTOR3 *bulletSize)
{
    D3DXVECTOR3 bulletTopLeft;
    D3DXVECTOR3 bulletBottomRight;

    collisionCountdown = 6;
    if (CheckAuxProjectileCollision(bulletCenter, bulletSize))
    {
        return 2;
    }
    bulletTopLeft.x = bulletCenter->x - bulletSize->x * 0.5f;
    bulletTopLeft.y = bulletCenter->y - bulletSize->y * 0.5f;
    bulletBottomRight.x = bulletCenter->x + bulletSize->x * 0.5f;
    bulletBottomRight.y = bulletCenter->y + bulletSize->y * 0.5f;
    if (killBoxTopLeft.x <= bulletBottomRight.x && killBoxTopLeft.y <= bulletBottomRight.y &&
        killBoxBottomRight.x >= bulletTopLeft.x && killBoxBottomRight.y >= bulletTopLeft.y)
    {
        g_PlayerCollisionFlags |= 2;
        if (playerState == 4)
        {
            AdvanceGrazeDisplay(0);
            return 1;
        }
        if (playerState == 0)
        {
            g_GrazeState.BeginPlayerDeath();
            Die();
        }
        return 1;
    }
    return 0;
}

i32 Player::CheckGraze(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 bulletTopLeft;
    D3DXVECTOR3 bulletBottomRight;

    collisionCountdown = 6;
    if (CheckAuxProjectileCollision(center, size))
    {
        return 2;
    }
    bulletTopLeft.x = center->x - size->x * 0.5f - 20.0f;
    bulletTopLeft.y = center->y - size->y * 0.5f - 20.0f;
    bulletBottomRight.x = center->x + size->x * 0.5f + 20.0f;
    bulletBottomRight.y = center->y + size->y * 0.5f + 20.0f;
    if (playerState == 2 || playerState == 1)
    {
        return 0;
    }
    if (!PLAYER_BOXES_OVERLAP(grazeBoxTopLeft.x, grazeBoxTopLeft.y, grazeBoxBottomRight.x, grazeBoxBottomRight.y,
                              bulletTopLeft.x, bulletTopLeft.y, bulletBottomRight.x, bulletBottomRight.y))
    {
        return 0;
    }
    ScoreGraze(center);
    return 1;
}

i32 Player::CalcItemBoxCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 itemTopLeft;
    D3DXVECTOR3 itemBottomRight;

    if (playerState != 0 && playerState != 3 && playerState != 4)
    {
        return 0;
    }
    itemTopLeft = *center - *size * 0.5f;
    itemBottomRight = *center + *size * 0.5f;
    if (itemBoxTopLeft.x > itemBottomRight.x || itemBoxBottomRight.x < itemTopLeft.x ||
        itemBoxTopLeft.y > itemBottomRight.y || itemBoxBottomRight.y < itemTopLeft.y)
    {
        return 0;
    }
    return 1;
}

i32 Player::CalcLaserHitbox(D3DXVECTOR3 *laserCenter, D3DXVECTOR3 *laserSize, D3DXVECTOR3 *rotation, f32 angle,
                             i32 canGraze)
{
    f32 sine = sinf(angle);
    f32 cosine = cosf(angle);
    f32 relativeX = positionCenter.x - rotation->x;
    f32 relativeY = positionCenter.y - rotation->y;
    f32 playerX = relativeX * cosine - relativeY * sine + rotation->x;
    f32 playerY = relativeX * sine + relativeY * cosine + rotation->y;
    f32 playerLeft = playerX - laserHitboxHalfSize.x;
    f32 playerTop = playerY - laserHitboxHalfSize.y;
    f32 playerRight = playerX + laserHitboxHalfSize.x;
    f32 playerBottom = playerY + laserHitboxHalfSize.y;
    f32 laserLeft = laserCenter->x - laserSize->x * 0.5f;
    f32 laserTop = laserCenter->y - laserSize->y * 0.5f;
    f32 laserRight = laserCenter->x + laserSize->x * 0.5f;
    f32 laserBottom = laserCenter->y + laserSize->y * 0.5f;

    if (PLAYER_BOXES_OVERLAP(playerLeft, playerTop, playerRight, playerBottom, laserLeft, laserTop, laserRight,
                             laserBottom))
    {
        g_PlayerCollisionFlags |= 2;
        if (playerState == 4)
        {
            AdvanceGrazeDisplay(0);
            return 1;
        }
        if (playerState == 0)
        {
            g_GrazeState.BeginPlayerDeath();
            Die();
            return 1;
        }
        return 0;
    }
    if (!canGraze)
    {
        return 0;
    }
    if (!PLAYER_BOXES_OVERLAP(playerLeft, playerTop, playerRight, playerBottom, laserLeft - 48.0f, laserTop - 48.0f,
                              laserRight + 48.0f, laserBottom + 48.0f))
    {
        return 0;
    }
    if (playerState == 2 || playerState == 1)
    {
        return 0;
    }
    ScoreGraze(&positionCenter);
    return 2;
}

i32 Player::ScoreGraze(D3DXVECTOR3 *center)
{
    D3DXVECTOR3 particlePosition;

    if (!g_HideGrazeCounter)
    {
        if (g_GameManager.grazeInStage < 9999)
        {
            ++g_GameManager.grazeInStage;
        }
        if (g_GameManager.grazeInTotal < 999999)
        {
            ++g_GameManager.grazeInTotal;
        }
    }
    particlePosition.x = (positionCenter.x + center->x) * 0.5f;
    particlePosition.y = (positionCenter.y + center->y) * 0.5f;
    particlePosition.z = (positionCenter.z + center->z) * 0.5f;
    g_EffectManager.SpawnParticles(8, &particlePosition, grazeSoundVariant == 1 && grazeVariant == 0 ? 3 : 1,
                                   grazeSoundVariant == 1 && grazeVariant == 0 ? -32640 : -1);
    IncreaseSubrank(6);
    g_GuiFlags = (g_GuiFlags & 0xFFFFFF3F) | 0x80;
    g_SoundPlayer.PlaySoundByIdx(30, 0);
    g_StageScore += 20 * ((g_RankValue - g_GameManager.rankBaseline) / 1500) + 2500;
    g_GameManager.scoreRelated += 200;
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
