#include "inttypes.hpp"

#include <stdlib.h>

namespace th07 {

struct PlayerLifecycleOverlay {
    u8 bytes[1];
};

struct PlayerLifecycleAnmManager {
    void ReleaseAnm(i32 slot);
};

struct PlayerLifecycleBossUi {
    i16 value;
    u8 unknown002[0x24A];
};

struct ChainElem;

struct Chain {
    void Cut(ChainElem *elem);
};

struct PlayerLifecycle {
    static i32 __fastcall DeletedCallback(PlayerLifecycleOverlay *player);
    static void __cdecl CutChain();
};

extern i32 g_PlayerLifecycleSupervisorState;
extern PlayerLifecycleAnmManager *g_PlayerLifecycleAnmManager;
extern i16 g_PlayerLifecycleHudActive;
extern i32 g_PlayerLifecycleHudTimer;
extern PlayerLifecycleBossUi g_PlayerLifecycleBossUi[];
extern void *g_PlayerLifecycleResources[2];
extern Chain g_Chain;
extern ChainElem *g_PlayerLifecycleChainElems[3];

static __forceinline void ResetBossUi(i32 index)
{
    g_PlayerLifecycleBossUi[index].value = 99;
}

#pragma var_order(firstResource, secondResource, player)
i32 __fastcall PlayerLifecycle::DeletedCallback(PlayerLifecycleOverlay *player)
{
    void *firstResource;
    void *secondResource;

    if ((g_PlayerLifecycleSupervisorState != 3 && g_PlayerLifecycleSupervisorState != 11
         && g_PlayerLifecycleSupervisorState != 12) != false) {
        g_PlayerLifecycleAnmManager->ReleaseAnm(10);
        g_PlayerLifecycleHudActive = 99;
        g_PlayerLifecycleHudTimer = 99;
        ResetBossUi(0);
        ResetBossUi(1);
        ResetBossUi(2);
    }

    if (g_PlayerLifecycleResources[0]) {
        firstResource = g_PlayerLifecycleResources[0];
        free(firstResource);
        g_PlayerLifecycleResources[0] = 0;
    }
    if (g_PlayerLifecycleResources[1]) {
        secondResource = g_PlayerLifecycleResources[1];
        free(secondResource);
        g_PlayerLifecycleResources[1] = 0;
    }
    return 0;
}

void __cdecl PlayerLifecycle::CutChain()
{
    g_Chain.Cut(g_PlayerLifecycleChainElems[0]);
    g_PlayerLifecycleChainElems[0] = 0;
    g_Chain.Cut(g_PlayerLifecycleChainElems[1]);
    g_PlayerLifecycleChainElems[1] = 0;
    g_Chain.Cut(g_PlayerLifecycleChainElems[2]);
    g_PlayerLifecycleChainElems[2] = 0;
}

} // namespace th07
