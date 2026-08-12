#include "BulletManager.hpp"

#include <string.h>

namespace th07
{

struct AnmManager
{
    u8 unknown[0x28EF0];
    void *scripts[1];

    void ReleaseAnm(int fileIndex);
    int LoadAnm(int fileIndex, const char *path, int spriteIndexOffset);
    void ResetBulletAnimation(Bullet *bullet, int spriteIndex);
    void SetAndExecuteScript(AnmVm *vm, void *script);
};

struct Chain
{
    int AddToCalcChain(ChainElem *chain, int priority);
    void AddToDrawChain(ChainElem *chain, int priority);
    void Cut(ChainElem *chain);
};

extern int g_SupervisorState;
extern AnmManager *g_AnmManager;
extern u32 g_BulletTemplateScriptIds[];
extern u8 g_ItemManager[0xAE57C];

extern BulletManager g_BulletManager;
extern ChainElem g_BulletManagerCalcChain;
extern ChainElem g_BulletManagerDrawChain;
extern Chain g_Chain;
extern u8 g_BulletManagerStorage[0x37A164];

extern void *g_EffectsColor;
extern u8 g_EffectsColorTable[];

struct BulletTemplateScriptIds
{
    u32 bullet;
    u32 spawnFast;
    u32 spawnNormal;
    u32 spawnSlow;
    u32 spawnDonut;
};

#pragma var_order(i, scriptForBullet, anmVmForBullet, anmManagerForBullet, scriptForFast, anmVmForFast, \
                  anmManagerForFast, scriptForNormal, anmVmForNormal, anmManagerForNormal, scriptForSlow, \
                  anmVmForSlow, anmManagerForSlow, scriptForDonut, anmVmForDonut, anmManagerForDonut)
int __fastcall BulletManager::AddedCallback(BulletManager *manager)
{
    AnmVm *anmVmForDonut;
    AnmManager *anmManagerForDonut;
    int scriptForDonut;
    AnmVm *anmVmForSlow;
    AnmManager *anmManagerForSlow;
    int scriptForSlow;
    AnmVm *anmVmForNormal;
    AnmManager *anmManagerForNormal;
    int scriptForNormal;
    AnmVm *anmVmForFast;
    AnmManager *anmManagerForFast;
    int scriptForFast;
    AnmVm *anmVmForBullet;
    AnmManager *anmManagerForBullet;
    int scriptForBullet;
    u32 i;

    if ((int)(g_SupervisorState != 3 && g_SupervisorState != 11 && g_SupervisorState != 12))
    {
        if (g_AnmManager->LoadAnm(11, "data/etama.anm", 512))
        {
            return -1;
        }
    }

    for (i = 0; i < 11; i++)
    {
        scriptForBullet = reinterpret_cast<BulletTemplateScriptIds *>(g_BulletTemplateScriptIds)[i].bullet;
        anmVmForBullet = &manager->templates[i].bullet;
        anmManagerForBullet = g_AnmManager;
        anmVmForBullet->scriptIndex = (u16)scriptForBullet;
        anmManagerForBullet->SetAndExecuteScript(anmVmForBullet,
                                                  anmManagerForBullet->scripts[scriptForBullet]);
        scriptForFast = reinterpret_cast<BulletTemplateScriptIds *>(g_BulletTemplateScriptIds)[i].spawnFast;
        anmVmForFast = &manager->templates[i].spawnFast;
        anmManagerForFast = g_AnmManager;
        anmVmForFast->scriptIndex = (u16)scriptForFast;
        anmManagerForFast->SetAndExecuteScript(anmVmForFast, anmManagerForFast->scripts[scriptForFast]);
        scriptForNormal = reinterpret_cast<BulletTemplateScriptIds *>(g_BulletTemplateScriptIds)[i].spawnNormal;
        anmVmForNormal = &manager->templates[i].spawnNormal;
        anmManagerForNormal = g_AnmManager;
        anmVmForNormal->scriptIndex = (u16)scriptForNormal;
        anmManagerForNormal->SetAndExecuteScript(anmVmForNormal,
                                                  anmManagerForNormal->scripts[scriptForNormal]);
        scriptForSlow = reinterpret_cast<BulletTemplateScriptIds *>(g_BulletTemplateScriptIds)[i].spawnSlow;
        anmVmForSlow = &manager->templates[i].spawnSlow;
        anmManagerForSlow = g_AnmManager;
        anmVmForSlow->scriptIndex = (u16)scriptForSlow;
        anmManagerForSlow->SetAndExecuteScript(anmVmForSlow, anmManagerForSlow->scripts[scriptForSlow]);
        scriptForDonut = reinterpret_cast<BulletTemplateScriptIds *>(g_BulletTemplateScriptIds)[i].spawnDonut;
        anmVmForDonut = &manager->templates[i].spawnDonut;
        anmManagerForDonut = g_AnmManager;
        anmVmForDonut->scriptIndex = (u16)scriptForDonut;
        anmManagerForDonut->SetAndExecuteScript(anmVmForDonut, anmManagerForDonut->scripts[scriptForDonut]);

        manager->templates[i].bullet.flags |= 0x1000;
        manager->templates[i].spawnFast.flags |= 0x1000;
        manager->templates[i].spawnNormal.flags |= 0x1000;
        manager->templates[i].spawnSlow.flags |= 0x1000;
        manager->templates[i].spawnDonut.flags |= 0x1000;
        manager->templates[i].bullet.baseSpriteIndex = manager->templates[i].bullet.activeSpriteIndex;
        manager->templates[i].bulletHeight = (u8)manager->templates[i].bullet.sprite->heightPx;

        if (manager->templates[i].bullet.sprite->heightPx <= 8.0f)
        {
            manager->templates[i].grazeWidth = 4.0f;
            manager->templates[i].grazeHeight = 4.0f;
            manager->templates[i].grazeKind = 5;
        }
        else if (manager->templates[i].bullet.sprite->heightPx <= 16.0f)
        {
            int scriptForSmallBullet;
            scriptForSmallBullet = reinterpret_cast<BulletTemplateScriptIds *>(g_BulletTemplateScriptIds)[i].bullet;
            switch (scriptForSmallBullet)
            {
            case 514:
                manager->templates[i].grazeWidth = 4.0f;
                manager->templates[i].grazeHeight = 4.0f;
                manager->templates[i].grazeKind = 4;
                break;
            case 516:
            case 518:
                manager->templates[i].grazeWidth = 4.0f;
                manager->templates[i].grazeHeight = 4.0f;
                manager->templates[i].grazeKind = 4;
                break;
            case 517:
                manager->templates[i].grazeWidth = 4.0f;
                manager->templates[i].grazeHeight = 4.0f;
                manager->templates[i].grazeKind = 4;
                break;
            default:
                manager->templates[i].grazeWidth = 6.0f;
                manager->templates[i].grazeHeight = 6.0f;
                manager->templates[i].grazeKind = 3;
                break;
            }
        }
        else if (manager->templates[i].bullet.sprite->heightPx <= 32.0f)
        {
            int scriptForLargeBullet;

            scriptForLargeBullet = reinterpret_cast<BulletTemplateScriptIds *>(g_BulletTemplateScriptIds)[i].bullet;
            if (scriptForLargeBullet == 520)
            {
                manager->templates[i].grazeWidth = 5.0f;
                manager->templates[i].grazeHeight = 5.0f;
                manager->templates[i].grazeKind = 1;
            }
            else
            {
                if (scriptForLargeBullet == 521)
                {
                    manager->templates[i].grazeWidth = 8.0f;
                    manager->templates[i].grazeHeight = 8.0f;
                }
                else
                {
                    manager->templates[i].grazeWidth = 10.0f;
                    manager->templates[i].grazeHeight = 10.0f;
                }
                manager->templates[i].grazeKind = 2;
            }
        }
        else
        {
            manager->templates[i].grazeKind = 0;
            manager->templates[i].grazeWidth = 24.0f;
            manager->templates[i].grazeHeight = 24.0f;
        }
    }

    memset(g_ItemManager, 0, sizeof(g_ItemManager));
    return 0;
}

int __fastcall BulletManager::DeletedCallback(BulletManager *manager)
{
    if ((int)(g_SupervisorState != 3 && g_SupervisorState != 11 && g_SupervisorState != 12))
    {
        g_AnmManager->ReleaseAnm(11);
        g_AnmManager->ReleaseAnm(12);
        g_AnmManager->ReleaseAnm(13);
        g_AnmManager->ReleaseAnm(14);
    }

    return 0;
}

int __fastcall BulletManager::RegisterChain(char *bulletAnmPath)
{
    BulletManager *manager;

    manager = &g_BulletManager;
    g_EffectsColor = g_EffectsColorTable;
    manager->Initialize();
    manager->bulletAnmPath = bulletAnmPath;

    g_BulletManagerCalcChain.callback = (int (__fastcall *)(void *))OnUpdate;
    g_BulletManagerCalcChain.addedCallback = 0;
    g_BulletManagerCalcChain.deletedCallback = 0;
    g_BulletManagerCalcChain.addedCallback = (int (__fastcall *)(void *))AddedCallback;
    g_BulletManagerCalcChain.deletedCallback = (int (__fastcall *)(void *))DeletedCallback;
    g_BulletManagerCalcChain.argument = manager;
    if (g_Chain.AddToCalcChain(&g_BulletManagerCalcChain, 12))
    {
        return -1;
    }

    g_BulletManagerDrawChain.callback = (int (__fastcall *)(void *))OnDraw;
    g_BulletManagerDrawChain.addedCallback = 0;
    g_BulletManagerDrawChain.deletedCallback = 0;
    g_BulletManagerDrawChain.argument = manager;
    g_Chain.AddToDrawChain(&g_BulletManagerDrawChain, 10);
    return 0;
}

void __cdecl BulletManager::CutChain()
{
    g_Chain.Cut(&g_BulletManagerCalcChain);
    g_Chain.Cut(&g_BulletManagerDrawChain);
    memset(g_BulletManagerStorage, 0, sizeof(g_BulletManagerStorage));
}

void BulletManager::RemoveAllBullets()
{
    Bullet *bullet;
    int i;

    for (bullet = g_BulletManager.bullets, i = 0; i < 1024; i++, bullet++)
    {
        BulletClearGroup firstClearGroup;
        BulletClearGroup secondClearGroup;

        if (!bullet->state)
        {
            continue;
        }

        firstClearGroup.first = 0;
        firstClearGroup.second = 0;
        firstClearGroup.third = 0;
        bullet->unknownB98ToBA0 = firstClearGroup;

        secondClearGroup.first = 0;
        secondClearGroup.second = 0;
        secondClearGroup.third = 0;
        bullet->unknownBA4ToBAC = secondClearGroup;
        bullet->unknownBB8 = 0;
        bullet->unknownBB4 = 0;
        bullet->unknownBB0 = 0;
        bullet->despawnState = 0;
        g_AnmManager->ResetBulletAnimation(
            bullet, (i16)bullet->animations[0].baseSpriteIndex + bullet->despawnState);
    }
}

void BulletManager::Initialize()
{
    memset(this, 0, sizeof(*this));
    nextBullet = bullets;
    bullets[1024].state = 6;
    nextBulletState = 6;
}

} // namespace th07
