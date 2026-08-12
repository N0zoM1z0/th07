#include "inttypes.hpp"

namespace th07
{

struct EnemySpawnVec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct EnemySpawnOverlay
{
    u8 unknown0000[0x1B8];
    u32 primaryColor;
    u8 unknown01BC[0x528];
    u8 eclContext[0x2428];
    EnemySpawnVec3 position;
    u8 unknown2B18[0xA0];
    i32 life;
    i32 maxLife;
    i32 score;
    u8 unknown2BC4[0xC];
    u32 color;
    u8 unknown2BD4[0x23C];
    i32 itemDrop;
    u8 unknown2E14[0x14];
    u8 unknownFlagBits : 6;
    u8 mirrored : 1;
    u8 isSlotOccupied : 1;
    u8 unknown2E29[0x211F];
};

struct EclManagerSpawnOverlay
{
    void CallEclSub(void *context, i16 subId);
    i32 RunEcl(EnemySpawnOverlay *enemy);
};

struct EnemyManagerSpawnOverlay
{
    u8 unknown0000[8];
    EnemySpawnOverlay enemyTemplate;
    EnemySpawnOverlay enemies[480];

    EnemySpawnOverlay *SpawnEnemy(i16 eclSubId, EnemySpawnVec3 *position,
                                  i32 life, i8 itemDrop, i32 score, i8 mirrored);
};

extern EclManagerSpawnOverlay g_EclManagerSpawn;

#pragma var_order(index, enemy, this)
EnemySpawnOverlay *EnemyManagerSpawnOverlay::SpawnEnemy(i16 eclSubId,
                                                        EnemySpawnVec3 *position,
                                                        i32 life, i8 itemDrop,
                                                        i32 score, i8 mirrored)
{
    EnemySpawnOverlay *enemy;
    i32 index;

    enemy = enemies;
    for (index = 0; index < 480; ++index, ++enemy)
    {
        if (enemy->isSlotOccupied)
            continue;

        *enemy = enemyTemplate;
        enemy->mirrored = mirrored;
        if (life >= 0)
            enemy->life = life;
        enemy->position = *position;
        g_EclManagerSpawn.CallEclSub(enemy->eclContext, eclSubId);
        if (g_EclManagerSpawn.RunEcl(enemy) == -1)
        {
            enemy->isSlotOccupied = 0;
            break;
        }

        enemy->color = enemy->primaryColor;
        enemy->itemDrop = itemDrop;
        if (score >= 0)
            enemy->score = score;
        enemy->maxLife = enemy->life;
        break;
    }
    return enemy;
}

} // namespace th07
