#pragma once

#include "inttypes.hpp"

#include <windows.h>

namespace th07
{

enum ChainCallbackResult
{
    CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB = 0,
    CHAIN_CALLBACK_RESULT_CONTINUE = 1,
    CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN = 2,
    CHAIN_CALLBACK_RESULT_BREAK = 3,
    CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS = 4,
    CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR = 5,
    CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB = 6,
};

typedef ChainCallbackResult (__fastcall *ChainCallback)(void *argument);
typedef i32 (__fastcall *ChainLifetimeCallback)(void *argument);

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

    ChainElem();
    ~ChainElem();
};
C_ASSERT(sizeof(ChainElem) == 0x20);

class Chain
{
  private:
    ChainElem calcChain;
    ChainElem drawChain;
    u32 midiOutputDeviceCount;
    u32 unknown44;

    void ReleaseSingleChain(ChainElem *root);

  public:
    Chain();
    ~Chain();

    i32 AddToCalcChain(ChainElem *element, i32 priority);
    i32 AddToDrawChain(ChainElem *element, i32 priority);
    i32 RunCalcChain();
    i32 RunDrawChain();
    void Release();
    ChainElem *CreateElem(ChainCallback callback);
    void Cut(ChainElem *element);
};
C_ASSERT(sizeof(Chain) == 0x48);

extern Chain g_Chain;

} // namespace th07
