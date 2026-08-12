#include "inttypes.hpp"

namespace th07
{

// Private overlay for the two target-confirmed EnemyManager draw-chain
// adapters.  The manager layout is not needed by either wrapper.
struct EnemyManagerRenderOverlay
{
    u8 unknown00[0x954598];
    void *bosses[8];

    i32 __fastcall DrawImpl(i32 drawGroup, i32 chainPriority);
    i32 OnDrawHighPriority();
    i32 OnDrawLowPriority();
    i32 HasActiveBoss();
};

i32 EnemyManagerRenderOverlay::OnDrawHighPriority()
{
    return DrawImpl(0, 2);
}

i32 EnemyManagerRenderOverlay::OnDrawLowPriority()
{
    return DrawImpl(2, 4);
}

i32 EnemyManagerRenderOverlay::HasActiveBoss()
{
    for (i32 i = 0; i < 8; ++i)
    {
        if (bosses[i])
            return 1;
    }
    return 0;
}

} // namespace th07
