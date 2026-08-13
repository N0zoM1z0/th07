#include "inttypes.hpp"

#include <stddef.h>

namespace th07
{

// This translation unit intentionally keeps its spawn-only view private.  The
// offsets below are direct observations from 0x00423730; they must not be
// promoted into BulletManager.hpp until all users agree on the complete ABI.
struct BulletSpawnVec2
{
    f32 x;
    f32 y;

    void VectorFromAngle(f32 angle, f32 magnitude);
};

struct BulletSpawnVec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct BulletSpawnTimer
{
    i32 previous;
    f32 subFrame;
    i32 current;
};
typedef char BulletSpawnTimer_size[(sizeof(BulletSpawnTimer) == 0xC) ? 1 : -1];

struct BulletSpawnVm
{
    u8 unknown00[0x1D4];
    i16 activeSpriteIndex;
    i16 baseSpriteIndex;
    i16 scriptIndex;
    u8 unknown1DA[6];
    i32 forceCopy;
    void *sprite;
    u8 unknown1E8[0x24C - 0x1E8];
};
typedef char BulletSpawnVm_size[(sizeof(BulletSpawnVm) == 0x24C) ? 1 : -1];

struct BulletSpawnCommandList
{
    u32 words[0x1E];
};
typedef char BulletSpawnCommandList_size[(sizeof(BulletSpawnCommandList) == 0x78) ? 1 : -1];

struct BulletSpawnSlot
{
    BulletSpawnVm animation[5];
    BulletSpawnVec3 grazeSize;
    u8 drawListIndex;
    u8 unknownB889;
    u8 grazeKind;
    u8 unknownB88B;
    BulletSpawnVec3 position;
    BulletSpawnVec3 velocity;
    u8 unknownBA4[0xC];
    f32 speed;
    u8 unknownBB4[8];
    f32 angle;
    u8 unknownBC0[8];
    BulletSpawnTimer lifetimeTimer;
    BulletSpawnTimer collisionTimer;
    u8 unknownBE0[0x10];
    i32 skipBoundsCountdown;
    u16 movementFlags;
    u16 collisionFlags;
    i16 baseSpriteIndex;
    u16 unknownBFA;
    u16 state;
    u16 offscreenCounter;
    u8 isActive;
    u8 grazeState;
    u8 unknownC02[2];
    BulletSpawnSlot *drawNext;
    i32 unknownC08;
    i32 commandOwner;
    i32 commandIndex;
    BulletSpawnCommandList spawnCommands;
    u8 unknownC8C[0xD68 - 0xC8C];

    void InitializeSpawnCommands();
};
typedef char BulletSpawnSlot_size[(sizeof(BulletSpawnSlot) == 0xD68) ? 1 : -1];

// The public wrapper at 0x424D20 populates templateSource before it calls
// SpawnOne.  The bytes after commandList are intentionally opaque: SpawnOne
// only copies the first 0x78 bytes beginning at +0x20.
struct BulletSpawnRequest
{
    i16 templateIndex;
    i16 baseSpriteIndex;
    BulletSpawnVec3 position;
    f32 angleOffset;
    f32 angleStep;
    f32 speedStart;
    f32 speedEnd;
    BulletSpawnCommandList commandList;
    u8 unknown98[0x24];
    i16 columns;
    i16 rows;
    u16 layoutMode;
    u16 unknownC2;
    u32 flags;
    i32 soundId;
    i32 commandOwner;
    BulletSpawnSlot *templateSource;
};
typedef char BulletSpawnRequest_template_offset[(offsetof(BulletSpawnRequest, templateSource) == 0xD0) ? 1 : -1];

struct BulletSpawnRng
{
    f32 RandomF32();
};

struct BulletSpawnAnmManager
{
    void SetSprite(BulletSpawnVm *vm, i32 spriteIndex);
};

extern BulletSpawnRng g_BulletSpawnRng;
extern BulletSpawnAnmManager *g_BulletSpawnAnmManager;
extern f32 g_BulletSpawnFrameMultiplier;

extern f32 __stdcall AddNormalizeAngle(f32 angle, f32 delta);
extern void __fastcall CopyVm(BulletSpawnVm *destination, BulletSpawnVm *source, i32 owner, i32 baseSpriteIndex);

class BulletSpawnManager
{
  public:
    i32 SpawnOne(BulletSpawnRequest *request, i32 column, i32 row, f32 baseAngle);
};

#pragma var_order(slot, i, speed, angle, source, sourceAuxiliary, destinationAuxiliary)
i32 BulletSpawnManager::SpawnOne(BulletSpawnRequest *request, i32 column, i32 row, f32 baseAngle)
{
    BulletSpawnSlot *slot;
    BulletSpawnSlot *source;
    BulletSpawnVm *sourceAuxiliary;
    BulletSpawnVm *destinationAuxiliary;
    f32 speed;
    f32 angle;
    i32 i;

    slot = *(BulletSpawnSlot **)(reinterpret_cast<u8 *>(this) + 0x37A15C);
    for (i = 0; i < 1024; i++)
    {
        if (slot->state == 0)
        {
            break;
        }

        slot++;
        if (slot->state == 6)
        {
            slot = reinterpret_cast<BulletSpawnSlot *>(reinterpret_cast<u8 *>(this) + 0xB8C0);
        }
    }

    if (i >= 1024)
    {
        return 1;
    }

    if (request->rows > 1)
    {
        speed = request->speedStart -
                (request->speedStart - request->speedEnd) * row / request->rows;
    }
    else
    {
        speed = request->speedStart;
    }

    angle = 0.0f;
    switch (request->layoutMode)
    {
    case 0:
    case 1:
        if (request->columns & 1)
        {
            angle = ((column + 1) / 2) * request->angleStep;
        }
        else
        {
            angle = (column / 2) * request->angleStep + request->angleStep * 0.5f;
        }
        if (column & 1)
        {
            angle *= -1.0f;
        }
        if (request->layoutMode == 0)
        {
            angle += baseAngle;
        }
        angle += request->angleOffset;
        break;

    case 2:
        angle += request->angleStep;
        angle += column * 6.2831855f / request->columns;
        angle += row * request->angleStep + request->angleOffset;
        break;

    case 3:
        angle += column * 6.2831855f / request->columns;
        angle += row * request->angleStep + request->angleOffset;
        break;

    case 4:
        angle += request->angleStep;
        angle += 3.1415927f / request->columns;
        angle += column * 6.2831855f / request->columns;
        angle += request->angleOffset;
        break;

    case 5:
        angle += 3.1415927f / request->columns;
        angle += column * 6.2831855f / request->columns;
        angle += request->angleOffset;
        break;

    case 6:
        angle = g_BulletSpawnRng.RandomF32() * (request->angleOffset - request->angleStep) + request->angleStep;
        break;

    case 7:
        speed = g_BulletSpawnRng.RandomF32() * (request->speedStart - request->speedEnd) + request->speedEnd;
        angle += column * 6.2831855f / request->columns;
        angle += row * request->angleStep + request->angleOffset;
        break;

    case 8:
        angle = g_BulletSpawnRng.RandomF32() * (request->angleOffset - request->angleStep) + request->angleStep;
        speed = g_BulletSpawnRng.RandomF32() * (request->speedStart - request->speedEnd) + request->speedEnd;
        break;
    }

    slot->state = 1;
    slot->isActive = 1;
    slot->grazeState = 0;
    slot->lifetimeTimer.previous = -999;
    slot->lifetimeTimer.subFrame = 0.0f;
    slot->lifetimeTimer.current = 0;
    slot->collisionTimer.previous = -999;
    slot->collisionTimer.subFrame = 0.0f;
    slot->collisionTimer.current = 0;
    slot->speed = speed;
    slot->angle = AddNormalizeAngle(angle, 0.0f);
    slot->position = request->position;
    slot->position.z = 0.1f;
    reinterpret_cast<BulletSpawnVec2 *>(&slot->velocity)->VectorFromAngle(angle,
                                                                            speed * g_BulletSpawnFrameMultiplier);
    slot->movementFlags = request->flags;
    slot->baseSpriteIndex = request->baseSpriteIndex;
    slot->unknownC08 = 0;

    source = request->templateSource;
    if (slot->animation[0].scriptIndex != source->animation[0].scriptIndex || source->animation[0].forceCopy)
    {
        slot->animation[0] = source->animation[0];
    }
    if (slot->animation[4].scriptIndex != source->animation[4].scriptIndex || source->animation[4].forceCopy)
    {
        slot->animation[4] = source->animation[4];
    }
    slot->grazeSize = source->grazeSize;
    slot->drawListIndex = source->drawListIndex;
    slot->unknownB889 = source->unknownB889;
    slot->grazeKind = source->grazeKind;
    slot->commandOwner = request->commandOwner;
    slot->skipBoundsCountdown = 0;

    if (slot->animation[0].activeSpriteIndex != source->animation[0].activeSpriteIndex + request->baseSpriteIndex)
    {
        g_BulletSpawnAnmManager->SetSprite(&slot->animation[0],
                                           source->animation[0].activeSpriteIndex + request->baseSpriteIndex);
    }
    if (slot->animation[4].activeSpriteIndex != source->animation[4].activeSpriteIndex + request->baseSpriteIndex)
    {
        g_BulletSpawnAnmManager->SetSprite(&slot->animation[4],
                                           source->animation[4].activeSpriteIndex + request->baseSpriteIndex);
    }

    if (request->flags & 2)
    {
        sourceAuxiliary = &source->animation[1];
        destinationAuxiliary = &slot->animation[1];
        if (destinationAuxiliary->scriptIndex != sourceAuxiliary->scriptIndex || sourceAuxiliary->forceCopy)
        {
            *destinationAuxiliary = *sourceAuxiliary;
        }
        CopyVm(destinationAuxiliary, sourceAuxiliary, reinterpret_cast<i32>(slot), request->baseSpriteIndex);
        slot->state = 2;
        {
            BulletSpawnVec3 spawnFastOffset;

            spawnFastOffset.x = slot->velocity.x * 4.0f;
            spawnFastOffset.y = slot->velocity.y * 4.0f;
            spawnFastOffset.z = slot->velocity.z * 4.0f;
            slot->position.x -= spawnFastOffset.x;
            slot->position.y -= spawnFastOffset.y;
            slot->position.z -= spawnFastOffset.z;
        }
    }
    else if (request->flags & 4)
    {
        sourceAuxiliary = &source->animation[2];
        destinationAuxiliary = &slot->animation[2];
        if (destinationAuxiliary->scriptIndex != sourceAuxiliary->scriptIndex || sourceAuxiliary->forceCopy)
        {
            *destinationAuxiliary = *sourceAuxiliary;
        }
        CopyVm(destinationAuxiliary, sourceAuxiliary, reinterpret_cast<i32>(slot), request->baseSpriteIndex);
        slot->state = 3;
        {
            BulletSpawnVec3 spawnNormalOffset;

            spawnNormalOffset.x = slot->velocity.x * 4.0f;
            spawnNormalOffset.y = slot->velocity.y * 4.0f;
            spawnNormalOffset.z = slot->velocity.z * 4.0f;
            slot->position.x -= spawnNormalOffset.x;
            slot->position.y -= spawnNormalOffset.y;
            slot->position.z -= spawnNormalOffset.z;
        }
    }
    else if (request->flags & 8)
    {
        sourceAuxiliary = &source->animation[3];
        destinationAuxiliary = &slot->animation[3];
        if (destinationAuxiliary->scriptIndex != sourceAuxiliary->scriptIndex || sourceAuxiliary->forceCopy)
        {
            *destinationAuxiliary = *sourceAuxiliary;
        }
        CopyVm(destinationAuxiliary, sourceAuxiliary, reinterpret_cast<i32>(slot), request->baseSpriteIndex);
        slot->state = 4;
        {
            BulletSpawnVec3 spawnSlowOffset;

            spawnSlowOffset.x = slot->velocity.x * 4.0f;
            spawnSlowOffset.y = slot->velocity.y * 4.0f;
            spawnSlowOffset.z = slot->velocity.z * 4.0f;
            slot->position.x -= spawnSlowOffset.x;
            slot->position.y -= spawnSlowOffset.y;
            slot->position.z -= spawnSlowOffset.z;
        }
    }

    slot->spawnCommands = request->commandList;
    slot->collisionFlags = request->flags;
    slot->movementFlags = 0;
    slot->commandIndex = 0;
    slot->InitializeSpawnCommands();

    if (*(i32 *)(reinterpret_cast<u8 *>(this) + 0x37A12C) && !(slot->collisionFlags & 0x1000))
    {
        slot->state = 5;
    }

    slot++;
    if (slot->state == 6)
    {
        slot = reinterpret_cast<BulletSpawnSlot *>(reinterpret_cast<u8 *>(this) + 0xB8C0);
    }
    *(BulletSpawnSlot **)(reinterpret_cast<u8 *>(this) + 0x37A15C) = slot;
    return 0;
}

} // namespace th07
