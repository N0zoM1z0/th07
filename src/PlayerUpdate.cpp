#include "Player.hpp"

#include <math.h>

#pragma intrinsic(sin, cos, fabs)

namespace th07
{
/*
 * This translation unit intentionally owns a narrow, target-attested view of
 * Player.  The shared collision header currently stops before the fields used
 * by the update loop, so making this a private overlay avoids guessing a
 * repository-wide Player layout.
 */
struct PlayerBombAnmManager
{
    u8 unknown00[0x28EF0];
    void *scripts[1];

    void SetAndExecute(void *vm, void *script);

    __forceinline void SetAndExecuteScriptIdx(void *vm, i32 script)
    {
        *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(vm) + 0x1D8) = static_cast<i16>(script);
        SetAndExecute(vm, scripts[script]);
    }
};

struct PlayerOrbAnimation
{
    u8 unknown000[0x1C6];
    i16 stopRequested;

    void RequestStop()
    {
        stopRequested = 1;
    }
};

struct EffectManager
{
    PlayerOrbAnimation *SpawnParticlesColored(i32 effect, D3DXVECTOR3 *position, i32 count, i32 blendMode,
                                               i32 color);
};

struct BulletUpdateTimerManager
{
    void Advance(i32 *current, i32 *subFrame);
};

struct PlayerBombGuiState
{
    i32 IsBombInputBlocked();
};

struct PlayerBombGrazeState
{
    i32 IsBombStartBlocked();
};

extern PlayerBombAnmManager *g_PlayerBombAnmManager;
extern EffectManager g_EffectManager;
extern BulletUpdateTimerManager g_TimerManager;
extern PlayerBombGuiState g_PlayerBombGuiState49FBF0;
extern PlayerBombGrazeState g_PlayerBombGrazeState626270;
extern u16 g_PlayerInputButtons;
extern i32 g_PlayerGameMode;
extern u8 g_PlayerOrbMode;
extern f32 g_FrameMultiplier;
extern f32 g_PlayerUpdateBoundLeft;
extern f32 g_PlayerUpdateBoundTop;
extern f32 g_PlayerUpdateBoundWidth;
extern f32 g_PlayerUpdateBoundHeight;

struct PlayerUpdateTimer
{
    i32 previous;
    union
    {
        i32 subFrameBits;
        f32 subFrame;
    };
    i32 current;

    f32 AsFramesFloat()
    {
        return current + subFrame;
    }

    i32 AsFrames()
    {
        return current;
    }

    void SetCurrent(i32 value)
    {
        current = value;
        subFrame = 0.0f;
        previous = -999;
    }
};

struct PlayerUpdateMovementConfig
{
    u8 unknown00[0x24];
    f32 unfocusedAxisSpeed;
    f32 focusedAxisSpeed;
    f32 unfocusedDiagonalSpeed;
    f32 focusedDiagonalSpeed;
};

struct PlayerUpdateOverlay
{
    u8 unknown0000[0x1D8];
    i16 movementAnimation;
    u8 unknown01DA[0x756];
    D3DXVECTOR3 position;                 // +0x930
    D3DXVECTOR3 unknown93C;
    D3DXVECTOR3 killBoxTopLeft;           // +0x948
    D3DXVECTOR3 killBoxBottomRight;       // +0x954
    D3DXVECTOR3 grazeBoxTopLeft;          // +0x960
    D3DXVECTOR3 grazeBoxBottomRight;      // +0x96C
    D3DXVECTOR3 itemBoxTopLeft;           // +0x978
    D3DXVECTOR3 itemBoxBottomRight;       // +0x984
    D3DXVECTOR3 killBoxHalfSize;          // +0x990
    D3DXVECTOR3 grazeBoxHalfSize;         // +0x99C
    D3DXVECTOR3 itemBoxHalfSize;          // +0x9A8
    D3DXVECTOR3 orbPosition[2];           // +0x9B4
    f32 horizontalVelocity;                // +0x9CC
    f32 verticalVelocity;                  // +0x9D0
    u8 unknown9D4[4];
    PlayerOrbAnimation *orbAnimation;     // +0x9D8
    u8 unknown9DC[0x1A14];
    f32 movementMultiplierX;              // +0x23F0
    f32 movementMultiplierY;              // +0x23F4
    u8 unknown23F8[0x12];
    i8 orbState;                          // +0x240A
    i8 focused;                           // +0x240B
    u8 unknown240C[4];
    PlayerUpdateTimer orbTimer;           // +0x2410
    i32 inputDirection;                   // +0x241C
    f32 previousInputX;                   // +0x2420
    f32 previousInputY;                   // +0x2424
    u8 unknown2428[0xB7E58 - 0x2428];
    f32 rotationAngle;                    // +0xB7E58
    u8 unknownB7E5C[0x14];
    PlayerUpdateMovementConfig *movement; // +0xB7E70
    u8 unknownB7E74[4];

    void UpdateBombCollisionState();
};

typedef char PlayerUpdateOverlaySizeMustMatch[(sizeof(PlayerUpdateOverlay) == 0xB7E78) ? 1 : -1];

static __forceinline void ResetTimer(PlayerUpdateTimer *timer)
{
    timer->current = 0;
    timer->subFrame = 0.0f;
    timer->previous = -999;
}

static __forceinline f32 AdvanceTimer(PlayerUpdateTimer *timer)
{
    timer->previous = timer->current;
    g_TimerManager.Advance(&timer->current, &timer->subFrameBits);
    return (f32)timer->current + timer->subFrame;
}

static __forceinline f32 Cos(f32 angle)
{
    return (f32)cos(angle);
}

static __forceinline f32 Sin(f32 angle)
{
    return (f32)sin(angle);
}

static __forceinline f32 Abs(f32 value)
{
    return (f32)fabs(value);
}

static __forceinline void SelectMovementAnimation(PlayerUpdateOverlay *player, f32 horizontal)
{
    if (horizontal < 0.0f && player->previousInputX >= 0.0f)
    {
        g_PlayerBombAnmManager->SetAndExecuteScriptIdx(player, 1025);
    }
    else if (horizontal == 0.0f && player->previousInputX < 0.0f)
    {
        g_PlayerBombAnmManager->SetAndExecuteScriptIdx(player, 1026);
    }
    if (horizontal > 0.0f && player->previousInputX <= 0.0f)
    {
        g_PlayerBombAnmManager->SetAndExecuteScriptIdx(player, 1027);
    }
    else if (horizontal == 0.0f && player->previousInputX > 0.0f)
    {
        g_PlayerBombAnmManager->SetAndExecuteScriptIdx(player, 1028);
    }
}

static __forceinline void UpdateStraightOrbs(PlayerUpdateOverlay *player)
{
    f32 left = 0.0f;
    f32 vertical = 0.0f;

    switch (player->orbState)
    {
    case 0:
        ResetTimer(&player->orbTimer);
        break;
    case 1:
        left = 24.0f;
        ResetTimer(&player->orbTimer);
        if (player->focused)
        {
            player->orbState = 2;
            player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
            goto focused;
        }
        break;
    case 2:
    focused:
        if (!player->focused)
        {
            player->orbState = 4;
            player->orbTimer.SetCurrent(8 - player->orbTimer.AsFrames());
            if (player->orbAnimation)
                player->orbAnimation->RequestStop();
            return;
        }
        AdvanceTimer(&player->orbTimer);
        {
            f32 t = player->orbTimer.AsFramesFloat() / 8.0f;
            vertical = (1.0f - t) * 32.0f + -32.0f;
            t *= t;
            left = -16.0f * t + 24.0f;
            if ((i32)(player->orbTimer.current >= 8))
                player->orbState = 3;
            if (!player->focused)
            {
                player->orbState = 4;
                player->orbTimer.SetCurrent(8 - player->orbTimer.AsFrames());
                if (player->orbAnimation)
                    player->orbAnimation->RequestStop();
                goto unfocused;
            }
        }
        break;
    case 3:
        left = 8.0f;
        vertical = -32.0f;
        ResetTimer(&player->orbTimer);
        if (!player->focused)
        {
            player->orbState = 4;
            if (player->orbAnimation)
                player->orbAnimation->RequestStop();
            goto unfocused;
        }
        break;
    case 4:
    unfocused:
        AdvanceTimer(&player->orbTimer);
        {
            f32 t = player->orbTimer.AsFramesFloat() / 8.0f;
            vertical = 32.0f * t + -32.0f;
            t *= t;
            t = 1.0f - t;
            left = -16.0f * t + 24.0f;
            if ((i32)(player->orbTimer.current >= 8))
                player->orbState = 1;
            if (player->focused)
            {
                player->orbState = 2;
                player->orbTimer.SetCurrent(8 - player->orbTimer.AsFrames());
                player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
                goto focused;
            }
        }
        break;
    default:
        break;
    }

    player->orbPosition[0].x -= left;
    player->orbPosition[1].x += left;
    player->orbPosition[0].y += vertical;
    player->orbPosition[1].y += vertical;
}

#pragma var_order(previousDirection, vertical, horizontal, verticalOrbOffset, horizontalOrbOffset, intermediateFloat, \
                  rotatingY, rotatingX, rotationStep)
int __fastcall Player::OnUpdate(Player *playerBase)
{
#define player reinterpret_cast<PlayerUpdateOverlay *>(playerBase)
    f32 horizontal = 0.0f;
    f32 vertical = 0.0f;
    i32 previousDirection = player->inputDirection;
    f32 horizontalOrbOffset;
    f32 verticalOrbOffset;
    f32 intermediateFloat;
    f32 rotatingY;
    f32 rotatingX;
    f32 rotationStep;

    player->inputDirection = 0;
    if (g_PlayerInputButtons & 0x10)
    {
        player->inputDirection = 1;
        if (g_PlayerInputButtons & 0x40)
            player->inputDirection = 5;
        if (g_PlayerInputButtons & 0x80)
            player->inputDirection = 6;
    }
    else if (g_PlayerInputButtons & 0x20)
    {
        player->inputDirection = 2;
        if (g_PlayerInputButtons & 0x40)
            player->inputDirection = 7;
        if (g_PlayerInputButtons & 0x80)
            player->inputDirection = 8;
    }
    else
    {
        if (g_PlayerInputButtons & 0x40)
            player->inputDirection = 3;
        if (g_PlayerInputButtons & 0x80)
            player->inputDirection = 4;
    }

    if (g_PlayerInputButtons & 4)
    {
        player->focused = 1;
        switch (player->inputDirection)
        {
        case 4: horizontal = player->movement->focusedAxisSpeed; break;
        case 3: horizontal = -player->movement->focusedAxisSpeed; break;
        case 1: vertical = -player->movement->focusedAxisSpeed; break;
        case 2: vertical = player->movement->focusedAxisSpeed; break;
        case 5: vertical = horizontal = -player->movement->focusedDiagonalSpeed; break;
        case 7: vertical = player->movement->focusedDiagonalSpeed; horizontal = -vertical; break;
        case 6: horizontal = player->movement->focusedDiagonalSpeed; vertical = -horizontal; break;
        case 8: vertical = horizontal = player->movement->focusedDiagonalSpeed; break;
        default: break;
        }
    }
    else
    {
        player->focused = 0;
        switch (player->inputDirection)
        {
        case 4: horizontal = player->movement->unfocusedAxisSpeed; break;
        case 3: horizontal = -player->movement->unfocusedAxisSpeed; break;
        case 1: vertical = -player->movement->unfocusedAxisSpeed; break;
        case 2: vertical = player->movement->unfocusedAxisSpeed; break;
        case 5: vertical = horizontal = -player->movement->unfocusedDiagonalSpeed; break;
        case 7: vertical = player->movement->unfocusedDiagonalSpeed; horizontal = -vertical; break;
        case 6: horizontal = player->movement->unfocusedDiagonalSpeed; vertical = -horizontal; break;
        case 8: vertical = horizontal = player->movement->unfocusedDiagonalSpeed; break;
        default: break;
        }
    }

    SelectMovementAnimation(player, horizontal);
    player->previousInputX = horizontal;
    player->previousInputY = vertical;
    player->horizontalVelocity = horizontal * player->movementMultiplierX * g_FrameMultiplier;
    player->verticalVelocity = vertical * player->movementMultiplierY * g_FrameMultiplier;
    player->position[0] += player->horizontalVelocity;
    player->position[1] += player->verticalVelocity;
    if (player->position.x < g_PlayerUpdateBoundLeft)
        player->position.x = g_PlayerUpdateBoundLeft;
    else if (player->position.x > g_PlayerUpdateBoundLeft + g_PlayerUpdateBoundWidth)
        player->position.x = g_PlayerUpdateBoundLeft + g_PlayerUpdateBoundWidth;
    if (player->position.y < g_PlayerUpdateBoundTop)
        player->position.y = g_PlayerUpdateBoundTop;
    else if (player->position.y > g_PlayerUpdateBoundTop + g_PlayerUpdateBoundHeight)
        player->position.y = g_PlayerUpdateBoundTop + g_PlayerUpdateBoundHeight;

    player->killBoxTopLeft = player->position - player->killBoxHalfSize;
    player->killBoxBottomRight = player->position + player->killBoxHalfSize;
    player->grazeBoxTopLeft = player->position - player->grazeBoxHalfSize;
    player->grazeBoxBottomRight = player->position + player->grazeBoxHalfSize;
    player->itemBoxTopLeft = player->position - player->itemBoxHalfSize;
    player->itemBoxBottomRight = player->position + player->itemBoxHalfSize;
    player->orbPosition[0] = player->position;
    player->orbPosition[1] = player->position;

    verticalOrbOffset = 0.0f;
    horizontalOrbOffset = verticalOrbOffset;
    if (*reinterpret_cast<u8 *>(&g_PlayerGameMode) != 2 || g_PlayerOrbMode != 1)
    {
        switch (player->orbState)
        {
        case 0:
            ResetTimer(&player->orbTimer);
            break;
        case 1:
            horizontalOrbOffset = 24.0f;
            ResetTimer(&player->orbTimer);
            if (player->focused)
            {
                player->orbState = 2;
                player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
                goto straightFocused;
            }
            break;
        case 2:
        straightFocused:
            AdvanceTimer(&player->orbTimer);
            {
                intermediateFloat = player->orbTimer.AsFramesFloat() / 8.0f;
                verticalOrbOffset = (1.0f - intermediateFloat) * 32.0f + -32.0f;
                intermediateFloat *= intermediateFloat;
                horizontalOrbOffset = -16.0f * intermediateFloat + 24.0f;
                if ((i32)(player->orbTimer.current >= 8))
                    player->orbState = 3;
                if (!player->focused)
                {
                    player->orbState = 4;
                    player->orbTimer.SetCurrent(8 - player->orbTimer.AsFrames());
                    if (player->orbAnimation)
                        player->orbAnimation->RequestStop();
                    goto straightUnfocused;
                }
            }
            break;
        case 3:
            horizontalOrbOffset = 8.0f;
            verticalOrbOffset = -32.0f;
            ResetTimer(&player->orbTimer);
            if (!player->focused)
            {
                player->orbState = 4;
                if (player->orbAnimation)
                    player->orbAnimation->RequestStop();
                goto straightUnfocused;
            }
            break;
        case 4:
        straightUnfocused:
            AdvanceTimer(&player->orbTimer);
            {
                intermediateFloat = player->orbTimer.AsFramesFloat() / 8.0f;
                verticalOrbOffset = 32.0f * intermediateFloat + -32.0f;
                intermediateFloat *= intermediateFloat;
                intermediateFloat = 1.0f - intermediateFloat;
                horizontalOrbOffset = -16.0f * intermediateFloat + 24.0f;
                if ((i32)(player->orbTimer.current >= 8))
                    player->orbState = 1;
                if (player->focused)
                {
                    player->orbState = 2;
                    player->orbTimer.SetCurrent(8 - player->orbTimer.AsFrames());
                    player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
                    goto straightFocused;
                }
            }
            break;
        default:
            break;
        }

        player->orbPosition[0].x -= horizontalOrbOffset;
        player->orbPosition[1].x += horizontalOrbOffset;
        player->orbPosition[0].y += verticalOrbOffset;
        player->orbPosition[1].y += verticalOrbOffset;
    }
    else
    {
        switch (player->orbState)
        {
        case 0:
            ResetTimer(&player->orbTimer);
            break;
        case 1:
            {
                horizontalOrbOffset = Cos(player->rotationAngle + 1.5707964f) * 24.0f;
                verticalOrbOffset = Sin(player->rotationAngle + 1.5707964f) * 24.0f;
            }
            ResetTimer(&player->orbTimer);
            if (player->focused)
            {
                player->orbState = 2;
                player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
                goto rotatingFocused;
            }
            player->orbPosition[0].x -= horizontalOrbOffset;
            player->orbPosition[1].x += horizontalOrbOffset;
            player->orbPosition[0].y -= verticalOrbOffset;
            player->orbPosition[1].y += verticalOrbOffset;
            break;
        case 2:
        rotatingFocused:
            if (!player->focused)
            {
                player->orbState = 4;
                player->orbTimer.SetCurrent(8 - player->orbTimer.AsFrames());
                if (player->orbAnimation)
                    player->orbAnimation->RequestStop();
                goto rotatingUnfocused;
            }
            AdvanceTimer(&player->orbTimer);
            intermediateFloat = player->orbTimer.AsFramesFloat() / 8.0f;
            {
                horizontalOrbOffset = Cos(player->rotationAngle + 1.5707964f) * 24.0f;
                verticalOrbOffset = Sin(player->rotationAngle + 1.5707964f) * 24.0f;
                rotatingX = Cos(player->rotationAngle + 0.22439948f) * 24.0f;
                rotatingY = Sin(player->rotationAngle + 0.22439948f) * 24.0f;
            }
            rotatingX = (rotatingX - horizontalOrbOffset) * intermediateFloat + horizontalOrbOffset;
            rotatingY = (rotatingY - verticalOrbOffset) * intermediateFloat + verticalOrbOffset;
            player->orbPosition[1].x += rotatingX;
            player->orbPosition[1].y += rotatingY;
            {
                rotatingX = Cos(player->rotationAngle - 0.22439948f) * 24.0f;
                rotatingY = Sin(player->rotationAngle - 0.22439948f) * 24.0f;
            }
            rotatingX = (rotatingX + horizontalOrbOffset) * intermediateFloat - horizontalOrbOffset;
            rotatingY = (rotatingY + verticalOrbOffset) * intermediateFloat - verticalOrbOffset;
            if ((i32)(player->orbTimer.current >= 8))
                player->orbState = 3;
            player->orbPosition[0].x += rotatingX;
            player->orbPosition[0].y += rotatingY;
            break;
        case 3:
            ResetTimer(&player->orbTimer);
            if (!player->focused)
            {
                player->orbState = 4;
                if (player->orbAnimation)
                    player->orbAnimation->RequestStop();
                goto rotatingUnfocused;
            }
            {
                rotatingX = Cos(player->rotationAngle + 0.22439948f) * 24.0f;
                rotatingY = Sin(player->rotationAngle + 0.22439948f) * 24.0f;
            }
            player->orbPosition[1].x += rotatingX;
            player->orbPosition[1].y += rotatingY;
            {
                rotatingX = Cos(player->rotationAngle - 0.22439948f) * 24.0f;
                rotatingY = Sin(player->rotationAngle - 0.22439948f) * 24.0f;
            }
            player->orbPosition[0].x += rotatingX;
            player->orbPosition[0].y += rotatingY;
            break;
        case 4:
        rotatingUnfocused:
            if (player->focused)
            {
                player->orbState = 2;
                player->orbTimer.SetCurrent(8 - player->orbTimer.AsFrames());
                player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
                goto rotatingFocused;
            }
            AdvanceTimer(&player->orbTimer);
            intermediateFloat = 1.0f - player->orbTimer.AsFramesFloat() / 8.0f;
            {
                horizontalOrbOffset = Cos(player->rotationAngle + 1.5707964f) * 24.0f;
                verticalOrbOffset = Sin(player->rotationAngle + 1.5707964f) * 24.0f;
                rotatingX = Cos(player->rotationAngle + 0.22439948f) * 24.0f;
                rotatingY = Sin(player->rotationAngle + 0.22439948f) * 24.0f;
            }
            rotatingX = (rotatingX - horizontalOrbOffset) * intermediateFloat + horizontalOrbOffset;
            rotatingY = (rotatingY - verticalOrbOffset) * intermediateFloat + verticalOrbOffset;
            player->orbPosition[1].x += rotatingX;
            player->orbPosition[1].y += rotatingY;
            {
                rotatingX = Cos(player->rotationAngle - 0.22439948f) * 24.0f;
                rotatingY = Sin(player->rotationAngle - 0.22439948f) * 24.0f;
            }
            rotatingX = (rotatingX + horizontalOrbOffset) * intermediateFloat - horizontalOrbOffset;
            rotatingY = (rotatingY + verticalOrbOffset) * intermediateFloat - verticalOrbOffset;
            if ((i32)(player->orbTimer.current >= 8))
                player->orbState = 1;
            player->orbPosition[0].x += rotatingX;
            player->orbPosition[0].y += rotatingY;
            break;
        default:
            break;
        }
    }

    if ((g_PlayerInputButtons & 1) && !g_PlayerBombGuiState49FBF0.IsBombInputBlocked())
    {
        if (!g_PlayerBombGrazeState626270.IsBombStartBlocked())
            player->UpdateBombCollisionState();

        if (!(g_PlayerInputButtons & 4))
        {
            if (player->horizontalVelocity != 0.0f)
            {
                rotationStep = -(player->horizontalVelocity / 4.0f) * 3.1415927f / 5.0f / 10.0f;
                player->rotationAngle -= rotationStep;
                if (player->rotationAngle < -2.1991148f)
                    player->rotationAngle = -2.1991148f;
                else if (player->rotationAngle > -0.94247782f)
                    player->rotationAngle = -0.94247782f;
            }
            else
            {
                if (Abs(player->rotationAngle - -1.5707964f) > 0.031415928f)
                {
                    rotationStep = player->rotationAngle < -1.5707964f ? 0.062831856f * g_FrameMultiplier
                                                                        : -0.062831856f * g_FrameMultiplier;
                    player->rotationAngle += rotationStep;
                }
                else
                    player->rotationAngle = -1.5707964f;
            }
        }
    }
    return 0;
#undef player
}
} // namespace th07
