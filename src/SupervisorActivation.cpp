#include "inttypes.hpp"

namespace th07
{
struct AnmManager
{
    void PrepareRenderState();
};

struct SupervisorRenderProxy;

typedef i32(__stdcall *SupervisorRenderStateCall)(
    SupervisorRenderProxy *receiver, i32 state, i32 enabled);

struct SupervisorRenderProxyVtable
{
    void *unknown00[50];
    SupervisorRenderStateCall setRenderState;
};

struct SupervisorRenderProxy
{
    SupervisorRenderProxyVtable *vtable;
};

struct SupervisorRenderState
{
    u8 unknown00[8];
    SupervisorRenderProxy *renderProxy;
    u8 unknown0C[0x2B0];
    i32 renderState;

    i32 ActivateRenderState();
};

extern AnmManager *g_AnmManager;

i32 SupervisorRenderState::ActivateRenderState()
{
    g_AnmManager->PrepareRenderState();

    if (renderState != 1)
    {
        renderState = 1;
        return renderProxy->vtable->setRenderState(renderProxy, 0x1C, 1);
    }

    return 0;
}
} // namespace th07
