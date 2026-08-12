#pragma once

#include "inttypes.hpp"

#include <d3dx8math.h>

namespace th07
{
class Player;

// Observed in the 0x43D9E0 damage-region walk.  The intervening words have
// not yet been assigned engine names.
struct PlayerDamageRegion
{
    f32 centerX;
    f32 centerY;
    f32 unknown08;
    f32 sizeX;
    f32 sizeY;
    f32 unknown14;
    i32 damage;
    i32 accumulatedDamage;
};
C_ASSERT(sizeof(PlayerDamageRegion) == 0x20);

// Observed in the 0x43E0A0 auxiliary-hitbox walk.  A zero sizeX selects the
// circular test, otherwise the first four fields form an AABB.
struct PlayerAuxCollision
{
    f32 centerX;
    f32 centerY;
    f32 sizeX;
    f32 sizeY;
    f32 radius;
    f32 unknown14;
    u32 unknown18;
    u32 collisionValue;
};
C_ASSERT(sizeof(PlayerAuxCollision) == 0x20);

struct PlayerBullet;
typedef i32(__fastcall *PlayerBulletCollisionCallback)(Player *player, PlayerBullet *bullet,
                                                        D3DXVECTOR3 *enemyPosition);

// This is the target-observed subset of the 0x364-byte player-bullet object.
// The prefix remains opaque until its AnmVm layout is recovered by its owner.
struct PlayerBullet
{
    u8 unknown000[0x1D8];
    i16 animationIndex;
    u8 unknown1DA[0x72];
    D3DXVECTOR3 position;
    u8 unknown258[0xC0];
    D3DXVECTOR3 size;
    f32 velocityX;
    f32 velocityY;
    u8 unknown32C[0x18];
    i32 collisionFrame;
    i16 damage;
    i16 state;
    i16 type;
    u8 unknown34E[0xE];
    PlayerBulletCollisionCallback collisionCallback;
    u8 unknown360[4];
};
C_ASSERT(sizeof(PlayerBullet) == 0x364);

// The fields and gaps below are direct target offsets used exclusively by the
// 0x43D9E0--0x43EE4E collision cluster.  Their names are deliberately
// conservative where packet evidence establishes position but not meaning.
class Player
{
  public:
    i32 CalcDamageToEnemy(D3DXVECTOR3 *enemyPosition, D3DXVECTOR3 *enemySize, i32 *bombHit);
    i32 CheckAuxProjectileCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size);
    i32 CalcKillBoxCollision(D3DXVECTOR3 *bulletCenter, D3DXVECTOR3 *bulletSize);
    i32 CheckGraze(D3DXVECTOR3 *center, D3DXVECTOR3 *size);
    i32 CalcItemBoxCollision(D3DXVECTOR3 *itemCenter, D3DXVECTOR3 *itemSize);
    i32 CalcLaserHitbox(D3DXVECTOR3 *laserCenter, D3DXVECTOR3 *laserSize, D3DXVECTOR3 *rotation, f32 angle,
                        i32 canGraze);
    i32 ScoreGraze(D3DXVECTOR3 *center);
    i32 Die();

  private:
    u8 unknown0000[0x930];
    D3DXVECTOR3 positionCenter;             // +0x930, observed in ScoreGraze/Die
    u8 unknown93C[0xC];
    D3DXVECTOR3 killBoxTopLeft;             // +0x948
    D3DXVECTOR3 killBoxBottomRight;         // +0x954
    D3DXVECTOR3 grazeBoxTopLeft;            // +0x960
    D3DXVECTOR3 grazeBoxBottomRight;        // +0x96C
    D3DXVECTOR3 itemBoxTopLeft;             // +0x978
    D3DXVECTOR3 itemBoxBottomRight;         // +0x984
    D3DXVECTOR3 laserHitboxHalfSize;        // +0x990
    u8 unknown99C[0x40];
    PlayerDamageRegion damageRegions[112];  // +0x9DC
    PlayerAuxCollision auxCollisions[96];   // +0x17DC
    u8 unknown23DC[0x28];
    i32 collisionCountdown;                 // +0x2404
    u8 playerState;                         // +0x2408
    u8 unknown2409[2];
    u8 grazeVariant;                        // +0x240B
    u8 collisionParticleCounter;            // +0x240C
    u8 grazeSoundVariant;                   // +0x240D
    u8 unknown240E[0x36];
    PlayerBullet bullets[96];               // +0x2444
    u8 unknown169C4[0x3C];
    i32 lastEnemyHitX;                      // +0x16A00
    i32 lastEnemyHitY;                      // +0x16A04
    i32 lastEnemyHitZ;                      // +0x16A08
    u8 unknown16A0C[0x14];
    i32 bombIsActive;                       // +0x16A20
};
C_ASSERT(sizeof(Player) == 0x16A24);
} // namespace th07
