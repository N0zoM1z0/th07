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

#pragma var_order(wrappedDistance, directDistance)
f32 __stdcall InterpolateWrappedAngle(f32 start, f32 end, f32 fraction)
{
    f32 directDistance;
    f32 wrappedDistance;

    if (start < end)
    {
        directDistance = end - start;
        wrappedDistance = start + 6.2831855f - end;
    }
    else
    {
        directDistance = start - end;
        wrappedDistance = end + 6.2831855f - start;
        start = end;
    }

    if (directDistance < wrappedDistance)
        return directDistance * fraction + start;
    return wrappedDistance * fraction + start;
}

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
