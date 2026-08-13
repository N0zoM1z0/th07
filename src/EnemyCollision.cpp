#include "Player.hpp"

namespace th07
{

extern Player g_Player;

struct EnemyCollisionTimer
{
    i32 previous;
    i32 subFrameBits;
    i32 current;

    i32 HasTicked()
    {
        return (current != previous) ? 1 : 0;
    }
};

struct EnemyCollisionOverlay
{
    u8 unknown0000[0x2BB8];
    i32 life;
    u8 unknown2BBC[8];
    EnemyCollisionTimer grazeTimer;
    u8 unknown2BD0[0x259];
    u8 interactable : 1;
    u8 unknownFlag1 : 1;
    u8 unknownFlag2 : 1;
    u8 unknownFlag3 : 1;
    u8 unknownFlag4 : 1;
    u8 suppressDirectCollision : 1;
    u8 boss : 1;
    u8 unknownFlag7 : 1;

    void CheckPlayerCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size);
};

#pragma var_order(grazeScale, grazeProduct, hitboxSize, killScale, killProduct)
void EnemyCollisionOverlay::CheckPlayerCollision(D3DXVECTOR3 *center, D3DXVECTOR3 *size)
{
    D3DXVECTOR3 hitboxSize;

    hitboxSize = *size / 0.7f;
    if (suppressDirectCollision)
    {
        if (grazeTimer.HasTicked() && grazeTimer.current % 6 == 0)
            g_Player.CheckGraze(center, &hitboxSize);
    }

    hitboxSize = *size / 1.5f;
    if (g_Player.CalcKillBoxCollision(center, &hitboxSize) == 1 && interactable && !boss && !suppressDirectCollision)
        life -= 10;
}

} // namespace th07
