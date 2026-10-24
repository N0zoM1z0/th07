#include "inttypes.hpp"

namespace th07
{
struct SupervisorGameState
{
    u8 unknown00[0x25];
    u8 slowModeEnabled;
};

// Both target callers load ECX with the Supervisor global at 0x00575950.
// This method currently needs no Supervisor field, but its thiscall home is
// observable code-generation evidence.
struct Supervisor
{
    i32 IsSlowModeEnabled();
};

extern SupervisorGameState *g_TargetGameManager626274;

#pragma optimize("s", on)
i32 Supervisor::IsSlowModeEnabled()
{
    i32 result;

    if (g_TargetGameManager626274 != 0 &&
        g_TargetGameManager626274->slowModeEnabled)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}
#pragma optimize("s", off)
} // namespace th07
