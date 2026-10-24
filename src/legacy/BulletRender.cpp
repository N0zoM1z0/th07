#include "BulletManager.hpp"

namespace th07
{

// These layouts are intentionally private to the renderer.  They record only
// offsets touched by 0x426B00 and 0x426C40; BulletManager.hpp remains owned by
// the main bullet-system lane.
struct BulletRenderVec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct BulletRenderAnmVm
{
    u8 unknown00[8];
    f32 rotationZ;                    // +0x008
    u8 unknown0C[0x0C];
    f32 scaleX;                       // +0x018
    f32 scaleY;                       // +0x01C
    u8 unknown20[0x198];
    u32 color;                        // +0x1B8
    u8 unknown1BC[4];
    u32 flags;                        // +0x1C0
    i16 activeSpriteIndex;            // +0x1C4
    u8 unknown1C6[2];
    BulletRenderVec3 position;        // +0x1C8
    u8 unknown1D4[0x24C - 0x1D4];
};
typedef char BulletRenderAnmVm_size[(sizeof(BulletRenderAnmVm) == 0x24C) ? 1 : -1];

struct BulletRenderBullet
{
    BulletRenderAnmVm animation[5];
    u8 unknownB7C[0x10];
    BulletRenderVec3 position;        // +0xB8C
    BulletRenderVec3 velocity;        // +0xB98
    u8 unknownBA4[0x18];
    f32 angle;                        // +0xBBC
    u8 unknownBC0[0x3C];
    u16 state;                        // +0xBFC
    u8 unknownBFE[6];
    BulletRenderBullet *drawNext;     // +0xC04
    u8 unknownC08[0xD68 - 0xC08];
};
typedef char BulletRenderBullet_size[(sizeof(BulletRenderBullet) == 0xD68) ? 1 : -1];

struct BulletRenderLaser
{
    BulletRenderAnmVm primaryAnimation;
    BulletRenderAnmVm secondaryAnimation;
    BulletRenderVec3 position;        // +0x498
    f32 angle;                        // +0x4A4
    f32 startOffset;                  // +0x4A8
    f32 endOffset;                    // +0x4AC
    u8 unknown4B0[4];
    f32 width;                        // +0x4B4
    u8 unknown4B8[4];
    f32 speed;                        // +0x4BC
    u8 unknown4C0[0x14];
    i32 isInUse;                      // +0x4D4
    u8 unknown4D8[0xE];
    i16 color;                        // +0x4E6
    u8 despawnState;                  // +0x4E8
    u8 drawSecondary;                 // +0x4E9
    u8 unknown4EA[0x4EC - 0x4EA];
};
typedef char BulletRenderLaser_size[(sizeof(BulletRenderLaser) == 0x4EC) ? 1 : -1];

// 0x417AF0: fsincos stores sine through ECX and cosine through EDX.
void __fastcall CalculateSinCos(f32 *sine, f32 *cosine, f32 angle)
{
    __asm
    {
        fld angle
        fsincos
        mov eax, cosine
        fstp DWORD PTR[eax]
        mov eax, sine
        fstp DWORD PTR[eax]
    }
}
extern f32 __stdcall AddNormalizeAngle(f32 first, f32 second);

struct BulletRenderAnmManager
{
    void Draw3(AnmVm *vm);
};
extern BulletRenderAnmManager *g_AnmManager;

struct BulletRenderItemManager
{
    void OnDraw();
};
extern BulletRenderItemManager g_ItemManager;

extern f32 g_RenderOffsetX;
extern f32 g_RenderOffsetY;

#pragma var_order(animation, normalizedAngle)
static void __fastcall DrawSelectedBulletAnimation(BulletRenderBullet *bullet)
{
    BulletRenderAnmVm *animation;
    f32 normalizedAngle;

    switch (bullet->state)
    {
    case 2:
        animation = &bullet->animation[1];
        break;
    case 3:
        animation = &bullet->animation[2];
        break;
    case 4:
        animation = &bullet->animation[3];
        break;
    case 5:
        animation = &bullet->animation[4];
        break;
    default:
        animation = &bullet->animation[0];
        break;
    }

    animation->position.x = bullet->position.x + g_RenderOffsetX;
    animation->position.y = bullet->position.y + g_RenderOffsetY;
    animation->position.z = 0.05f;
    animation->color = (animation->color & 0xFF000000) | 0x00FFFFFF;
    if (animation->activeSpriteIndex)
    {
        normalizedAngle = AddNormalizeAngle(1.5707964f + bullet->angle, 0.0f);
        animation->rotationZ = normalizedAngle;
        animation->flags |= 4;
    }

    g_AnmManager->Draw3(reinterpret_cast<AnmVm *>(animation));
}

#pragma var_order(i, sine, laser, centerOffset, cosine, bullet)
int __fastcall BulletManager::OnDraw(BulletManager *manager)
{
    BulletRenderLaser *laser;
    i32 i;
    f32 sine;
    f32 cosine;
    f32 centerOffset;
    BulletRenderBullet *bullet;

    laser = reinterpret_cast<BulletRenderLaser *>(reinterpret_cast<u8 *>(manager) + 0x366628);
    for (i = 0; i < 64; i++, laser++)
    {
        if (!laser->isInUse)
        {
            continue;
        }

        CalculateSinCos(&sine, &cosine, laser->angle);
        centerOffset = (laser->endOffset - laser->startOffset) / 2.0f + laser->startOffset;
        laser->primaryAnimation.position.x = cosine * centerOffset + laser->position.x;
        laser->primaryAnimation.position.y = sine * centerOffset + laser->position.y;
        laser->primaryAnimation.position.z = 0.05f;
        laser->color = (laser->color & 0xFF000000) | 0x00FFFFFF;
        laser->primaryAnimation.position.x += g_RenderOffsetX;
        laser->primaryAnimation.position.y += g_RenderOffsetY;
        g_AnmManager->Draw3(reinterpret_cast<AnmVm *>(&laser->primaryAnimation));

        if ((laser->startOffset < 16.0f || laser->speed == 0.0f) &&
            (!laser->drawSecondary || laser->despawnState))
        {
            laser->secondaryAnimation.position.x = cosine * laser->startOffset + laser->position.x;
            laser->secondaryAnimation.position.y = sine * laser->startOffset + laser->position.y;
            laser->secondaryAnimation.position.z = 0.05f;
            laser->secondaryAnimation.color = laser->primaryAnimation.color;
            laser->secondaryAnimation.flags |= 0x20;
            laser->secondaryAnimation.color = (laser->secondaryAnimation.color & 0x00FFFFFF) | 0xFF000000;
            laser->secondaryAnimation.scaleX =
                laser->width / 10.0f * ((16.0f - laser->startOffset) / 16.0f);
            laser->secondaryAnimation.scaleY = laser->secondaryAnimation.scaleX;
            if (laser->secondaryAnimation.scaleY <= 0.0f)
            {
                laser->secondaryAnimation.scaleX = laser->width / 10.0f;
                laser->secondaryAnimation.scaleY = laser->secondaryAnimation.scaleX;
            }
            laser->secondaryAnimation.position.x += g_RenderOffsetX;
            laser->secondaryAnimation.position.y += g_RenderOffsetY;
            g_AnmManager->Draw3(reinterpret_cast<AnmVm *>(&laser->secondaryAnimation));
        }
    }

    g_ItemManager.OnDraw();

    for (i = 0; i < 6; i++)
    {
        bullet = *reinterpret_cast<BulletRenderBullet **>(
            reinterpret_cast<u8 *>(manager) + 0x37A144 + sizeof(BulletRenderBullet *) * i);
        while (bullet)
        {
            DrawSelectedBulletAnimation(bullet);
            bullet = bullet->drawNext;
        }
    }

    return 1;
}

} // namespace th07
