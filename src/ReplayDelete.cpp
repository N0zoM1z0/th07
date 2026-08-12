#include "inttypes.hpp"

#include <stdlib.h>

namespace th07 {

struct ChainElem;

struct Chain {
    void Cut(ChainElem *elem);
};

extern Chain g_Chain;

struct ReplayDeleteOverlay {
    u8 unknown000[0x04];
    void *replayData;
    u8 unknown008[0x38];
    void *replayFile;
    u8 unknown044[0x84];
    ChainElem *calcChain;
    ChainElem *drawChain;
    ChainElem *highPriorityChain;
};

extern ReplayDeleteOverlay *g_ReplayDeleteManager;

struct ReplayDelete {
    i32 DeletedCallback();
};

static __forceinline void ReplayFree(void *memory)
{
    free(memory);
}

i32 ReplayDelete::DeletedCallback()
{
    g_Chain.Cut(reinterpret_cast<ReplayDeleteOverlay *>(this)->calcChain);
    reinterpret_cast<ReplayDeleteOverlay *>(this)->calcChain = 0;
    if (reinterpret_cast<ReplayDeleteOverlay *>(this)->drawChain) {
        g_Chain.Cut(reinterpret_cast<ReplayDeleteOverlay *>(this)->drawChain);
        reinterpret_cast<ReplayDeleteOverlay *>(this)->drawChain = 0;
    }
    if (reinterpret_cast<ReplayDeleteOverlay *>(this)->highPriorityChain) {
        g_Chain.Cut(reinterpret_cast<ReplayDeleteOverlay *>(this)->highPriorityChain);
        reinterpret_cast<ReplayDeleteOverlay *>(this)->highPriorityChain = 0;
    }
    ReplayFree(g_ReplayDeleteManager->replayData);
    if (reinterpret_cast<ReplayDeleteOverlay *>(this)->replayFile) {
        ReplayFree(reinterpret_cast<ReplayDeleteOverlay *>(this)->replayFile);
    }
    delete g_ReplayDeleteManager;
    g_ReplayDeleteManager = 0;
    g_ReplayDeleteManager = 0;
    return 0;
}

} // namespace th07
