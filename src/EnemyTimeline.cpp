#include "Rng.hpp"
#include "Timer.hpp"

namespace th07
{

struct EnemyTimelineVec3
{
    f32 x;
    f32 y;
    f32 z;
};

struct EnemyTimelineArgs
{
    u32 uintVar1;
    u32 uintVar2;
    u32 uintVar3;
    u32 uintVar4;
    u32 uintVar5;
    u32 uintVar6;

    EnemyTimelineVec3 *AsVec()
    {
        return reinterpret_cast<EnemyTimelineVec3 *>(&uintVar1);
    }
};

struct EnemyTimelineInstruction
{
    i16 time;
    i16 arg0;
    i16 opcode;
    i16 size;
    EnemyTimelineArgs args;
};

struct EnemyTimelineEnemy
{
    u8 unknown0000[0x2B08];
    i32 runInterrupt;
    u8 unknown2B0C[0x31C];
    u8 unknownSlotFlags : 6;
    u8 invertX : 1;
    u8 isSlotOccupied : 1;
};

struct EnemyTimelineManager
{
    EnemyTimelineEnemy *SpawnEnemy(i32 eclSubId, EnemyTimelineVec3 *position,
                                    i32 life, i32 itemDrop, i32 score, i32 mirrored);
};

struct EnemyTimelineGui
{
    u8 unknown000[0x24];
    u8 bossPresent;

    u8 BossPresent()
    {
        return bossPresent;
    }

    void MsgRead(i32 messageId);
    i32 MsgWait();
};

struct EnemyTimelinePowerState
{
    void ApplyPower();
};

struct EnemyTimelineGameManager
{
    u8 unknown000[0x7C];
    f32 currentPower;
};

struct EnemyTimelineArea
{
    f32 width;
    f32 height;
};

struct EnemyTimelineLane
{
    ZunTimer timer;
    EnemyTimelineInstruction *instruction;

    void Run();
};

extern EnemyTimelineManager g_EnemyTimelineManager;
extern EnemyTimelineGui g_EnemyTimelineGui;
extern Rng g_EnemyTimelineRng;
extern EnemyTimelineEnemy *g_EnemyTimelineBosses[8];
extern EnemyTimelinePowerState g_EnemyTimelinePowerState;
extern EnemyTimelineGameManager *g_EnemyTimelineGameManager;
extern TimerManager g_EnemyTimelineTimerManager;
extern u8 g_EnemyTimelineGameMode;
extern EnemyTimelineArea g_EnemyTimelineArea;
extern const f32 g_EnemyTimelineRandomSentinel;

inline void ApplyTimelinePower(i32 power)
{
    g_EnemyTimelineGameManager->currentPower = (f32)power;
    g_EnemyTimelinePowerState.ApplyPower();
}

#pragma var_order(spawnedEnemy, args1, args2, args3, position1, position2, args4, position3, position4)
#pragma optimize("p", on)
void EnemyTimelineLane::Run()
{
    EnemyTimelineVec3 position4;
    EnemyTimelineVec3 position3;
    EnemyTimelineArgs *args4;
    EnemyTimelineVec3 position2;
    EnemyTimelineVec3 position1;
    EnemyTimelineArgs *args3;
    EnemyTimelineArgs *args2;
    EnemyTimelineArgs *args1;
    EnemyTimelineEnemy *spawnedEnemy;

    while (instruction->time >= 0)
    {
        if ((timer.current == instruction->time) ? 1 : 0)
        {
            switch (instruction->opcode)
            {
            case 0:
                if (!g_EnemyTimelineGui.BossPresent())
                {
                    args1 = &instruction->args;
                    g_EnemyTimelineManager.SpawnEnemy(instruction->arg0,
                        args1->AsVec(), args1->uintVar4,
                        args1->uintVar5, args1->uintVar6, 0);
                }
                break;
            case 1:
                if (!g_EnemyTimelineGui.BossPresent())
                    g_EnemyTimelineManager.SpawnEnemy(instruction->arg0,
                        instruction->args.AsVec(), -1, -1, -1, 0);
                break;
            case 2:
                if (!g_EnemyTimelineGui.BossPresent())
                {
                    args2 = &instruction->args;
                    spawnedEnemy = g_EnemyTimelineManager.SpawnEnemy(
                        instruction->arg0, args2->AsVec(), args2->uintVar4,
                        args2->uintVar5, args2->uintVar6, 1);
                }
                break;
            case 3:
                if (!g_EnemyTimelineGui.BossPresent())
                    spawnedEnemy = g_EnemyTimelineManager.SpawnEnemy(
                        instruction->arg0, instruction->args.AsVec(),
                        -1, -1, -1, 1);
                break;
            case 4:
                if (!g_EnemyTimelineGui.BossPresent())
                {
                    args3 = &instruction->args;
                    position1 = *args3->AsVec();
                    if (args3->AsVec()->x <= g_EnemyTimelineRandomSentinel)
                        position1.x = g_EnemyTimelineRng.GetRandomF32InRange(g_EnemyTimelineArea.width);
                    if (args3->AsVec()->y <= g_EnemyTimelineRandomSentinel)
                        position1.y = g_EnemyTimelineRng.GetRandomF32InRange(g_EnemyTimelineArea.height);
                    if (args3->AsVec()->z <= g_EnemyTimelineRandomSentinel)
                        position1.z = g_EnemyTimelineRng.GetRandomF32InRange(800.0f);
                    g_EnemyTimelineManager.SpawnEnemy(instruction->arg0,
                        &position1, args3->uintVar4, args3->uintVar5,
                        args3->uintVar6, 0);
                }
                break;
            case 5:
                if (!g_EnemyTimelineGui.BossPresent())
                {
                    position2 = *instruction->args.AsVec();
                    if (position2.x <= g_EnemyTimelineRandomSentinel)
                        position2.x = g_EnemyTimelineRng.GetRandomF32InRange(g_EnemyTimelineArea.width);
                    if (position2.y <= g_EnemyTimelineRandomSentinel)
                        position2.y = g_EnemyTimelineRng.GetRandomF32InRange(g_EnemyTimelineArea.height);
                    if (position2.z <= g_EnemyTimelineRandomSentinel)
                        position2.z = g_EnemyTimelineRng.GetRandomF32InRange(800.0f);
                    g_EnemyTimelineManager.SpawnEnemy(instruction->arg0,
                        &position2, -1, -1, -1, 0);
                }
                break;
            case 6:
                if (!g_EnemyTimelineGui.BossPresent())
                {
                    args4 = &instruction->args;
                    position3 = *args4->AsVec();
                    if (args4->AsVec()->x <= g_EnemyTimelineRandomSentinel)
                        position3.x = g_EnemyTimelineRng.GetRandomF32InRange(g_EnemyTimelineArea.width);
                    if (args4->AsVec()->y <= g_EnemyTimelineRandomSentinel)
                        position3.y = g_EnemyTimelineRng.GetRandomF32InRange(g_EnemyTimelineArea.height);
                    if (args4->AsVec()->z <= g_EnemyTimelineRandomSentinel)
                        position3.z = g_EnemyTimelineRng.GetRandomF32InRange(800.0f);
                    spawnedEnemy = g_EnemyTimelineManager.SpawnEnemy(
                        instruction->arg0, &position3, args4->uintVar4,
                        args4->uintVar5, args4->uintVar6, 0);
                    spawnedEnemy->invertX = 1;
                }
                break;
            case 7:
                if (!g_EnemyTimelineGui.BossPresent())
                {
                    position4 = *instruction->args.AsVec();
                    if (position4.x <= g_EnemyTimelineRandomSentinel)
                        position4.x = g_EnemyTimelineRng.GetRandomF32InRange(g_EnemyTimelineArea.width);
                    if (position4.y <= g_EnemyTimelineRandomSentinel)
                        position4.y = g_EnemyTimelineRng.GetRandomF32InRange(g_EnemyTimelineArea.height);
                    if (position4.z <= g_EnemyTimelineRandomSentinel)
                        position4.z = g_EnemyTimelineRng.GetRandomF32InRange(800.0f);
                    spawnedEnemy = g_EnemyTimelineManager.SpawnEnemy(
                        instruction->arg0, &position4, -1, -1, -1, 0);
                    spawnedEnemy->invertX = 1;
                }
                break;
            case 8:
                g_EnemyTimelineGui.MsgRead(
                    instruction->arg0 + 10 * (u8)g_EnemyTimelineGameMode);
                break;
            case 9:
                if (g_EnemyTimelineGui.MsgWait())
                {
                    timer.Decrement(1);
                    goto done;
                }
                break;
            case 10:
                g_EnemyTimelineBosses[instruction->args.uintVar1]->runInterrupt =
                    instruction->args.uintVar2;
                break;
            case 11:
                ApplyTimelinePower(instruction->arg0);
                break;
            case 12:
                if (g_EnemyTimelineBosses[instruction->arg0] &&
                    g_EnemyTimelineBosses[instruction->arg0]->isSlotOccupied)
                {
                    timer.Decrement(1);
                    goto done;
                }
                break;
            }
        }
        else if ((timer.current < instruction->time) ? 1 : 0)
        {
            break;
        }
        instruction = reinterpret_cast<EnemyTimelineInstruction *>(
            reinterpret_cast<u8 *>(instruction) + instruction->size);
    }

done:
    timer.previous = timer.current;
    g_EnemyTimelineTimerManager.Advance(&timer.current, &timer.subFrame);
}
#pragma optimize("p", off)

} // namespace th07
