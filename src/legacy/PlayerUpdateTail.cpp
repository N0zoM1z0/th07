#include "inttypes.hpp"

namespace th07 {

struct PlayerUpdateTailOverlay;

struct PlayerUpdateTailAnmManager {
    i32 ExecuteScript(void *vm);
};

extern i8 g_PlayerUpdateDisabled;
extern i32 g_PlayerUiState;
extern i16 g_PlayerUiTransition;
extern PlayerUpdateTailAnmManager *g_PlayerUpdateAnmManager;

extern i32 __fastcall RunPlayerMovement(PlayerUpdateTailOverlay *player);

struct PlayerUpdateTailOverlay {
    i8 bytes[1];

    void ResetAuxiliaryState();
    void BeginBomb();
    i32 UpdateStateTwo();
    void UpdateStateVisual();
    void UpdateShotAndCollisionState();
    void UpdatePlayerBullets();
    void UpdatePlayerEffects();
    void UpdateUiState();
    i32 OnUpdate();
};

struct PlayerUpdateTailVec3 {
    f32 x;
    f32 y;
    f32 z;
};

#pragma var_order(first, second, uiState0, uiState1, uiState2)
void PlayerUpdateTailOverlay::UpdateUiState()
{
    PlayerUpdateTailVec3 first;
    PlayerUpdateTailVec3 second;
    i32 uiState0;
    i32 uiState1;
    i32 uiState2;

    first.x = -999.0f;
    first.y = -999.0f;
    first.z = 0.0f;
    *reinterpret_cast<PlayerUpdateTailVec3 *>(bytes + 0x2428) = first;
    second.x = -999.0f;
    second.y = -999.0f;
    second.z = 0.0f;
    *reinterpret_cast<PlayerUpdateTailVec3 *>(bytes + 0x2434) = second;
    *reinterpret_cast<i32 *>(bytes + 0x2440) = 0;

    if (*reinterpret_cast<f32 *>(bytes + 0x934) >= 400.0f) {
        uiState0 = g_PlayerUiState;
        if (uiState0 != 2 && *reinterpret_cast<f32 *>(bytes + 0x930) < 160.0f) {
            g_PlayerUiTransition = 2;
            g_PlayerUiState = 2;
        } else {
            uiState1 = g_PlayerUiState;
            if (uiState1 == 2 && *reinterpret_cast<f32 *>(bytes + 0x930) > 160.0f) {
                g_PlayerUiTransition = 3;
                g_PlayerUiState = 3;
            }
        }
    } else {
        uiState2 = g_PlayerUiState;
        if (uiState2 == 2) {
            g_PlayerUiTransition = 3;
            g_PlayerUiState = 3;
        }
    }
}

i32 PlayerUpdateTailOverlay::OnUpdate()
{
    if (g_PlayerUpdateDisabled) {
        return 1;
    }
    ResetAuxiliaryState();
    BeginBomb();
    if (bytes[0x2408] == 2) {
        if (UpdateStateTwo()) {
            goto UPDATE_STATE_VISUAL;
        }
        goto UPDATE_STATE_DONE;
    }
    if (bytes[0x2408] == 1) {
UPDATE_STATE_VISUAL:
        UpdateStateVisual();
    }
UPDATE_STATE_DONE:
    UpdateShotAndCollisionState();
    if (bytes[0x2408] != 2 && bytes[0x2408] != 1) {
        RunPlayerMovement(this);
    }
    g_PlayerUpdateAnmManager->ExecuteScript(this);
    if (bytes[0x240A]) {
        g_PlayerUpdateAnmManager->ExecuteScript(bytes + 0x24C);
        g_PlayerUpdateAnmManager->ExecuteScript(bytes + 0x498);
    }
    UpdatePlayerBullets();
    UpdatePlayerEffects();
    UpdateUiState();
    return 1;
}

} // namespace th07
