#include "inttypes.hpp"

namespace th07 {

struct ReplayStopInput {
    u16 input;
    u16 auxiliary;
};

struct ReplayStopOverlay {
    u8 unknown000[0x84];
    ReplayStopInput *currentInput;
    ReplayStopInput *stageBookmarks[7];
};

extern ReplayStopOverlay *g_ReplayStopManager;
extern i32 g_ReplayStopStage;

struct ReplayStop {
    static void StopRecording();
};

void ReplayStop::StopRecording()
{
    ReplayStopOverlay *manager;
    i32 stage;

    manager = g_ReplayStopManager;
    if (manager) {
        ++manager->currentInput;
        manager->currentInput->input = 0;
        stage = g_ReplayStopStage - 1;
        if (stage >= 7) {
            stage = 6;
        }
        manager->stageBookmarks[stage] = manager->currentInput + 1;
    }
}

} // namespace th07
