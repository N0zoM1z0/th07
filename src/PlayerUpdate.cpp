#include "Player.hpp"

#include <math.h>

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
};

struct EffectManager
{
    void *SpawnParticlesColored(i32 effect, D3DXVECTOR3 *position, i32 count, i32 blendMode, i32 color);
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
extern u8 g_PlayerShotType;
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
    void *orbAnimation;                   // +0x9D8
    u8 unknown9DC[0x1A14];
    f32 movementMultiplierX;              // +0x23F0
    f32 movementMultiplierY;              // +0x23F4
    u8 unknown23F8[0x12];
    u8 orbState;                          // +0x240A
    u8 focused;                           // +0x240B
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
    timer->previous = -999;
    timer->subFrame = 0.0f;
    timer->current = 0;
}

static __forceinline f32 AdvanceTimer(PlayerUpdateTimer *timer)
{
    timer->previous = timer->current;
    g_TimerManager.Advance(&timer->current, &timer->subFrameBits);
    return (f32)timer->current + timer->subFrame;
}

static __forceinline void SelectMovementAnimation(PlayerUpdateOverlay *player, f32 horizontal)
{
    if (horizontal < 0.0f)
    {
        player->movementAnimation = 1025;
        g_PlayerBombAnmManager->SetAndExecute(player, g_PlayerBombAnmManager->scripts[1025]);
    }
    else if (horizontal == 0.0f && player->previousInputX < 0.0f)
    {
        player->movementAnimation = 1026;
        g_PlayerBombAnmManager->SetAndExecute(player, g_PlayerBombAnmManager->scripts[1026]);
    }
    if (horizontal > 0.0f)
    {
        player->movementAnimation = 1027;
        g_PlayerBombAnmManager->SetAndExecute(player, g_PlayerBombAnmManager->scripts[1027]);
    }
    else if (horizontal == 0.0f && player->previousInputX > 0.0f)
    {
        player->movementAnimation = 1028;
        g_PlayerBombAnmManager->SetAndExecute(player, g_PlayerBombAnmManager->scripts[1028]);
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
        player->orbTimer.previous = player->orbTimer.current;
        AdvanceTimer(&player->orbTimer);
        {
            f32 t = ((f32)player->orbTimer.current + player->orbTimer.subFrame) / 8.0f;
            vertical = (1.0f - t) * 32.0f - 32.0f;
            left = 24.0f - 16.0f * t * t;
            if (player->orbTimer.current >= 8)
                player->orbState = 3;
            if (!player->focused)
            {
                i32 remaining = 8 - player->orbTimer.current;
                player->orbState = 4;
                ResetTimer(&player->orbTimer);
                player->orbTimer.current = remaining;
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
                *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(player->orbAnimation) + 454) = 1;
        }
        break;
    case 4:
        if (player->focused)
        {
            i32 remaining = 8 - player->orbTimer.current;
            player->orbState = 2;
            ResetTimer(&player->orbTimer);
            player->orbTimer.current = remaining;
            player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
            goto focused;
        }
        player->orbTimer.previous = player->orbTimer.current;
        AdvanceTimer(&player->orbTimer);
        {
            f32 t = ((f32)player->orbTimer.current + player->orbTimer.subFrame) / 8.0f;
            vertical = 32.0f * t - 32.0f;
            left = 24.0f - 16.0f * (1.0f - t * t);
            if (player->orbTimer.current >= 8)
                player->orbState = 1;
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

static __forceinline void UpdateRotatingOrbs(PlayerUpdateOverlay *player)
{
    const f32 piOverTwo = 1.5707964f;
    const f32 spread = 0.22439948f;
    f32 angle = player->rotationAngle;
    f32 x;
    f32 y;

    for (;;)
    {
    switch (player->orbState)
    {
    case 0:
        ResetTimer(&player->orbTimer);
        return;
    case 1:
        ResetTimer(&player->orbTimer);
        if (player->focused)
        {
            player->orbState = 2;
            player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
            goto focused;
        }
        x = cos(angle + piOverTwo) * 24.0f;
        y = sin(angle + piOverTwo) * 24.0f;
        player->orbPosition[0].x -= x;
        player->orbPosition[1].x += x;
        player->orbPosition[0].y -= y;
        player->orbPosition[1].y += y;
        return;
    case 2:
    focused:
        player->orbTimer.previous = player->orbTimer.current;
        AdvanceTimer(&player->orbTimer);
        {
            f32 t = ((f32)player->orbTimer.current + player->orbTimer.subFrame) / 8.0f;
            f32 centerX = cos(angle + piOverTwo) * 24.0f;
            f32 centerY = sin(angle + piOverTwo) * 24.0f;
            f32 outX = cos(angle + spread) * 24.0f;
            f32 outY = sin(angle + spread) * 24.0f;
            f32 inX = cos(angle - spread) * 24.0f;
            f32 inY = sin(angle - spread) * 24.0f;
            player->orbPosition[1].x += (outX - centerX) * t + centerX;
            player->orbPosition[1].y += (outY - centerY) * t + centerY;
            player->orbPosition[0].x += (inX + centerX) * t - centerX;
            player->orbPosition[0].y += (inY + centerY) * t - centerY;
            if (player->orbTimer.current >= 8)
                player->orbState = 3;
            if (!player->focused)
            {
                i32 remaining = 8 - player->orbTimer.current;
                player->orbState = 4;
                ResetTimer(&player->orbTimer);
                player->orbTimer.current = remaining;
            }
        }
        return;
    case 3:
        ResetTimer(&player->orbTimer);
        if (player->focused)
        {
            player->orbPosition[1].x += cos(angle + spread) * 24.0f;
            player->orbPosition[1].y += sin(angle + spread) * 24.0f;
            player->orbPosition[0].x += cos(angle - spread) * 24.0f;
            player->orbPosition[0].y += sin(angle - spread) * 24.0f;
            return;
        }
        player->orbState = 4;
        if (player->orbAnimation)
            *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(player->orbAnimation) + 454) = 1;
        continue;
    case 4:
        if (player->focused)
        {
            i32 remaining = 8 - player->orbTimer.current;
            player->orbState = 2;
            ResetTimer(&player->orbTimer);
            player->orbTimer.current = remaining;
            player->orbAnimation = g_EffectManager.SpawnParticlesColored(24, &player->position, 2, 1, -1);
            goto focused;
        }
        player->orbTimer.previous = player->orbTimer.current;
        AdvanceTimer(&player->orbTimer);
        {
            f32 t = 1.0f - ((f32)player->orbTimer.current + player->orbTimer.subFrame) / 8.0f;
            f32 centerX = cos(angle + piOverTwo) * 24.0f;
            f32 centerY = sin(angle + piOverTwo) * 24.0f;
            f32 outX = cos(angle + spread) * 24.0f;
            f32 outY = sin(angle + spread) * 24.0f;
            f32 inX = cos(angle - spread) * 24.0f;
            f32 inY = sin(angle - spread) * 24.0f;
            player->orbPosition[1].x += (outX - centerX) * t + centerX;
            player->orbPosition[1].y += (outY - centerY) * t + centerY;
            player->orbPosition[0].x += (inX + centerX) * t - centerX;
            player->orbPosition[0].y += (inY + centerY) * t - centerY;
            if (player->orbTimer.current >= 8)
                player->orbState = 1;
        }
        return;
    default:
        return;
    }
    }
}

int __fastcall Player::OnUpdate(Player *playerBase)
{
    PlayerUpdateOverlay *player = reinterpret_cast<PlayerUpdateOverlay *>(playerBase);
    f32 horizontal = 0.0f;
    f32 vertical = 0.0f;

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

    player->focused = (g_PlayerInputButtons & 4) != 0;
    f32 axisSpeed = player->focused ? player->movement->focusedAxisSpeed : player->movement->unfocusedAxisSpeed;
    f32 diagonalSpeed = player->focused ? player->movement->focusedDiagonalSpeed
                                        : player->movement->unfocusedDiagonalSpeed;
    switch (player->inputDirection)
    {
    case 1: vertical = -axisSpeed; break;
    case 2: vertical = axisSpeed; break;
    case 3: horizontal = -axisSpeed; break;
    case 4: horizontal = axisSpeed; break;
    case 5: horizontal = vertical = -diagonalSpeed; break;
    case 6: horizontal = diagonalSpeed; vertical = -diagonalSpeed; break;
    case 7: horizontal = -diagonalSpeed; vertical = diagonalSpeed; break;
    case 8: horizontal = vertical = diagonalSpeed; break;
    default: break;
    }

    SelectMovementAnimation(player, horizontal);
    player->previousInputX = horizontal;
    player->previousInputY = vertical;
    player->horizontalVelocity = horizontal * player->movementMultiplierX * g_FrameMultiplier;
    player->verticalVelocity = vertical * player->movementMultiplierY * g_FrameMultiplier;
    player->position.x += player->horizontalVelocity;
    player->position.y += player->verticalVelocity;
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

    if (*reinterpret_cast<u8 *>(&g_PlayerGameMode) == 2 && g_PlayerShotType == 1)
        UpdateRotatingOrbs(player);
    else
        UpdateStraightOrbs(player);

    if ((g_PlayerInputButtons & 1) && !g_PlayerBombGuiState49FBF0.IsBombInputBlocked())
    {
        if (!g_PlayerBombGrazeState626270.IsBombStartBlocked())
            player->UpdateBombCollisionState();
    }

    if (!(g_PlayerInputButtons & 4))
    {
        f32 *rotation = &player->rotationAngle;
        if (player->horizontalVelocity == 0.0f)
        {
            f32 delta = *rotation + 1.5707964f;
            f32 magnitude = delta < 0.0f ? -delta : delta;
            if (magnitude <= 0.031415928f)
                *rotation = -1.5707964f;
            else
                *rotation += *rotation >= -1.5707964f ? -0.062831856f * g_FrameMultiplier
                                                      : 0.062831856f * g_FrameMultiplier;
        }
        else
        {
            *rotation -= (-(player->horizontalVelocity / 4.0f) * 3.1415927f / 5.0f / 10.0f);
            if (*rotation < -2.1991148f)
                *rotation = -2.1991148f;
            else if (*rotation > -0.94247782f)
                *rotation = -0.94247782f;
        }
    }
    return 0;
}
} // namespace th07
