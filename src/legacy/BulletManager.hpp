#pragma once

#include "inttypes.hpp"

namespace th07
{

struct BulletVector3
{
    f32 x;
    f32 y;
    f32 z;
};

struct AnmSprite
{
    u8 unknown[44];
    f32 heightPx;
};

struct AnmVm
{
    u8 unknown0[0x1C0];
    u32 flags;
    u8 unknown1[0x10];
    u16 activeSpriteIndex;
    u16 baseSpriteIndex;
    u16 scriptIndex;
    u8 unknown2[0x1E4 - 0x1DC];
    AnmSprite *sprite;
    u8 unknown3[0x24C - 0x1E8];
};

struct BulletTypeSprites
{
    AnmVm bullet;
    AnmVm spawnFast;
    AnmVm spawnNormal;
    AnmVm spawnSlow;
    AnmVm spawnDonut;
    f32 grazeWidth;
    f32 grazeHeight;
    u8 unknown0[5];
    u8 bulletHeight;
    u8 grazeKind;
    u8 unknown1;
};

struct BulletClearGroup
{
    i32 first;
    i32 second;
    i32 third;
};

struct Bullet
{
    AnmVm animations[5];
    u8 unknownB7C[0x10];
    BulletVector3 position;
    union
    {
        BulletVector3 velocity;
        BulletClearGroup unknownB98ToBA0;
    };
    union
    {
        BulletVector3 acceleration;
        BulletClearGroup unknownBA4ToBAC;
    };
    i32 unknownBB0;
    i32 unknownBB4;
    i32 unknownBB8;
    f32 angle;
    u8 unknownBC0[0x34];
    u16 movementFlags;
    u16 spawnFlags;
    i16 despawnState;
    u16 unknownBFA;
    u16 state;
    u8 unknownBFE[2];
    u8 grazeState;
    u8 grazeKind;
    u8 unknownC02[2];
    Bullet *nextInDrawGroup;
    u8 unknownC08[0x160];
};
typedef char BulletSizeCheck[sizeof(Bullet) == 0xD68 ? 1 : -1];

struct Laser
{
    u8 unknown00[0x4EC];
};
typedef char LaserSizeCheck[sizeof(Laser) == 0x4EC ? 1 : -1];

struct ChainElem
{
    i16 priority;
    u16 flags;
    int (__fastcall *callback)(void *);
    int (__fastcall *addedCallback)(void *);
    int (__fastcall *deletedCallback)(void *);
    ChainElem *prev;
    ChainElem *next;
    ChainElem *unknown;
    void *argument;
};

class BulletManager
{
  public:
    static int __fastcall AddedCallback(BulletManager *manager);
    static int __fastcall DeletedCallback(BulletManager *manager);
    static int __fastcall RegisterChain(char *bulletAnmPath);
    static void __cdecl CutChain();

    void RemoveAllBullets();
    void Initialize();

    static int __fastcall OnUpdate(BulletManager *manager);
    static int __fastcall OnDraw(BulletManager *manager);

    BulletTypeSprites templates[16];
    Bullet bullets[1025];
    Laser lasers[64];
    i32 activeBulletCount;
    i32 clearCountdown;
    u32 timers[3];
    u32 unknown37A13C;
    char *bulletAnmPath;
    Bullet *drawGroupHeads[6];
    Bullet *nextBullet;
    u32 nextBulletState;
};
typedef char BulletManagerSizeCheck[sizeof(BulletManager) == 0x37A164 ? 1 : -1];

} // namespace th07
