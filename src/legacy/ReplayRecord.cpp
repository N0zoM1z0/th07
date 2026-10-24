#include "inttypes.hpp"

namespace th07 {

struct ReplayRecordInput {
    u16 input;
    u16 capturedInput;
};

struct ReplayRecord {
    i32 frameId;
    u8 unknown04[0x80];
    ReplayRecordInput *currentInput;
    ReplayRecordInput *stageBookmarks[7];
    u8 *auxiliaryCursor;
    u8 *auxiliaryBookmarks[7];
    u8 unknownC4[0x12];
    u16 capturedInput;

    i32 OnUpdate();
};

struct ReplayRecordGameManager {
    u8 unknown00[0x25];
    u8 isInMenu;
};

extern u32 g_ReplayRecordFlags;
extern u16 g_ReplayRecordPreviousInput;
extern u16 g_ReplayRecordCurrentInput;
extern u16 g_ReplayRecordRawInput;
extern void *g_ReplayRecordGameManager;
extern i32 g_ReplayRecordSupervisorFlags;
extern i32 g_ReplayRecordStage;
extern u16 g_ReplayRecordInputValue;
extern i32 g_ReplayRecordExtra;

#pragma var_order(stageIndex, input)
i32 ReplayRecord::OnUpdate()
{
    if (!((g_ReplayRecordFlags >> 2) & 1)) {
        return 1;
    }

    g_ReplayRecordPreviousInput = g_ReplayRecordCurrentInput;
    g_ReplayRecordCurrentInput = g_ReplayRecordRawInput;

    if (reinterpret_cast<ReplayRecordGameManager *>(g_ReplayRecordGameManager)->isInMenu) {
        return 1;
    }

    if ((static_cast<u32>(g_ReplayRecordSupervisorFlags) >> 3) & 1) {
        return 1;
    }

    u16 input;
    i32 stageIndex = g_ReplayRecordStage - 1;
    if (stageIndex >= 7) {
        stageIndex = 6;
    }

    input = g_ReplayRecordRawInput;
    g_ReplayRecordCurrentInput = input;

    ++currentInput;
    stageBookmarks[stageIndex] = currentInput + 1;
    currentInput->input = input;
    currentInput->capturedInput = capturedInput;

    if ((frameId % 30) == 0) {
        *auxiliaryCursor = static_cast<u8>(g_ReplayRecordInputValue) |
                           (g_ReplayRecordExtra ? 0x80 : 0);
        auxiliaryCursor[1] = static_cast<u8>(g_ReplayRecordInputValue);
        auxiliaryBookmarks[stageIndex] = auxiliaryCursor + 2;
        ++auxiliaryCursor;
    }

    ++frameId;
    return 1;
}

} // namespace th07
