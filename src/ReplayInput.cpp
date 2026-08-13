#include "inttypes.hpp"

namespace th07 {

extern u16 g_ReplayInputSnapshot;
extern i32 g_ReplayInputCounter;
extern i32 g_ReplayInputPending;
extern u32 g_ReplayInputFlags;
extern u8 g_ReplayInputMode;

struct ReplayInputGuiState {
    u8 unknown0000[0x209B0];
    u8 restartReady;
};

struct ReplayInputGui {
    u8 unknown00[8];
    ReplayInputGuiState *state;

    i32 IsBombInputBlocked();
    i32 IsRestartReady();
};

struct ReplayInputUi {
    i32 IsBusy();
};

extern ReplayInputGui g_ReplayInputGui;
extern ReplayInputUi g_ReplayInputUi;

i32 ReplayInputGui::IsRestartReady()
{
    return (i32)state->restartReady;
}

struct ReplayInput {
    i32 frameCounter;
    u8 unknown004[0xD0];
    u16 previousInput;
    u16 inputFlags;

    i32 InitializeInputState();
    i32 UpdateControl();
};

i32 ReplayInput::InitializeInputState()
{
    inputFlags = 0;
    previousInput = g_ReplayInputSnapshot;
    g_ReplayInputCounter = 0;
    if (g_ReplayInputPending) {
        inputFlags |= 0x100;
    }
    g_ReplayInputPending = 0;
    return 1;
}

i32 ReplayInput::UpdateControl()
{
    if (((g_ReplayInputFlags >> 2) & 1) == 0) {
        return 1;
    }
    if (g_ReplayInputGui.IsBombInputBlocked() && g_ReplayInputGui.IsRestartReady() && frameCounter % 3 != 2) {
        return 6;
    }
    if (g_ReplayInputMode == 2 && !g_ReplayInputUi.IsBusy() && frameCounter % 5 != 4) {
        return 6;
    }
    return 1;
}

} // namespace th07
