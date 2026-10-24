// Compile registration natively with the shared initializer kept out of the
// speculative constructor import. The linker selects the exact constructor
// emitted by ResultScreenCtor.cpp. No class/initializer body changes by TU.
#define TH07_SCORE_NODE_INIT_NOINLINE
#pragma optimize("s", on)
#include "Global.hpp"
#pragma function(memset)
#include "ResultScreen.hpp"
#include "GameManager.hpp"
#include "utils.hpp"

// FUNCTION: TH07 0x0044a302
ZunResult ResultScreen::RegisterChain(u32 type)
{
    ResultScreen *resultScreen = ZUN_NEW(ResultScreen, "ResultSysInf");

    // STRING: TH07 0x0049635c
    utils::DebugPrint2("Stg.PlayTimeAll = %d\r\n",
                       g_GameManager.totalPlayTime);
    if (type == 1)
    {
        if (!g_GameManager.practice)
        {
            resultScreen->resultScreenState = RESULT_STATE_ENTER_NAME;
        }
        else
        {
            resultScreen->resultScreenState = RESULT_STATE_PRACTICE_END;
        }
    }
    else if (type == 2)
    {
        resultScreen->resultScreenState = RESULT_STATE_INIT_PARSE_ONLY;
        AddedCallback(resultScreen);
        return ZUN_SUCCESS;
    }
    resultScreen->calcChain = g_Chain.CreateElem((ChainCallback)OnUpdate);
    resultScreen->calcChain->addedCallback =
        (ChainLifecycleCallback)AddedCallback;
    resultScreen->calcChain->deletedCallback =
        (ChainLifecycleCallback)DeletedCallback;
    resultScreen->calcChain->arg = resultScreen;
    if (g_Chain.AddToCalcChain(resultScreen->calcChain, CHAIN_PRIO_CALC_RESULTSCREEN))
    {
        return ZUN_ERROR;
    }

    resultScreen->drawChain = g_Chain.CreateElem((ChainCallback)OnDraw);
    resultScreen->drawChain->arg = resultScreen;
    g_Chain.AddToDrawChain(resultScreen->drawChain, CHAIN_PRIO_DRAW_RESULTSCREEN);

    return ZUN_SUCCESS;
}
