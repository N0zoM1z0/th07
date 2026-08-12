#pragma once

#include "inttypes.hpp"

namespace th07
{

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
    u8 unknown0[0x1D6];
    i16 spriteIndex;
    u8 unknown1[0xB98 - 0x1D8];
    BulletClearGroup unknownB98ToBA0;
    BulletClearGroup unknownBA4ToBAC;
    i32 unknownBB0;
    i32 unknownBB4;
    i32 unknownBB8;
    u8 unknown2[0xBF8 - 0xBBC];
    i16 despawnState;
    u8 unknown3[2];
    u16 isInUse;
    u8 unknown4[0xD68 - 0xBFE];
};

struct ChainElem
{
    i16 priority;
    u16 flags;
    int (__cdecl *callback)(void *);
    int (__cdecl *addedCallback)(void *);
    int (__cdecl *deletedCallback)(void *);
    ChainElem *prev;
    ChainElem *next;
    ChainElem *unknown;
    void *argument;
};

class BulletManager
{
  public:
    static int __cdecl AddedCallback(BulletManager *manager);
    static int __fastcall DeletedCallback(BulletManager *manager);
    static int __fastcall RegisterChain(char *bulletAnmPath);
    static void __cdecl CutChain();

    void RemoveAllBullets();
    void Initialize();

    static int __cdecl OnUpdate(BulletManager *manager);
    static int __cdecl OnDraw(BulletManager *manager);

    BulletTypeSprites templates[11];
    u8 unknownAfterTemplates[0x33BC];
    Bullet bullets[1024];
    u8 unknownAfterBullets[0x14E80];
    char *bulletAnmPath;
};

} // namespace th07
