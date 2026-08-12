#include "BulletManager.hpp"

#include <string.h>

namespace th07
{

struct AnmManager
{
    u8 unknown[0x28EF0];
    void *scripts[1];

    void ReleaseAnm(int fileIndex);
    void ResetBulletAnimation(Bullet *bullet, int spriteIndex);
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

extern int __cdecl LoadAnm(int fileIndex, const char *path, int spriteIndexOffset);
extern void __cdecl SetAndExecuteScript(AnmVm *vm, void *script);

int __fastcall BulletManager::AddedCallback(BulletManager *inputManager)
{
    BulletManager *manager;
    int scriptForLargeBullet;
    int shouldLoadAnm;
    int anmManagerForDonut;
    int scriptForDonut;
    int anmManagerForSlow;
    int scriptForSlow;
    int anmManagerForNormal;
    int scriptForNormal;
    int anmManagerForFast;
    int scriptForFast;
    int anmManagerForBullet;
    int scriptForBullet;
    u32 i;

    manager = inputManager;
    shouldLoadAnm = g_SupervisorState != 3 && g_SupervisorState != 11 && g_SupervisorState != 12;
    if (shouldLoadAnm && LoadAnm(11, "data/etama.anm", 512))
    {
        return -1;
    }

    for (i = 0; i < 11; i++)
    {
        scriptForBullet = g_BulletTemplateScriptIds[5 * i];
        anmManagerForBullet = (int)g_AnmManager;
        manager->templates[i].bullet.scriptIndex = (u16)scriptForBullet;
        SetAndExecuteScript(&manager->templates[i].bullet,
                            *(void **)(anmManagerForBullet + 4 * scriptForBullet + 0x28EF0));
        scriptForFast = g_BulletTemplateScriptIds[5 * i + 1];
        anmManagerForFast = (int)g_AnmManager;
        manager->templates[i].spawnFast.scriptIndex = (u16)scriptForFast;
        SetAndExecuteScript(&manager->templates[i].spawnFast,
                            *(void **)(anmManagerForFast + 4 * scriptForFast + 0x28EF0));
        scriptForNormal = g_BulletTemplateScriptIds[5 * i + 2];
        anmManagerForNormal = (int)g_AnmManager;
        manager->templates[i].spawnNormal.scriptIndex = (u16)scriptForNormal;
        SetAndExecuteScript(&manager->templates[i].spawnNormal,
                            *(void **)(anmManagerForNormal + 4 * scriptForNormal + 0x28EF0));
        scriptForSlow = g_BulletTemplateScriptIds[5 * i + 3];
        anmManagerForSlow = (int)g_AnmManager;
        manager->templates[i].spawnSlow.scriptIndex = (u16)scriptForSlow;
        SetAndExecuteScript(&manager->templates[i].spawnSlow,
                            *(void **)(anmManagerForSlow + 4 * scriptForSlow + 0x28EF0));
        scriptForDonut = g_BulletTemplateScriptIds[5 * i + 4];
        anmManagerForDonut = (int)g_AnmManager;
        manager->templates[i].spawnDonut.scriptIndex = (u16)scriptForDonut;
        SetAndExecuteScript(&manager->templates[i].spawnDonut,
                            *(void **)(anmManagerForDonut + 4 * scriptForDonut + 0x28EF0));

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
            switch (g_BulletTemplateScriptIds[5 * i])
            {
            case 514:
            case 516:
            case 517:
            case 518:
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
            scriptForLargeBullet = g_BulletTemplateScriptIds[5 * i];
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
    int shouldReleaseAnm;

    if (g_SupervisorState != 3 && g_SupervisorState != 11 && g_SupervisorState != 12)
    {
        shouldReleaseAnm = 1;
    }
    else
    {
        shouldReleaseAnm = 0;
    }
    if (shouldReleaseAnm)
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

        if (!bullet->isInUse)
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
        g_AnmManager->ResetBulletAnimation(bullet, bullet->spriteIndex + bullet->despawnState);
    }
}

} // namespace th07
