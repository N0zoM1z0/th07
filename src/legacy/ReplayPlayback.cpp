#include "inttypes.hpp"

namespace th07
{
struct ReplayPlaybackGameManager
{
    u8 unknown00[0x25];
    u8 isInMenu;
};

struct ReplayPlaybackInput
{
    u16 input;
    u16 unknown02;
};

struct ReplayPlayback
{
    i32 frameId;
    u8 unknown004[0x80];
    ReplayPlaybackInput *inputCursor;
    u8 unknown088[0x1C];
    i8 *extraCursor;

    i32 OnUpdate();
};

extern u32 g_ReplayPlaybackFlags;
extern void *g_ReplayPlaybackGameManager;
extern u16 g_ReplayPlaybackPreviousInput;
extern u16 g_ReplayPlaybackCurrentInput;
extern u16 g_ReplayPlaybackRepeatPulse;
extern u16 g_ReplayPlaybackRepeatFrames;
extern u16 g_ReplayPlaybackInputValue;
extern i32 g_ReplayPlaybackExtra;

i32 ReplayPlayback::OnUpdate()
{
    i32 unknown;

    if (((g_ReplayPlaybackFlags >> 2) & 1) == 0)
    {
        return 1;
    }
    if (reinterpret_cast<ReplayPlaybackGameManager *>(g_ReplayPlaybackGameManager)->isInMenu)
    {
        return 1;
    }

    unknown = 0;
    g_ReplayPlaybackPreviousInput = g_ReplayPlaybackCurrentInput;
    g_ReplayPlaybackCurrentInput = inputCursor->input;
    ++inputCursor;
    g_ReplayPlaybackRepeatPulse = 0;
    if (g_ReplayPlaybackPreviousInput == g_ReplayPlaybackCurrentInput)
    {
        if (g_ReplayPlaybackRepeatFrames >= 30)
        {
            if (g_ReplayPlaybackRepeatFrames % 8 == 0)
            {
                g_ReplayPlaybackRepeatPulse = 1;
            }
            if (g_ReplayPlaybackRepeatFrames >= 38)
            {
                g_ReplayPlaybackRepeatFrames = 30;
            }
        }
        ++g_ReplayPlaybackRepeatFrames;
    }
    else
    {
        g_ReplayPlaybackRepeatFrames = 0;
    }

    if (frameId % 30 == 0)
    {
        g_ReplayPlaybackInputValue = extraCursor[1] & 0x7F;
        g_ReplayPlaybackExtra = extraCursor[1] >> 7;
        ++extraCursor;
    }
    ++frameId;
    return 1;
}

} // namespace th07
