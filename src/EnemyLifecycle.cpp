#include "inttypes.hpp"

namespace th07
{

struct ChainNodeOverlay
{
    u8 unknown00[4];
    void *callback;
    void *addedCallback;
    void *deletedCallback;
    u8 unknown10[0xC];
    void *arg;
};

struct ChainOverlay
{
    void Cut(ChainNodeOverlay *node);
    i32 AddToCalcChain(ChainNodeOverlay *node, i32 priority);
    i32 AddToDrawChain(ChainNodeOverlay *node, i32 priority);
};

// The storage type is defined privately in EnemyManagerUpdate.cpp.  This
// declaration exists only to preserve the target-attested member ABI here.
struct EnemyManagerUpdateOverlay
{
    void ClearObservedStorage();
};

extern ChainOverlay g_Chain;
extern ChainNodeOverlay g_EnemyCalcChain;
extern ChainNodeOverlay g_EnemyDrawHighChain;
extern ChainNodeOverlay g_EnemyDrawLowChain;
extern ChainNodeOverlay g_TargetResetCalcChain13478F8;
extern ChainNodeOverlay g_TargetResetDrawChain1347918;

extern i32 __fastcall Target41C790(void *argument);
extern i32 __fastcall Target41CDE0(void *argument);
extern i32 __fastcall Target41D050(void *argument);
extern i32 __fastcall Target41CA10(void *argument);

struct EnemyAnmManager
{
    void ReleaseAnm(i32 slot);
    i32 LoadAnm(i32 slot, const char *filename, i32 offset);
};

struct EnemyVec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct BossUiSlot
{
    EnemyVec3 position;
    u8 unknown0C[0x240];
};

extern EnemyAnmManager *g_EnemyAnmManager;
extern BossUiSlot g_EnemyBossUi[4];

struct EnemyRng
{
    u16 GetRandomU16();

    __forceinline u16 GetRandomU16InRange(u16 range)
    {
        return range != 0 ? GetRandomU16() % range : 0;
    }
};

extern EnemyRng g_EnemyRng;
struct EnemyManagerLifecycle;
extern EnemyManagerLifecycle g_EnemyManagerLifecycle;
i32 __fastcall EnemyManagerUpdateCallback(EnemyManagerLifecycle *manager);
i32 __fastcall EnemyManagerAddedCallback(EnemyManagerLifecycle *manager);
i32 __fastcall EnemyManagerDeletedCallback(EnemyManagerLifecycle *manager);
i32 __fastcall EnemyManagerDrawHighCallback(EnemyManagerLifecycle *manager);
i32 __fastcall EnemyManagerDrawLowCallback(EnemyManagerLifecycle *manager);

struct EnemyManagerLifecycle
{
    const char *enemyAnmFilename;
    const char *enemy2AnmFilename;
    u8 unknown0008[0x9545B0];
    u16 randomItemSpawnIndex;
    u16 randomItemTableIndex;
    u8 unknown9545BC[0xC];
    i32 spellActive;

    static void CutChain();
    static i32 __fastcall RegisterChain(const char *enemyAnm, const char *enemy2Anm);
    i32 OnUpdate();
    i32 OnDrawHighPriority();
    i32 OnDrawLowPriority();
    void Initialize();
    i32 ReleaseAnmSlots17To20();
    i32 AddedCallback();
    i32 DeletedCallback();
};

void EnemyManagerLifecycle::CutChain()
{
    g_Chain.Cut(&g_EnemyCalcChain);
    g_Chain.Cut(&g_EnemyDrawHighChain);
    g_Chain.Cut(&g_EnemyDrawLowChain);
}

i32 __cdecl RegisterObservedResetChains()
{
    EnemyManagerUpdateOverlay *manager;

    manager = reinterpret_cast<EnemyManagerUpdateOverlay *>(0x012FE250);
    manager->ClearObservedStorage();

    g_TargetResetCalcChain13478F8.callback = reinterpret_cast<void *>(&Target41C790);
    g_TargetResetCalcChain13478F8.addedCallback = 0;
    g_TargetResetCalcChain13478F8.deletedCallback = 0;
    g_TargetResetCalcChain13478F8.addedCallback = reinterpret_cast<void *>(&Target41CDE0);
    g_TargetResetCalcChain13478F8.deletedCallback = reinterpret_cast<void *>(&Target41D050);
    g_TargetResetCalcChain13478F8.arg = manager;
    if (g_Chain.AddToCalcChain(&g_TargetResetCalcChain13478F8, 11))
        return -1;

    g_TargetResetDrawChain1347918.callback = reinterpret_cast<void *>(&Target41CA10);
    g_TargetResetDrawChain1347918.addedCallback = 0;
    g_TargetResetDrawChain1347918.deletedCallback = 0;
    g_TargetResetDrawChain1347918.arg = manager;
    g_Chain.AddToDrawChain(&g_TargetResetDrawChain1347918, 9);
    return 0;
}

i32 __fastcall EnemyManagerLifecycle::RegisterChain(const char *enemyAnm, const char *enemy2Anm)
{
    EnemyManagerLifecycle *manager;

    manager = &g_EnemyManagerLifecycle;
    manager->Initialize();
    manager->enemyAnmFilename = enemyAnm;
    manager->enemy2AnmFilename = enemy2Anm;

    g_EnemyCalcChain.callback = reinterpret_cast<void *>(&EnemyManagerUpdateCallback);
    g_EnemyCalcChain.addedCallback = 0;
    g_EnemyCalcChain.deletedCallback = 0;
    g_EnemyCalcChain.addedCallback = reinterpret_cast<void *>(&EnemyManagerAddedCallback);
    g_EnemyCalcChain.deletedCallback = reinterpret_cast<void *>(&EnemyManagerDeletedCallback);
    g_EnemyCalcChain.arg = manager;
    if (g_Chain.AddToCalcChain(&g_EnemyCalcChain, 10))
        return -1;

    g_EnemyDrawHighChain.callback = reinterpret_cast<void *>(&EnemyManagerDrawHighCallback);
    g_EnemyDrawHighChain.addedCallback = 0;
    g_EnemyDrawHighChain.deletedCallback = 0;
    g_EnemyDrawHighChain.arg = manager;
    if (g_Chain.AddToDrawChain(&g_EnemyDrawHighChain, 5))
        return -1;

    g_EnemyDrawLowChain.callback = reinterpret_cast<void *>(&EnemyManagerDrawLowCallback);
    g_EnemyDrawLowChain.addedCallback = 0;
    g_EnemyDrawLowChain.deletedCallback = 0;
    g_EnemyDrawLowChain.arg = manager;
    if (g_Chain.AddToDrawChain(&g_EnemyDrawLowChain, 7))
        return -1;
    return 0;
}

i32 EnemyManagerLifecycle::ReleaseAnmSlots17To20()
{
    g_EnemyAnmManager->ReleaseAnm(17);
    g_EnemyAnmManager->ReleaseAnm(18);
    g_EnemyAnmManager->ReleaseAnm(19);
    g_EnemyAnmManager->ReleaseAnm(20);
    return 0;
}

#pragma var_order(enemies, position)
i32 EnemyManagerLifecycle::AddedCallback()
{
    u8 *enemies;
    EnemyVec3 position;

    enemies = reinterpret_cast<u8 *>(this) + 0x4F50;
    if (enemyAnmFilename && g_EnemyAnmManager->LoadAnm(15, enemyAnmFilename, 0x900))
        return -1;
    if (enemy2AnmFilename && g_EnemyAnmManager->LoadAnm(16, enemy2AnmFilename, 0x900))
        return -1;

    randomItemSpawnIndex = g_EnemyRng.GetRandomU16InRange(3);
    randomItemTableIndex = g_EnemyRng.GetRandomU16InRange(8);
    spellActive = 0;

    position.x = -999.0f;
    position.y = -999.0f;
    position.z = -999.0f;
    __asm
    {
        xor edx, edx
        imul edx, edx, 0x24c
        add edx, OFFSET g_EnemyBossUi
        mov eax, DWORD PTR[position.x]
        mov DWORD PTR[edx], eax
        mov ecx, DWORD PTR[position.y]
        mov DWORD PTR[edx + 4], ecx
        mov eax, DWORD PTR[position.z]
        mov DWORD PTR[edx + 8], eax

        mov ecx, 1
        imul ecx, ecx, 0x24c
        add ecx, OFFSET g_EnemyBossUi
        mov edx, DWORD PTR[position.x]
        mov DWORD PTR[ecx], edx
        mov eax, DWORD PTR[position.y]
        mov DWORD PTR[ecx + 4], eax
        mov edx, DWORD PTR[position.z]
        mov DWORD PTR[ecx + 8], edx

        mov eax, 2
        imul eax, eax, 0x24c
        add eax, OFFSET g_EnemyBossUi
        mov ecx, DWORD PTR[position.x]
        mov DWORD PTR[eax], ecx
        mov edx, DWORD PTR[position.y]
        mov DWORD PTR[eax + 4], edx
        mov ecx, DWORD PTR[position.z]
        mov DWORD PTR[eax + 8], ecx

        mov edx, 3
        imul edx, edx, 0x24c
        add edx, OFFSET g_EnemyBossUi
        mov eax, DWORD PTR[position.x]
        mov DWORD PTR[edx], eax
        mov ecx, DWORD PTR[position.y]
        mov DWORD PTR[edx + 4], ecx
        mov eax, DWORD PTR[position.z]
        mov DWORD PTR[edx + 8], eax
    }
    return 0;
}

#pragma var_order(this, position)
i32 EnemyManagerLifecycle::DeletedCallback()
{
    EnemyVec3 position;

    g_EnemyAnmManager->ReleaseAnm(16);
    g_EnemyAnmManager->ReleaseAnm(15);
    position.x = -999.0f;
    position.y = -999.0f;
    position.z = -999.0f;
    // Preserve VC7's unoptimized constant-index address materialization.
    __asm
    {
        xor eax, eax
        imul eax, eax, 0x24c
        add eax, OFFSET g_EnemyBossUi
        mov ecx, DWORD PTR[position.x]
        mov DWORD PTR[eax], ecx
        mov edx, DWORD PTR[position.y]
        mov DWORD PTR[eax + 4], edx
        mov ecx, DWORD PTR[position.z]
        mov DWORD PTR[eax + 8], ecx

        mov edx, 1
        imul edx, edx, 0x24c
        add edx, OFFSET g_EnemyBossUi
        mov eax, DWORD PTR[position.x]
        mov DWORD PTR[edx], eax
        mov ecx, DWORD PTR[position.y]
        mov DWORD PTR[edx + 4], ecx
        mov eax, DWORD PTR[position.z]
        mov DWORD PTR[edx + 8], eax

        mov ecx, 2
        imul ecx, ecx, 0x24c
        add ecx, OFFSET g_EnemyBossUi
        mov edx, DWORD PTR[position.x]
        mov DWORD PTR[ecx], edx
        mov eax, DWORD PTR[position.y]
        mov DWORD PTR[ecx + 4], eax
        mov edx, DWORD PTR[position.z]
        mov DWORD PTR[ecx + 8], edx

        mov eax, 3
        imul eax, eax, 0x24c
        add eax, OFFSET g_EnemyBossUi
        mov ecx, DWORD PTR[position.x]
        mov DWORD PTR[eax], ecx
        mov edx, DWORD PTR[position.y]
        mov DWORD PTR[eax + 4], edx
        mov ecx, DWORD PTR[position.z]
        mov DWORD PTR[eax + 8], ecx
    }
    return 0;
}

} // namespace th07
