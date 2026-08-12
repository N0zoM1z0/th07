#include "inttypes.hpp"
#include <string.h>

namespace th07
{
enum ChainCallbackResult
{
    CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB = 0,
};

typedef ChainCallbackResult(__fastcall *ChainCallback)(void *argument);
typedef i32(__fastcall *ChainLifetimeCallback)(void *argument);

class ChainElem
{
  public:
    i16 priority;
    u16 isHeapAllocated : 1;
    ChainCallback callback;
    ChainLifetimeCallback addedCallback;
    ChainLifetimeCallback deletedCallback;
    ChainElem *prev;
    ChainElem *next;
    ChainElem *unknown;
    void *argument;
};

struct Chain
{
    i32 AddToCalcChain(ChainElem *element, i32 priority);
    i32 AddToDrawChain(ChainElem *element, i32 priority);
    ChainElem *CreateElem(ChainCallback callback);
};

extern Chain g_Chain;

struct PlayerLifecycleTimer
{
    i32 current;
    i32 fraction;
    i32 previous;
};

struct PlayerLifecycleOverlay
{
    u8 unknown0000[0x2409];
    u8 initialState;
    u8 unknown240A[0x145F6];
    PlayerLifecycleTimer currentTimer;
    u8 unknown16A0C[0xA1450];
    ChainElem *calcChain;
    ChainElem *bombDrawChain;
    ChainElem *secondaryBulletDrawChain;
    u8 unknownB7E68[0x10];
};

extern PlayerLifecycleOverlay *g_PlayerLifecyclePlayer;

class PlayerLifecycle
{
  public:
    static ChainCallbackResult __fastcall OnUpdate(void *argument);
    static ChainCallbackResult __fastcall OnDrawBomb(void *argument);
    static ChainCallbackResult __fastcall OnDrawSecondaryBullets(void *argument);
    static i32 __fastcall AddedCallback(void *argument);
    static i32 __fastcall DeletedCallback(void *argument);
    static i32 __fastcall RegisterChain(i32 initialState);
};

i32 __fastcall PlayerLifecycle::RegisterChain(i32 initialState)
{
    PlayerLifecycleTimer *timer;
    PlayerLifecycleOverlay *player;

    player = reinterpret_cast<PlayerLifecycleOverlay *>(&g_PlayerLifecyclePlayer);
    memset(player, 0, sizeof(*player));
    timer = &player->currentTimer;
    timer->previous = 0;
    timer->fraction = 0;
    timer->current = -999;
    player->initialState = initialState;

    player->calcChain = g_Chain.CreateElem(OnUpdate);
    player->bombDrawChain = g_Chain.CreateElem(OnDrawBomb);
    player->secondaryBulletDrawChain = g_Chain.CreateElem(OnDrawSecondaryBullets);
    player->calcChain->argument = player;
    player->bombDrawChain->argument = player;
    player->secondaryBulletDrawChain->argument = player;
    player->calcChain->addedCallback = AddedCallback;
    player->calcChain->deletedCallback = DeletedCallback;
    if (g_Chain.AddToCalcChain(player->calcChain, 8))
    {
        return -1;
    }
    g_Chain.AddToDrawChain(player->bombDrawChain, 6);
    g_Chain.AddToDrawChain(player->secondaryBulletDrawChain, 8);
    return 0;
}

} // namespace th07
