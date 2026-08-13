#include "EclManager.hpp"
#include "Timer.hpp"

#include <d3dx8math.h>
#include <math.h>
#include <string.h>

extern "C" f64 __cdecl _CIatan2();
extern "C" f32 __cdecl TargetCISqrt();
#pragma intrinsic(atan2)

namespace th07
{
// The active difficulty bitmask is read at 0x004106EC.  Its owner and public
// name belong to the game-state lane, so retain the target-address label.
extern u8 g_TargetDifficultyMask626284;
extern EclManager g_TargetEclManager1347938;
extern i32 g_TargetSpellActive12FE0C8;
extern i32 g_TargetCaptureEligible12FE0C4;
extern i32 g_TargetSpellBaseScore12FE0CC;
extern i32 g_TargetSpellPerTick12FE0D4;
extern i32 g_TargetSpellId12FE0D8;
extern i32 g_TargetSpellTimerPrevious12FE0E0;
extern i32 g_TargetSpellTimerCurrent12FE0E4;
extern i32 g_TargetSpellTimerSubframe12FE0E8;
extern i32 g_TargetSpellScores49F1B8[];
extern i32 g_TargetDifficulty62F85C;
extern i32 g_TargetControl4D44F8;
// Target-observed ECL spell-control word at 0x00413CEF (opcode 0x93).
extern i32 g_TargetSpellControl12FE0F0;
// Target-observed opcode 0x8C float control words at 0x00416106--0x004161BC.
extern f32 g_TargetSpellFloat12FE25C;
extern f32 g_TargetSpellFloat12FE260;
extern f32 g_TargetSpellFloat12FE264;
extern f32 g_TargetSpellFloat12FE268;
// Target-observed ECL presentation/control globals.
extern f32 g_TargetEclSlotFloatX49FC24[8];
extern f32 g_TargetEclSlotFloatY49FC44[8];
extern i32 g_TargetEclSlotValue49FC64[8];
extern i32 g_TargetEclControl134CBF4;
extern i32 g_TargetEclControl49FC08;
extern i32 g_TargetEclTimer62F898;
extern u8 g_TargetScoreState626278[];
// Target-observed boss control/UI globals used by opcode 0x63.
extern u8 g_TargetBossPresent49FC14;
extern f32 g_TargetBossHealth49FC18;
extern i16 g_TargetBossUi134DB5A[];
extern i32 g_TargetRank62F8A4;
extern u8 g_TargetBulletManager62F958[];
// The target reads this game-configuration word in the opcode-121 rotation
// handler.  Its wider ownership remains with the ECL operand lane.
namespace EclOperands
{
extern i32 g_TargetInt626280;
}
struct EclTailTimerManager
{
    void Advance(i32 *current, i32 *subFrame);
};
extern EclTailTimerManager g_EclTailTimerManager;
struct EffectManager
{
    void *SpawnParticles(i32 effect, D3DXVECTOR3 *position, i32 count, i32 color);
};
extern EffectManager g_EffectManager;
extern void *g_EffectsColor[];

// ABI corroborated by the target-pinned BulletUpdate unit (0x004326F0).
struct BulletUpdateVec3
{
    f32 x;
    f32 y;
    f32 z;
};
struct BulletUpdateItemManager
{
    void Spawn(BulletUpdateVec3 *position, i32 itemType, i32 count);
};
extern BulletUpdateItemManager g_ItemManager;

namespace EclOperands
{

// This declaration is deliberately the same private overlay used by
// EclOperands.cpp.  RunEcl only passes it to the already-recovered resolver
// helpers; it does not claim a complete Enemy layout.
struct EnemyOverlay
{
    u8 bytes[1];

    f32 ResolveFloat(f32 operand);
};

struct Vector3
{
    f32 x;
    f32 y;
    f32 z;
};

struct TargetPlayerOverlay
{
    f32 AngleToPlayer(const Vector3 *position);
};

struct TargetRngOverlay
{
    u16 GetRandomU16();
    u32 RandomU32();
    f32 RandomF32();
};

extern TargetPlayerOverlay g_TargetPlayer4BDAD8;
extern TargetRngOverlay g_TargetRng49FE20;
extern f32 g_TargetFloat4BE408;

i32 __fastcall ResolveInt(EnemyOverlay *enemy, i32 operand);
i32 *__fastcall ResolveIntLValue(EnemyOverlay *enemy, i32 *operand, u16 flags, i32 flagIndex);
f32 *__fastcall ResolveFloatLValue(EnemyOverlay *enemy, f32 *operand, u16 flags, i32 flagIndex);

} // namespace EclOperands

void __fastcall TargetClampEnemy(EclOperands::EnemyOverlay *enemy);
void __cdecl TargetSpawnBullet(void *manager, void *request);
void *__cdecl TargetSpawnLaser(void *manager, void *request);
void __cdecl TargetClearBullets(void *manager, i32 mode);
f32 __stdcall TargetAddNormalizeAngle(f32 angle, f32 delta);
void __cdecl TargetEffect423090(i32 effectId, i32 parameter);
void __cdecl Target44C930(i32 soundId, i32 parameter);
void __cdecl Target439401(i32 value);
void __cdecl Target424C00(f32 *position, f32 value);
void __cdecl Target42F5A2(i32 value);
void __cdecl DebugPrint(const char *format, ...);

namespace SpellLifecycle
{
struct EnemyOverlay
{
    u8 bytes[1];

    void UnregisterBoss();
};
struct SpellVmOverlay
{
    u8 bytes[1];
};
struct AnmManagerOverlay
{
    u8 bytes[1];

    void SetAndExecuteScript(void *vm, void *script);
    void ConfigureBoss(EnemyOverlay *enemy, void *source, i32 value);
};
struct SpellStartInstruction;
u32 __fastcall StartSpellcard(EnemyOverlay *enemy, const SpellStartInstruction *instruction);
void __fastcall FinishSpellcard(EnemyOverlay *enemy, const void *instruction);
extern u8 *g_TargetAnmManager4B9E44;
extern EnemyOverlay *g_TargetSpellBosses12FE098[8];
} // namespace SpellLifecycle

struct EclOp121InstructionOverlay
{
    u8 raw00[0x10];
    i32 rawParameter10;
};

struct EclResetSlot41AE90
{
    u8 bytes[0x2D8];
};
typedef char EclResetSlot41AE90_size[(sizeof(EclResetSlot41AE90) == 0x2D8) ? 1 : -1];

extern EclResetSlot41AE90 g_TargetResetSlots12FE26C[400];

struct EclOp121MovementBits
{
    u8 copyTransform : 1;
    u8 unknown : 7;
};

#pragma var_order(bossIndex)
void __fastcall EclOp121CopyBossTransform(EclOperands::EnemyOverlay *enemy,
                                          const EclOp121InstructionOverlay *instruction)
{
    i32 bossIndex;

    bossIndex = instruction->rawParameter10;
    *reinterpret_cast<EclOperands::Vector3 *>(enemy->bytes + 0x2B0C) =
        *reinterpret_cast<const EclOperands::Vector3 *>(
            SpellLifecycle::g_TargetSpellBosses12FE098[bossIndex]->bytes + 0x2B0C);
    *reinterpret_cast<EclOperands::Vector3 *>(enemy->bytes + 0x2B18) =
        *reinterpret_cast<const EclOperands::Vector3 *>(
            SpellLifecycle::g_TargetSpellBosses12FE098[bossIndex]->bytes + 0x2B18);
    *reinterpret_cast<f32 *>(enemy->bytes + 0x2B54) =
        *reinterpret_cast<const f32 *>(
            SpellLifecycle::g_TargetSpellBosses12FE098[bossIndex]->bytes + 0x2B54);
    reinterpret_cast<EclOp121MovementBits *>(enemy->bytes + 0x2E2B)->copyTransform = 1;
}

// Target 0x00418110 is an intentional empty opcode-121 table entry.  The
// unoptimized VC7 parameter homes are part of its observed 16-byte body.
void __fastcall EclOp121NoOp(EclOperands::EnemyOverlay *enemy,
                             const EclOp121InstructionOverlay *instruction)
{
}

// These overlays deliberately remain local to the opcode-121 helpers.  They
// express only fields and member-call ABIs directly observed in their target
// bodies; BulletManager's public layout remains owned by the bullet lane.
struct EclOp121BulletCommand
{
    f32 unknown00;
    u8 unknown04[0x14];
};
typedef char EclOp121BulletCommand_size[sizeof(EclOp121BulletCommand) == 0x18 ? 1 : -1];

struct EclOp121StateCommand
{
    f32 parameter;
    f32 angle;
    i32 duration;
    u8 unknown0C[4];
};
typedef char EclOp121StateCommand_size[sizeof(EclOp121StateCommand) == 0x10 ? 1 : -1];

struct EclOp121RotationCommand
{
    f32 interval;
    f32 angularStep;
    i32 duration;
    u8 unknown0C[0x14];
};
typedef char EclOp121RotationCommand_size[sizeof(EclOp121RotationCommand) == 0x20 ? 1 : -1];

struct EclOp121LargeCommand
{
    f32 angle;
    f32 parameter;
    i32 duration;
    u8 unknown0C[0x14];
};
typedef char EclOp121LargeCommand_size[sizeof(EclOp121LargeCommand) == 0x20 ? 1 : -1];

struct EclOp121WideCommand
{
    f32 unknown00;
    f32 angle;
    i32 duration;
    i32 unknown0C;
    u8 unknown10[0x70];
};
typedef char EclOp121WideCommand_size[sizeof(EclOp121WideCommand) == 0x80 ? 1 : -1];

struct EclOp121SpawnCommand
{
    f32 angle;
    f32 angularStep;
    i32 duration;
    u8 unknown0C[4];
};
typedef char EclOp121SpawnCommand_size[sizeof(EclOp121SpawnCommand) == 0x10 ? 1 : -1];

struct EclOp121HugeCommand
{
    u8 unknown00[8];
    i32 value08;
};

struct EclOp121Bullet
{
    union
    {
        u8 bytes[1];
        struct
        {
            u8 unknown000[0xC24];
            EclOp121BulletCommand commands[2];
        };
    };

    void QueueRotationCommand(f32 speedDelta, f32 unusedDelta, i32 duration,
                              f32 angularStep, f32 interval);
    EclOp121RotationCommand *ReserveRotationCommand(f32 speedDelta, f32 unusedDelta, i32 size);
    EclOp121LargeCommand *ReserveLargeCommand(i32 unknown0, i32 unknown1, i32 size);
    EclOp121WideCommand *ReserveWideCommand(i32 unknown0, i32 unknown1, i32 size);
    EclOp121HugeCommand *ReserveHugeCommand(i32 unknown0, i32 unknown1, i32 size);
    EclOp121StateCommand *ReserveStateCommand(i32 unknown0, i32 unknown1, i32 size);
    void QueueLargeCommand(i32 unknown0, i32 unknown1, i32 duration, f32 parameter, f32 angle);
    void QueueWideCommand(i32 unknown0, i32 unknown1, i32 duration, i32 unknown0C,
                          f32 unknown00, f32 angle);
    void QueueHugeCommand(i32 unknown0, i32 unknown1, i32 value08);
    void QueueStateCommand(i32 unknown0, i32 unknown1, i32 duration, f32 parameter, f32 angle);
    __forceinline EclOp121BulletCommand &CommandAt(i32 index)
    {
        return commands[index];
    }
    void Clear();
};

struct EclOp121BulletSpawnRequest
{
    i16 templateIndex;
    i16 baseSpriteIndex;
    EclOperands::Vector3 position;
    f32 angleOffset;
    f32 angleStep;
    f32 speedStart;
    u8 unknown1C[0xA0];
    i16 columns;
    i16 rows;
    u16 layoutMode;
    u16 unknownC2;
    i32 flags;
    u8 unknownC8[4];
    i32 commandOwner;
    u8 unknownD0[4];

    void QueueSpawnCommand(i32 unused0, i32 unused1, i32 duration,
                           f32 angle, f32 angularStep);
    EclOp121SpawnCommand *ReserveSpawnCommand(i32 unused0, i32 unused1, i32 size);
};
typedef char EclOp121BulletSpawnRequest_size[
    sizeof(EclOp121BulletSpawnRequest) == 0xD4 ? 1 : -1];

struct EclOp121BulletManager
{
    void Spawn(EclOp121BulletSpawnRequest *request);
};

struct EclOp121AnmManager
{
    void ResetBulletAnimation(EclOp121Bullet *bullet, i32 spriteIndex);
};

struct EclOp121TimerManager
{
    void ApplyTime(f32 value);
    i32 SelectBgm(i32 index);
    void PlayBgm(const char *path);
};

extern EclOp121AnmManager *g_TargetAnmManager4B9E44;
extern f32 g_TargetFrameMultiplier575AC8;
extern f32 g_TargetRealOne498A54;
extern EclOp121TimerManager g_TargetTimerManager575950;

void EclOp121Bullet::QueueRotationCommand(f32 speedDelta, f32 unusedDelta, i32 duration,
                                           f32 angularStep, f32 interval)
{
    EclOp121RotationCommand *command;

    command = ReserveRotationCommand(speedDelta, unusedDelta, 0x20);
    command->duration = duration;
    command->interval = interval;
    command->angularStep = angularStep;
}

void EclOp121Bullet::QueueLargeCommand(i32 unknown0, i32 unknown1, i32 duration,
                                        f32 parameter, f32 angle)
{
    EclOp121LargeCommand *command;

    command = ReserveLargeCommand(unknown0, unknown1, 0x20);
    command->duration = duration;
    command->angle = angle;
    command->parameter = parameter;
}

void EclOp121Bullet::QueueWideCommand(i32 unknown0, i32 unknown1, i32 duration,
                                       i32 unknown0C, f32 unknown00, f32 angle)
{
    EclOp121WideCommand *command;

    command = ReserveWideCommand(unknown0, unknown1, 0x80);
    command->duration = duration;
    command->unknown0C = unknown0C;
    command->unknown00 = unknown00;
    command->angle = angle;
}

void EclOp121Bullet::QueueHugeCommand(i32 unknown0, i32 unknown1, i32 value08)
{
    EclOp121HugeCommand *command;

    command = ReserveHugeCommand(unknown0, unknown1, 0x2000);
    command->value08 = value08;
}

void EclOp121Bullet::QueueStateCommand(i32 unknown0, i32 unknown1, i32 duration,
                                        f32 parameter, f32 angle)
{
    EclOp121StateCommand *command;

    command = ReserveStateCommand(unknown0, unknown1, 0x10);
    command->duration = duration;
    command->parameter = parameter;
    command->angle = angle;
}

void EclOp121BulletSpawnRequest::QueueSpawnCommand(i32 unused0, i32 unused1, i32 duration,
                                                     f32 angle, f32 angularStep)
{
    EclOp121SpawnCommand *command;

    command = ReserveSpawnCommand(unused0, unused1, 0x10);
    command->duration = duration;
    command->angle = angle;
    command->angularStep = angularStep;
}

void __fastcall Target44B310(i32 first, i32 second, i32 third, i32 fourth, i32 fifth);

#pragma var_order(slot, i)
void __cdecl Target41AE90()
{
    i32 i;
    EclResetSlot41AE90 *slot;

    slot = g_TargetResetSlots12FE26C;
    for (i = 0; i < 400; ++i, ++slot)
    {
        if (*(i8 *)(slot->bytes + 0x2CD) == 30)
            *(f32 *)(slot->bytes + 0x278) = -0.01f;
    }
}

#pragma var_order(i, bullet, angularStep)
void __fastcall EclOp121RotateBullets(EclOperands::EnemyOverlay *enemy,
                                      const EclOp121InstructionOverlay *instruction)
{
    f32 angularStep;
    EclOp121Bullet *bullet;
    i32 i;

    bullet = reinterpret_cast<EclOp121Bullet *>(0x0063B218);
    Target44B310(1, 30, 12, 0, 0);
    Target44B310(3, 4, 3, 0x80FFCFCF, 0);

    for (i = 0; i < 1024; i++, bullet = reinterpret_cast<EclOp121Bullet *>(bullet->bytes + 0xD68))
    {
        if (*(u16 *)(bullet->bytes + 0xBFC) == 0 || *(u16 *)(bullet->bytes + 0xBFC) == 5)
            continue;
        if (*(void **)(bullet->bytes + 0x1E4) != 0 && *(i32 *)(bullet->bytes + 0xC08) == 0)
        {
            if (instruction->rawParameter10 == 1 && *(i16 *)(bullet->bytes + 0xBF8) != 8)
                continue;
            if (instruction->rawParameter10 == 2 && *(i16 *)(bullet->bytes + 0xBF8) != 4)
                continue;

            if (*(i16 *)(bullet->bytes + 0xBF8) == 2)
                angularStep = -3.1415927f /
                              (EclOperands::g_TargetRng49FE20.RandomF32() * 60.0f + 180.0f);
            else if (*(i16 *)(bullet->bytes + 0xBF8) == 6)
                angularStep = 3.1415927f /
                              (EclOperands::g_TargetRng49FE20.RandomF32() * 60.0f + 180.0f);
            else if (*(i16 *)(bullet->bytes + 0xBF8) == 8)
                angularStep = 3.1415927f /
                              (EclOperands::g_TargetRng49FE20.RandomF32() * 60.0f + 180.0f);
            else if (*(i16 *)(bullet->bytes + 0xBF8) == 4)
                angularStep = -3.1415927f /
                              (EclOperands::g_TargetRng49FE20.RandomF32() * 60.0f + 180.0f);

            *(f32 *)(bullet->bytes + 0xBB0) = 0.3f;
            memset(bullet->bytes + 0xC14, 0, 0x78);
            if (EclOperands::g_TargetInt626280 < 3)
                bullet->QueueRotationCommand(0.0f, 0.0f, 60, angularStep, 0.016666668f);
            else
                bullet->QueueRotationCommand(0.0f, 0.0f, 240, angularStep, 0.0052631579f);
            *(i32 *)(bullet->bytes + 0xC08) = 1;
        }
    }
}

#pragma var_order(request, i, bullet, unusedInstructionParameter, targetSlotEC, targetSlotE8)
void __fastcall EclOp121FindLargeBullet(EclOperands::EnemyOverlay *enemy,
                                        const EclOp121InstructionOverlay *instruction)
{
    i32 unusedInstructionParameter;
    // Target-observed unaccessed debug-frame dwords at EBP-0xEC/-0xE8.
    i32 targetSlotEC;
    i32 targetSlotE8;
    EclOp121Bullet *bullet;
    i32 i;
    EclOp121BulletSpawnRequest request;

    bullet = reinterpret_cast<EclOp121Bullet *>(0x0063B218);
    memset(&request, 0, sizeof(request));
    request.commandOwner = -1;
    unusedInstructionParameter = instruction->rawParameter10;
    *(f32 *)(enemy->bytes + 0x70C) = -999.0f;

    for (i = 0; i < 1024; i++, bullet = reinterpret_cast<EclOp121Bullet *>(bullet->bytes + 0xD68))
    {
        if (*(u16 *)(bullet->bytes + 0xBFC) == 0 || *(u16 *)(bullet->bytes + 0xBFC) == 5)
            continue;
        if (*(f32 *)(*(u8 **)(bullet->bytes + 0x1E4) + 0x2C) >= 60.0f)
        {
            *(f32 *)(enemy->bytes + 0x70C) = *(f32 *)(bullet->bytes + 0xB8C);
            *(f32 *)(enemy->bytes + 0x710) = *(f32 *)(bullet->bytes + 0xB90);
            g_EffectManager.SpawnParticles(2, reinterpret_cast<D3DXVECTOR3 *>(bullet->bytes + 0xB8C), 1, -1);
            bullet->Clear();
            break;
        }
    }
}

void __fastcall EclOp121CopyPrimaryBossState(EclOperands::EnemyOverlay *enemy,
                                             const EclOp121InstructionOverlay *instruction)
{
    SpellLifecycle::EnemyOverlay *boss;
    i32 returnValue;

    boss = SpellLifecycle::g_TargetSpellBosses12FE098[0];
    *reinterpret_cast<EclOperands::Vector3 *>(enemy->bytes + 0x2B8C) =
        *reinterpret_cast<const EclOperands::Vector3 *>(boss->bytes + 0x2B0C);
    *reinterpret_cast<i32 *>(enemy->bytes + 0x2B6C) =
        *reinterpret_cast<const i32 *>(boss->bytes + 0x2B6C);
    *reinterpret_cast<i32 *>(enemy->bytes + 0x2B60) =
        *reinterpret_cast<const i32 *>(boss->bytes + 0x2B60);
    returnValue = *reinterpret_cast<i32 *>(boss->bytes + 0x72C);
}

void __fastcall EclOp121ResetBulletFamily(EclOperands::EnemyOverlay *enemy,
                                          const EclOp121InstructionOverlay *instruction)
{
    Target44B310(1, 0x50, 8, 0, 0);
    Target41AE90();
}

void __fastcall EclOp121CallTarget44B310(EclOperands::EnemyOverlay *enemy,
                                         const EclOp121InstructionOverlay *instruction)
{
    Target44B310(3, instruction->rawParameter10, 1, 0xD0CFCFFF, 0);
}

void __fastcall EclOp121ApplyTimer3(EclOperands::EnemyOverlay *enemy,
                                    const EclOp121InstructionOverlay *instruction)
{
    g_TargetTimerManager575950.ApplyTime(3.0f);
}

void __fastcall EclOp121PlayBgm13B(EclOperands::EnemyOverlay *enemy,
                                   const EclOp121InstructionOverlay *instruction)
{
    if (g_TargetTimerManager575950.SelectBgm(2))
        g_TargetTimerManager575950.PlayBgm("bgm/th07_13b.mid");
}

#pragma var_order(i, bullet)
void __fastcall EclOp121AdvanceStateOneBullets(EclOperands::EnemyOverlay *enemy,
                                               const EclOp121InstructionOverlay *instruction)
{
    EclOp121Bullet *bullet;
    i32 i;

    bullet = reinterpret_cast<EclOp121Bullet *>(0x0063B218);
    Target44B310(3, 16, 1, 0x50CFCFFF, 0);
    for (i = 0; i < 1024; i++, bullet = reinterpret_cast<EclOp121Bullet *>(bullet->bytes + 0xD68))
    {
        if (*(u16 *)(bullet->bytes + 0xBFC) == 0)
            continue;
        if (*(i32 *)(bullet->bytes + 0xC08) == 1)
        {
            bullet->QueueStateCommand(
                0, 0, 90, 0.026666667f,
                EclOperands::g_TargetPlayer4BDAD8.AngleToPlayer(
                    reinterpret_cast<const EclOperands::Vector3 *>(bullet->bytes + 0xB8C)));
            bullet->CommandAt(1).unknown00 = 0.0f;
            *(i32 *)(bullet->bytes + 0xC08) = 2;
        }
    }
}

#pragma var_order(i, bullet)
void __fastcall EclOp121CountSprite636(EclOperands::EnemyOverlay *enemy,
                                       const EclOp121InstructionOverlay *instruction)
{
    EclOp121Bullet *bullet;
    i32 i;

    bullet = reinterpret_cast<EclOp121Bullet *>(0x0063B218);
    *reinterpret_cast<i32 *>(enemy->bytes + 0x6FC) = 0;
    for (i = 0; i < 1024; i++, bullet = reinterpret_cast<EclOp121Bullet *>(bullet->bytes + 0xD68))
    {
        if (*(u16 *)(bullet->bytes + 0xBFC) == 0)
            continue;
        if (*(i32 *)(bullet->bytes + 0xC08) == 0)
        {
            if (*(i16 *)(bullet->bytes + 0x1D4) == 636)
                ++*reinterpret_cast<i32 *>(enemy->bytes + 0x6FC);
        }
    }
}

#pragma var_order(i, bullet, multiplier, velocity)
void __fastcall EclOp121ScaleBullets(EclOperands::EnemyOverlay *enemy,
                                     const EclOp121InstructionOverlay *instruction)
{
    EclOperands::Vector3 *velocity;
    f32 multiplier;
    EclOp121Bullet *bullet;
    i32 i;

    __asm {
        mov eax, instruction
        fild dword ptr [eax + 10h]
        fdivr dword ptr [g_TargetRealOne498A54]
        fstp dword ptr [g_TargetFrameMultiplier575AC8]
    }
    *reinterpret_cast<i16 *>(0x013481EE) = 2;
    *reinterpret_cast<i16 *>(0x0134843A) = 2;
    bullet = reinterpret_cast<EclOp121Bullet *>(0x0063B218);
    for (i = 0; i < 1024; i++, bullet = reinterpret_cast<EclOp121Bullet *>(bullet->bytes + 0xD68))
    {
        if (*(u16 *)(bullet->bytes + 0xBFC) == 0)
            continue;

        multiplier = g_TargetFrameMultiplier575AC8;
        velocity = reinterpret_cast<EclOperands::Vector3 *>(bullet->bytes + 0xB98);
        velocity->x = multiplier * velocity->x;
        velocity->y = multiplier * velocity->y;
        velocity->z = multiplier * velocity->z;
        *(i16 *)(bullet->bytes + 0x1D6) = *(i16 *)(bullet->bytes + 0x1D4);
        if (*(i16 *)(bullet->bytes + 0x1D4) >= 608 && *(i16 *)(bullet->bytes + 0x1D4) <= 623)
            g_TargetAnmManager4B9E44->ResetBulletAnimation(bullet, 623);
    }
}

#pragma var_order(modeForSwitch, instructionMode, squaredDistance, sqrtTemporary, \
                  distance, radius, bullet, i, request)
void __fastcall EclOp121CancelBulletsInRadius(EclOperands::EnemyOverlay *enemy,
                                              const EclOp121InstructionOverlay *instruction)
{
    i32 modeForSwitch;
    i32 instructionMode;
    f32 squaredDistance;
    f32 sqrtTemporary;
    f32 distance;
    f32 radius;
    EclOp121Bullet *bullet;
    i32 i;
    EclOp121BulletSpawnRequest request = {0};

    bullet = reinterpret_cast<EclOp121Bullet *>(0x0063B218);
    request.commandOwner = -1;
    instructionMode = instruction->rawParameter10;
    modeForSwitch = instructionMode;
    switch ((u32)modeForSwitch)
    {
    case 0:
        Target44B310(1, 0x20, 12, 0, 0);
        Target44B310(3, 4, 1, 0x80CFCFFF, 0);
        radius = 128.0f;
        break;
    case 1:
        radius = 192.0f;
        break;
    case 2:
        radius = 256.0f;
        break;
    case 3:
        radius = 999.0f;
        break;
    }

    for (i = 0; i < 1024; i++, bullet = reinterpret_cast<EclOp121Bullet *>(bullet->bytes + 0xD68))
    {
        if (*(u16 *)(bullet->bytes + 0xBFC) == 0 || *(u16 *)(bullet->bytes + 0xBFC) == 5)
            continue;
        if (*(void **)(bullet->bytes + 0x1E4) == 0)
            continue;
        if (*(i16 *)(bullet->bytes + 0xBF8) != 2)
            continue;

        squaredDistance =
            (*(f32 *)(enemy->bytes + 0x2B10) - *(f32 *)(bullet->bytes + 0xB90)) *
                (*(f32 *)(enemy->bytes + 0x2B10) - *(f32 *)(bullet->bytes + 0xB90)) +
            (*(f32 *)(enemy->bytes + 0x2B0C) - *(f32 *)(bullet->bytes + 0xB8C)) *
                (*(f32 *)(enemy->bytes + 0x2B0C) - *(f32 *)(bullet->bytes + 0xB8C));
        sqrtTemporary = TargetCISqrt();
        distance = sqrtTemporary;
        if (!(distance < radius))
            continue;

        request.position.x = *(f32 *)(bullet->bytes + 0xB8C);
        request.position.y = *(f32 *)(bullet->bytes + 0xB90);
        request.position.z = *(f32 *)(bullet->bytes + 0xB94);
        request.templateIndex = 0;
        request.baseSpriteIndex = 6;
        request.angleOffset = 0.0f;
        request.angleStep = -3.1415927f;
        request.speedStart = 0.7f;
        request.columns = 2;
        request.rows = 1;
        request.layoutMode = 6;
        request.flags = 2;
        request.QueueSpawnCommand(0, 0, 180,
                                  EclOperands::g_TargetRng49FE20.RandomF32() * 0.005f + 0.013f,
                                  1.5707964f);
        reinterpret_cast<EclOp121BulletManager *>(g_TargetBulletManager62F958)->Spawn(&request);
        bullet->Clear();
    }
}

namespace
{

// Observed in the cached 0x00410520 decompilation.  The instruction stream is
// not the TH06 structure: notably, time is a 32-bit word and the relative
// jump offset at +0x10 is a 32-bit value.  The remaining payload is typed at
// each opcode site, so it is represented as raw words here.
struct RunEclInstruction
{
    i32 time;       // +0x00
    i16 opcode;     // +0x04
    i16 nextOffset; // +0x06
    u8 unknown08;
    u8 difficultyMask; // +0x09
    u16 operandFlags;  // +0x0A
    i32 operand[8];    // +0x0C
};
typedef char RunEclInstructionOpcodeOffsetCheck[offsetof(RunEclInstruction, opcode) == 4 ? 1 : -1];
typedef char RunEclInstructionFlagOffsetCheck[offsetof(RunEclInstruction, operandFlags) == 10 ? 1 : -1];

// These are target-observed byte offsets used by the dispatcher itself.  They
// are kept private rather than promoting a speculative Enemy definition into
// EclManager.hpp.
enum EnemyRunEclOffset
{
    ENEMY_CURRENT_INSTRUCTION = 0x6E4,
    ENEMY_SCRIPT_TIME = 0x6F0,
    ENEMY_CALLBACK_CONTEXT = 0x6FC,
    ENEMY_SCRIPT_TIMER_PREVIOUS = 0x764,
    ENEMY_SCRIPT_TIMER_CURRENT = 0x768,
    ENEMY_SCRIPT_TIMER_LIMIT = 0x76C,
    ENEMY_CONTEXT_STACK_BASE = 0x8FC,
    ENEMY_CONTEXT_STACK_DEPTH = 0x2A7C,
    ENEMY_INTERRUPT_TABLE = 0x2A88,
    ENEMY_RUN_INTERRUPT = 0x2B08,
    ENEMY_TIMER_CALLBACK_THRESHOLD = 0x2F58,
    ENEMY_TIMER_CALLBACK = 0x2EE4,
    ENEMY_TIMER_CALLBACK_CONTEXT = 0x2EE8,
    ENEMY_TIMER_CALLBACK_TIMER = 0x2F5C,
    ENEMY_TIMER_CALLBACK_RAN = 0x8F4,
    ENEMY_MOVEMENT_FLAGS = 0x2E28,
    ENEMY_DISABLE_CALL_STACK = 0x2E2A,
};

static __forceinline RunEclInstruction *&CurrentInstruction(EclOperands::EnemyOverlay *enemy)
{
    return *(RunEclInstruction **)(enemy->bytes + ENEMY_CURRENT_INSTRUCTION);
}

static __forceinline i32 &ScriptTime(EclOperands::EnemyOverlay *enemy)
{
    return *(i32 *)(enemy->bytes + ENEMY_SCRIPT_TIME);
}

static __forceinline i32 InstructionDue(EclOperands::EnemyOverlay *enemy,
                                        const RunEclInstruction *instruction)
{
    return !(ScriptTime(enemy) - instruction->time);
}

static __forceinline i32 &IntAt(EclOperands::EnemyOverlay *enemy, i32 offset)
{
    return *(i32 *)(enemy->bytes + offset);
}

static __forceinline f32 &FloatAt(EclOperands::EnemyOverlay *enemy, i32 offset)
{
    return *(f32 *)(enemy->bytes + offset);
}

static __forceinline u8 &ByteAt(EclOperands::EnemyOverlay *enemy, i32 offset)
{
    return *(u8 *)(enemy->bytes + offset);
}

static __forceinline i32 ReadInt(EclOperands::EnemyOverlay *enemy, const RunEclInstruction *instruction,
                                 i32 operandIndex)
{
    if (instruction->operandFlags & (1 << operandIndex))
        return EclOperands::ResolveInt(enemy, instruction->operand[operandIndex]);
    return instruction->operand[operandIndex];
}

static __forceinline f32 ReadFloat(EclOperands::EnemyOverlay *enemy, const RunEclInstruction *instruction,
                                   i32 operandIndex)
{
    if (instruction->operandFlags & (1 << operandIndex))
        return enemy->ResolveFloat(*(const f32 *)&instruction->operand[operandIndex]);
    return *(const f32 *)&instruction->operand[operandIndex];
}

static __forceinline i32 ReadShortInt(EclOperands::EnemyOverlay *enemy, const RunEclInstruction *instruction,
                                      i32 byteOffset, i32 flagIndex)
{
    if (instruction->operandFlags & (1 << flagIndex))
        return EclOperands::ResolveInt(enemy, *(const i16 *)((const u8 *)instruction + byteOffset));
    return *(const i16 *)((const u8 *)instruction + byteOffset);
}

static __forceinline i32 RawI32(const RunEclInstruction *instruction, i32 byteOffset)
{
    return *(const i32 *)((const u8 *)instruction + byteOffset);
}

static __forceinline i32 *WriteInt(EclOperands::EnemyOverlay *enemy, RunEclInstruction *instruction)
{
    return EclOperands::ResolveIntLValue(enemy, &instruction->operand[0], instruction->operandFlags, 0);
}

static __forceinline f32 *WriteFloat(EclOperands::EnemyOverlay *enemy, RunEclInstruction *instruction)
{
    return EclOperands::ResolveFloatLValue(enemy, (f32 *)&instruction->operand[0], instruction->operandFlags, 0);
}

static __forceinline f32 *WriteFloatAt(EclOperands::EnemyOverlay *enemy, RunEclInstruction *instruction,
                                       i32 operandIndex)
{
    return EclOperands::ResolveFloatLValue(enemy, (f32 *)&instruction->operand[operandIndex],
                                           instruction->operandFlags, operandIndex);
}

static __forceinline void Advance(RunEclInstruction *&instruction)
{
    instruction = (RunEclInstruction *)((u8 *)instruction + instruction->nextOffset);
}

static __forceinline void Jump(EclOperands::EnemyOverlay *enemy, RunEclInstruction *&instruction)
{
    ScriptTime(enemy) = instruction->operand[0];
    instruction = (RunEclInstruction *)((u8 *)instruction + instruction->operand[1]);
}

static __forceinline void ConditionalJump(EclOperands::EnemyOverlay *enemy, RunEclInstruction *&instruction)
{
    ScriptTime(enemy) = instruction->operand[2];
    instruction = (RunEclInstruction *)((u8 *)instruction + instruction->operand[3]);
}

// The target copies exactly 0x218 bytes of context around each nested ECL
// call.  That observed span includes data not yet represented by the public
// EnemyEclContext declaration, so use a private byte copy here.
static __forceinline void SaveEclContext(EclOperands::EnemyOverlay *enemy)
{
    memcpy(enemy->bytes + ENEMY_CONTEXT_STACK_BASE +
               0x218 * IntAt(enemy, ENEMY_CONTEXT_STACK_DEPTH),
           enemy->bytes + ENEMY_CURRENT_INSTRUCTION, 0x218);
}

struct EclTimerCallbackTimer
{
    i32 previous;
    f32 subFrame;
    i32 current;
};

// Observed at 0x0041054E--0x00410699.  This is separate from the script
// interrupt selected through +0x2B08: the callback uses its own timer and
// restores the saved ECL context before entering the requested subroutine.
static __forceinline void EnterTimerCallback(EclOperands::EnemyOverlay *enemy,
                                             RunEclInstruction *&instruction)
{
    EclTimerCallbackTimer *timer;
    i32 current;
    i32 threshold;

    // Keep the positive path as the lexical fallthrough.  The target uses
    // forward `jl` exits at 0x00410558 and 0x004105C6 to the common resume
    // sequence; equivalent early returns reverse both branches under VC7.
    if (IntAt(enemy, ENEMY_TIMER_CALLBACK) >= 0)
    {
        timer = (EclTimerCallbackTimer *)(enemy->bytes + ENEMY_TIMER_CALLBACK_TIMER);
        timer->previous = timer->current;
        g_EclTailTimerManager.Advance(&timer->current, (i32 *)&timer->subFrame);
        current = timer->current;
        threshold = IntAt(enemy, ENEMY_TIMER_CALLBACK_THRESHOLD);
        if (current >= threshold)
        {
            // The target rematerializes this receiver at 0x004105CC before
            // the reset stores instead of reusing the timer used for Advance.
            EclTimerCallbackTimer *resetTimer =
                (EclTimerCallbackTimer *)(enemy->bytes + ENEMY_TIMER_CALLBACK_TIMER);
            resetTimer->current = 0;
            resetTimer->subFrame = 0.0f;
            resetTimer->previous = -999;
            SaveEclContext(enemy);
            memcpy(enemy->bytes + ENEMY_CALLBACK_CONTEXT, enemy->bytes + ENEMY_TIMER_CALLBACK_CONTEXT, 0x68);
            g_TargetEclManager1347938.CallEclSub((EnemyEclContext *)(enemy->bytes + ENEMY_CURRENT_INSTRUCTION),
                                                 (i16)IntAt(enemy, ENEMY_TIMER_CALLBACK));
            if (IntAt(enemy, ENEMY_CONTEXT_STACK_DEPTH) < 15)
                IntAt(enemy, ENEMY_CONTEXT_STACK_DEPTH) = IntAt(enemy, ENEMY_CONTEXT_STACK_DEPTH) + 1;
            instruction = CurrentInstruction(enemy);
            IntAt(enemy, ENEMY_TIMER_CALLBACK_RAN) = 1;
        }
    }
}

// Exact payload accesses for target opcodes 0x40--0x48.  The request starts
// at Enemy+0x2BD4 and is supplied to 0x00424D20; names are intentionally
// avoided because the complete shooter type belongs to the bullet lane.
static __forceinline void SpawnBulletVariant(EclOperands::EnemyOverlay *enemy, RunEclInstruction *instruction)
{
    u8 *request;
    i32 rankDelta;
    f32 speedDelta;

    if (IntAt(enemy, 4 * 2798) <= 0)
        return;

    request = enemy->bytes + 4 * 2805;
    *(i16 *)(request + 0) = (i16)ReadShortInt(enemy, instruction, 12, 0);
    *(i16 *)(request + 188) = (i16)ReadInt(enemy, instruction, 1);
    *(i16 *)(request + 190) = (i16)ReadInt(enemy, instruction, 2);
    *(i16 *)(request + 192) = (i16)(instruction->opcode - 64);
    *(f32 *)(request + 4) = FloatAt(enemy, 4 * 2755) + FloatAt(enemy, 4 * 2781);
    *(f32 *)(request + 8) = FloatAt(enemy, 4 * 2756) + FloatAt(enemy, 4 * 2782);
    *(f32 *)(request + 12) = FloatAt(enemy, 4 * 2757) + FloatAt(enemy, 4 * 2783);
    *(f32 *)(request + 16) = ReadFloat(enemy, instruction, 5);
    *(f32 *)(request + 20) = ReadFloat(enemy, instruction, 6);
    *(f32 *)(request + 24) = ReadFloat(enemy, instruction, 3);
    *(f32 *)(request + 28) = ReadFloat(enemy, instruction, 4);

    if (!g_TargetSpellActive12FE0C8)
    {
        rankDelta = *(i16 *)(enemy->bytes + 2 * 5592) +
                    g_TargetRank62F8A4 * (*(i16 *)(enemy->bytes + 2 * 5593) - *(i16 *)(enemy->bytes + 2 * 5592)) / 32;
        *(i16 *)(request + 188) = (i16)(*(i16 *)(request + 188) + rankDelta);
        if (*(i16 *)(request + 188) <= 0)
            *(i16 *)(request + 188) = 1;

        rankDelta = *(i16 *)(enemy->bytes + 2 * 5594) +
                    g_TargetRank62F8A4 * (*(i16 *)(enemy->bytes + 2 * 5595) - *(i16 *)(enemy->bytes + 2 * 5594)) / 32;
        *(i16 *)(request + 190) = (i16)(*(i16 *)(request + 190) + rankDelta);
        if (*(i16 *)(request + 190) <= 0)
            *(i16 *)(request + 190) = 1;

        speedDelta = (FloatAt(enemy, 4 * 2795) - FloatAt(enemy, 4 * 2794)) * g_TargetRank62F8A4 / 32.0f +
                     FloatAt(enemy, 4 * 2794);
        if (*(f32 *)(request + 24) != 0.0f)
        {
            *(f32 *)(request + 24) += speedDelta;
            if (*(f32 *)(request + 24) < 0.3f)
                *(f32 *)(request + 24) = 0.3f;
        }
        *(f32 *)(request + 28) += speedDelta / 2.0f;
        if (*(f32 *)(request + 28) < 0.3f)
            *(f32 *)(request + 28) = 0.3f;
    }

    *(i16 *)(request + 194) = 0;
    *(i32 *)(request + 196) = instruction->operand[7];
    *(i16 *)(request + 2) = (i16)ReadShortInt(enemy, instruction, 14, 1);
    if ((ByteAt(enemy, ENEMY_MOVEMENT_FLAGS) & 0x20) == 0)
        TargetSpawnBullet(g_TargetBulletManager62F958, request);
}

static __forceinline void *LaserSlot(EclOperands::EnemyOverlay *enemy, i32 index)
{
    return *(void **)(enemy->bytes + 4 * (2915 + index));
}

static __forceinline void SpawnLaserVariant(EclOperands::EnemyOverlay *enemy, RunEclInstruction *instruction)
{
    u8 *request;
    i32 slot;

    request = enemy->bytes + 4 * 2862;
    *(f32 *)(request + 4) = FloatAt(enemy, 4 * 2755) + FloatAt(enemy, 4 * 2781);
    *(f32 *)(request + 8) = FloatAt(enemy, 4 * 2756) + FloatAt(enemy, 4 * 2782);
    *(f32 *)(request + 12) = FloatAt(enemy, 4 * 2757) + FloatAt(enemy, 4 * 2783);
    *(i16 *)(request + 0) = *(i16 *)((u8 *)instruction + 12);
    *(i16 *)(request + 2) = (i16)ReadShortInt(enemy, instruction, 14, 1);
    *(f32 *)(request + 16) = ReadFloat(enemy, instruction, 1);
    *(f32 *)(request + 24) = ReadFloat(enemy, instruction, 2);
    *(f32 *)(request + 152) = ReadFloat(enemy, instruction, 3);
    *(f32 *)(request + 156) = ReadFloat(enemy, instruction, 4);
    *(f32 *)(request + 160) = ReadFloat(enemy, instruction, 5);
    *(i32 *)(request + 164) = RawI32(instruction, 36);
    *(i32 *)(request + 168) = RawI32(instruction, 40);
    *(i32 *)(request + 172) = RawI32(instruction, 44);
    *(i32 *)(request + 176) = RawI32(instruction, 48);
    *(i32 *)(request + 180) = RawI32(instruction, 52);
    *(i32 *)(request + 184) = RawI32(instruction, 56);
    *(i32 *)(request + 196) = RawI32(instruction, 60);
    *(i16 *)(request + 192) = instruction->opcode != 83;
    slot = IntAt(enemy, 4 * 2947);
    *(void **)(enemy->bytes + 4 * (2915 + slot)) = TargetSpawnLaser(g_TargetBulletManager62F958, request);
}

} // namespace

// Target-observed dispatcher entry: ECX is EclManager, Enemy is the single
// stack argument, and the target returns with `ret 4`.  This baseline covers
// the opcode cluster whose operand accesses and resolver calls are directly
// visible at 0x00410744--0x00411F1B.  Higher opcode families are intentionally
// not guessed until their owning Enemy/manager layouts are recovered.
ZunResult EclManager::RunEcl(Enemy *enemy)
{
#define rawEnemy ((EclOperands::EnemyOverlay *)enemy)
    RunEclInstruction *instruction;

// The target reloads the active instruction before both the initial dispatch
// and each post-interrupt resume (0x00410531 / 0x00414BFD).
run_ecl_top:
    instruction = CurrentInstruction(rawEnemy);
    // A pending script interrupt branches forward into the shared opcode-109
    // continuation below.  Keeping that continuation physically shared is
    // required by the target's entry branch at 0x00410549.
    if (IntAt(rawEnemy, ENEMY_RUN_INTERRUPT) >= 0)
        goto run_script_interrupt;

    // The timer callback path begins at 0x0041054E after the +0x2B08 test.
    EnterTimerCallback(rawEnemy, instruction);

    // Target 0x0041069A--0x004106D2 bypasses instruction dispatch while the
    // per-script delay is active, stepping both target-observed ZunTimer
    // instances backward before resuming the post-dispatch enemy update.
    const i32 scriptTimerLimit = IntAt(rawEnemy, ENEMY_SCRIPT_TIMER_LIMIT);
    if (scriptTimerLimit > 0)
    {
        ((ZunTimer *)(rawEnemy->bytes + ENEMY_SCRIPT_TIMER_PREVIOUS))->Decrement(1);
        ((ZunTimer *)(rawEnemy->bytes + 0x6E8))->Decrement(1);
        goto post_ecl_dispatch;
    }

    // Observed at 0x004106D7: subtract, neg, sbb, inc, and test materialize
    // a zero difference before the dispatcher.  Keep that arithmetic form:
    // it is both target-faithful and preserves the VC7 boolean topology.
    while (InstructionDue(rawEnemy, instruction))
    {
        void *laser;

        // 0x004106EC tests the instruction difficulty byte before dispatch.
        // A clear bit means that this instruction is skipped but still
        // consumes its encoded extent.
        // The target performs a dword mask load at 0x004106F9.  Retain the
        // established symbol declaration while expressing that observed
        // access width locally.
        if ((instruction->difficultyMask & *(const i32 *)&g_TargetDifficultyMask626284) == 0)
            goto advance_instruction;

        switch (instruction->opcode)
        {
        case 1: // target 0x00410744
            return ZUN_ERROR;

        // Target 0x00410749--0x004107AF: schedule the script-delay timer.
        // This case is physically emitted before the jump handlers in the
        // original dispatcher even though its opcode value is 0x2d.
        case 45:
        {
            i32 timerLimit;
            if (instruction->operandFlags & 1)
                timerLimit = EclOperands::ResolveInt(rawEnemy, instruction->operand[0]);
            else
                timerLimit = instruction->operand[0];
            ZunTimer *const scriptTimer =
                (ZunTimer *)(rawEnemy->bytes + ENEMY_SCRIPT_TIMER_PREVIOUS);
            scriptTimer->current = timerLimit;
            scriptTimer->subFrame = 0.0f;
            scriptTimer->previous = -999;
            break;
        }

        case 3: // decrement an integer lvalue, then jump while it remains positive
        {
            i32 *const value = EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[2],
                                                              instruction->operandFlags, 2);
            --*value;
            i32 remaining;
            if (instruction->operandFlags & (1 << 2))
                remaining = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
            else
                remaining = instruction->operand[2];
            // Target 0x0041081C keeps the positive path as fallthrough into
            // the shared Jump body, with the non-positive path taking the
            // long advance exit.  A forwarding label reverses this topology
            // under VC7 (`jle` plus a short jump), despite equivalent state.
            if (remaining <= 0)
                goto advance_instruction;
        }

        // The target switch table enters opcode 2 at the same Jump body that
        // opcode 3 falls through to when its decremented value remains
        // positive (0x00410823).  Preserve that physical sharing.
        case 2: // jump: time at +0x0c, byte displacement at +0x10
            Jump(rawEnemy, instruction);
            continue;

        case 4: // integer assignment
        {
            // Target 0x00410843 uses the literal operand-1 flag bit (0x2),
            // then retains the resolved value in a stack temporary for the
            // following operand-0 lvalue store.
            i32 value;
            if (instruction->operandFlags & 2)
                value = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                value = instruction->operand[1];
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) = value;
            break;
        }

        case 5: // float assignment
        {
            // As with opcode 4, target 0x00410898 uses the fixed operand-1
            // flag bit and retains the resolved scalar for the operand-0
            // lvalue write.
            f32 value;
            if (instruction->operandFlags & 2)
                value = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                value = *(const f32 *)&instruction->operand[1];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = value;
            break;
        }

        // The target emits this operand-0 angle-normalization handler between
        // opcodes 5 and 6 (0x004108EE--0x00410953), irrespective of its
        // numeric opcode value.
        case 40:
        {
            f32 angle;
            f32 normalizedAngle;
            if (instruction->operandFlags & 1)
                angle = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                angle = *(const f32 *)&instruction->operand[0];
            normalizedAngle = TargetAddNormalizeAngle(angle, 0.0f);
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = normalizedAngle;
            break;
        }

        case 6:
        {
            i32 limit;
            i32 value;
            if (instruction->operandFlags & 2)
                limit = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                limit = instruction->operand[1];
            if (limit)
                value = EclOperands::g_TargetRng49FE20.RandomU32() % limit;
            else
                value = 0;
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) = value;
            break;
        }
        case 7:
        {
            i32 limit;
            i32 base;
            i32 randomValue;
            if (instruction->operandFlags & 2)
                limit = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                limit = instruction->operand[1];
            if (instruction->operandFlags & 4)
                base = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
            else
                base = instruction->operand[2];
            if (limit)
                randomValue = EclOperands::g_TargetRng49FE20.RandomU32() % limit;
            else
                randomValue = 0;
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) =
                randomValue + base;
            break;
        }
        case 8:
        {
            f32 multiplier;
            f32 value;
            if (instruction->operandFlags & 2)
                multiplier = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                multiplier = *(const f32 *)&instruction->operand[1];
            value = EclOperands::g_TargetRng49FE20.RandomF32() * multiplier;
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = value;
            break;
        }
        case 9:
        {
            f32 multiplier;
            f32 addend;
            f32 value;
            if (instruction->operandFlags & 2)
                multiplier = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                multiplier = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 4)
                addend = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                addend = *(const f32 *)&instruction->operand[2];
            value = EclOperands::g_TargetRng49FE20.RandomF32() * multiplier + addend;
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = value;
            break;
        }
        case 10:
        {
            i32 multiplier;
            if (instruction->operandFlags & 2)
                multiplier = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                multiplier = instruction->operand[1];
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) =
                ((EclOperands::g_TargetRng49FE20.GetRandomU16() & 1) ? 1 : -1) * multiplier;
            break;
        }
        case 11:
        {
            f32 multiplier;
            f32 sign;
            if (EclOperands::g_TargetRng49FE20.GetRandomU16() & 1)
                sign = 1.0f;
            else
                sign = -1.0f;
            if (instruction->operandFlags & 2)
                multiplier = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                multiplier = *(const f32 *)&instruction->operand[1];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = sign * multiplier;
            break;
        }
        case 17:
            ++*EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0);
            break;
        case 18:
            --*EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0);
            break;
        case 43:
        {
            i32 bossIndex;
            i32 value;
            if (instruction->operandFlags & 2)
            {
                if (instruction->operandFlags & 4)
                    bossIndex = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
                else
                    bossIndex = instruction->operand[2];
                value = EclOperands::ResolveInt(
                    (EclOperands::EnemyOverlay *)SpellLifecycle::g_TargetSpellBosses12FE098[bossIndex],
                    instruction->operand[1]);
            }
            else
            {
                value = instruction->operand[1];
            }
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) = value;
            break;
        }
        case 44:
        {
            i32 bossIndex;
            f32 value;
            if (instruction->operandFlags & 2)
            {
                if (instruction->operandFlags & 4)
                    bossIndex = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
                else
                    bossIndex = instruction->operand[2];
                value = ((EclOperands::EnemyOverlay *)SpellLifecycle::g_TargetSpellBosses12FE098[bossIndex])
                            ->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            }
            else
            {
                value = *(const f32 *)&instruction->operand[1];
            }
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = value;
            break;
        }

        // 0x0c--0x10: target integer ALU block.  The order of operands is
        // taken from the target stores, rather than from the TH06 enum.
        case 12:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 2)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                left = instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
            else
                right = instruction->operand[2];
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) =
                left + right;
            break;
        }
        case 19:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 2)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                left = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                right = *(const f32 *)&instruction->operand[2];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = left + right;
            break;
        }
        case 13:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 2)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                left = instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
            else
                right = instruction->operand[2];
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) =
                left - right;
            break;
        }
        case 20:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 2)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                left = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                right = *(const f32 *)&instruction->operand[2];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = left - right;
            break;
        }
        case 14:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 2)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                left = instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
            else
                right = instruction->operand[2];
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) =
                left * right;
            break;
        }
        case 21:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 2)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                left = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                right = *(const f32 *)&instruction->operand[2];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = left * right;
            break;
        }
        case 15:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 2)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                left = instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
            else
                right = instruction->operand[2];
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) =
                left / right;
            break;
        }
        case 22:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 2)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                left = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                right = *(const f32 *)&instruction->operand[2];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = left / right;
            break;
        }
        case 16:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 2)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                left = instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
            else
                right = instruction->operand[2];
            *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[0], instruction->operandFlags, 0) =
                left % right;
            break;
        }
        case 23:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 2)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                left = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 4)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                right = *(const f32 *)&instruction->operand[2];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = (f32)fmod(left, right);
            break;
        }

        case 24:
        {
            f32 value;
            if (instruction->operandFlags & 2)
                value = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                value = *(const f32 *)&instruction->operand[1];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = (f32)sin(value);
            break;
        }
        case 25:
        {
            f32 value;
            if (instruction->operandFlags & 2)
                value = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                value = *(const f32 *)&instruction->operand[1];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) = (f32)cos(value);
            break;
        }

        case 26:
        {
            f32 operand3;
            f32 operand1;
            f32 operand4;
            f32 operand2;
            if (instruction->operandFlags & 8)
                operand3 = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[3]);
            else
                operand3 = *(const f32 *)&instruction->operand[3];
            if (instruction->operandFlags & 2)
                operand1 = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                operand1 = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 16)
                operand4 = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[4]);
            else
                operand4 = *(const f32 *)&instruction->operand[4];
            if (instruction->operandFlags & 4)
                operand2 = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                operand2 = *(const f32 *)&instruction->operand[2];
            *EclOperands::ResolveFloatLValue(rawEnemy, (f32 *)&instruction->operand[0],
                                              instruction->operandFlags, 0) =
                (f32)atan2(operand3 - operand1, operand4 - operand2);
            break;
        }

        // Conditional jumps 0x1c--0x27 update the instruction time from +0x14
        // and branch by the signed +0x18 displacement.  Their comparison
        // polarity is target-observed at 0x00411945--0x00411F1B.
        case 28:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 1)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[0]);
            else
                left = instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                right = instruction->operand[1];
            if (left == right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 29:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 1)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                left = *(const f32 *)&instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                right = *(const f32 *)&instruction->operand[1];
            if (left == right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 30:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 1)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[0]);
            else
                left = instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                right = instruction->operand[1];
            if (left != right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 31:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 1)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                left = *(const f32 *)&instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                right = *(const f32 *)&instruction->operand[1];
            if (left != right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 32:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 1)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[0]);
            else
                left = instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                right = instruction->operand[1];
            if (left < right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 33:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 1)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                left = *(const f32 *)&instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                right = *(const f32 *)&instruction->operand[1];
            if (left < right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 34:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 1)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[0]);
            else
                left = instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                right = instruction->operand[1];
            if (left > right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 35:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 1)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                left = *(const f32 *)&instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                right = *(const f32 *)&instruction->operand[1];
            if (left > right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 36:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 1)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[0]);
            else
                left = instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                right = instruction->operand[1];
            if (left <= right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 37:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 1)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                left = *(const f32 *)&instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                right = *(const f32 *)&instruction->operand[1];
            if (left <= right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 38:
        {
            i32 left;
            i32 right;
            if (instruction->operandFlags & 1)
                left = EclOperands::ResolveInt(rawEnemy, instruction->operand[0]);
            else
                left = instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                right = instruction->operand[1];
            if (left >= right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }
        case 39:
        {
            f32 left;
            f32 right;
            if (instruction->operandFlags & 1)
                left = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                left = *(const f32 *)&instruction->operand[0];
            if (instruction->operandFlags & 2)
                right = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                right = *(const f32 *)&instruction->operand[1];
            if (left >= right) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        }

        // 0x2d--0x3f are target-observed direct Enemy state transitions.  No
        // complete Enemy type is implied: each store below is an attested
        // offset in this dispatcher.
        case 46:
        {
            f32 x;
            f32 y;
            f32 z;
            if (instruction->operandFlags & 1)
                x = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                x = *(const f32 *)&instruction->operand[0];
            if (instruction->operandFlags & 2)
                y = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                y = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 4)
                z = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                z = *(const f32 *)&instruction->operand[2];
            FloatAt(rawEnemy, 0x2B0C) = x;
            FloatAt(rawEnemy, 0x2B10) = y;
            FloatAt(rawEnemy, 0x2B14) = z;
            TargetClampEnemy(rawEnemy);
            break;
        }

        case 47:
        {
            f32 x;
            f32 y;
            f32 z;
            if (instruction->operandFlags & 1)
                x = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[0]);
            else
                x = *(const f32 *)&instruction->operand[0];
            if (instruction->operandFlags & 2)
                y = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[1]);
            else
                y = *(const f32 *)&instruction->operand[1];
            if (instruction->operandFlags & 4)
                z = rawEnemy->ResolveFloat(*(const f32 *)&instruction->operand[2]);
            else
                z = *(const f32 *)&instruction->operand[2];
            FloatAt(rawEnemy, 0x2B18) = x;
            FloatAt(rawEnemy, 0x2B1C) = y;
            FloatAt(rawEnemy, 0x2B20) = z;
            FloatAt(rawEnemy, 0x2B54) = FloatAt(rawEnemy, 0x2B1C);
            rawEnemy->bytes[ENEMY_MOVEMENT_FLAGS] &= 0xFC;
            break;
        }

        case 48:
            FloatAt(rawEnemy, 0x2B58) = ReadFloat(rawEnemy, instruction, 0);
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) = (ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) & 0xFC) | 1;
            break;

        case 49:
            FloatAt(rawEnemy, 0x2B64) = ReadFloat(rawEnemy, instruction, 0);
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) = (ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) & 0xFC) | 1;
            break;

        case 50:
            FloatAt(rawEnemy, 0x2B68) = ReadFloat(rawEnemy, instruction, 0);
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) = (ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) & 0xFC) | 1;
            break;

        case 56:
            IntAt(rawEnemy, 4 * 2793) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 2792) = IntAt(rawEnemy, 4 * 2793);
            IntAt(rawEnemy, 4 * 2791) = 0;
            IntAt(rawEnemy, 4 * 2790) = -999;
            FloatAt(rawEnemy, 4 * 2787) = ReadFloat(rawEnemy, instruction, 1);
            FloatAt(rawEnemy, 4 * 2788) = ReadFloat(rawEnemy, instruction, 2);
            FloatAt(rawEnemy, 4 * 2789) = ReadFloat(rawEnemy, instruction, 3);
            FloatAt(rawEnemy, 4 * 2775) = ReadFloat(rawEnemy, instruction, 4);
            FloatAt(rawEnemy, 4 * 2776) = ReadFloat(rawEnemy, instruction, 5);
            FloatAt(rawEnemy, 4 * 2779) = ReadFloat(rawEnemy, instruction, 6);
            FloatAt(rawEnemy, 4 * 2780) = ReadFloat(rawEnemy, instruction, 7);
            rawEnemy->bytes[ENEMY_MOVEMENT_FLAGS] |= 3;
            break;

        case 57:
            FloatAt(rawEnemy, 4 * 2779) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 4 * 2780) = ReadFloat(rawEnemy, instruction, 1);
            break;

        case 58:
            FloatAt(rawEnemy, 4 * 2775) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 4 * 2776) = ReadFloat(rawEnemy, instruction, 1);
            break;

        case 59:
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) = (ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) & 0xFC) | 1;
            IntAt(rawEnemy, 4 * 2793) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 2792) = IntAt(rawEnemy, 4 * 2793);
            IntAt(rawEnemy, 4 * 2791) = 0;
            IntAt(rawEnemy, 4 * 2790) = -999;
            break;

        case 60:
            rawEnemy->bytes[ENEMY_MOVEMENT_FLAGS] |= 3;
            IntAt(rawEnemy, 4 * 2793) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 2792) = IntAt(rawEnemy, 4 * 2793);
            IntAt(rawEnemy, 4 * 2791) = 0;
            IntAt(rawEnemy, 4 * 2790) = -999;
            break;

        case 61:
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) = (ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) & 0xFC) | 2;
            IntAt(rawEnemy, 4 * 2793) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 2792) = IntAt(rawEnemy, 4 * 2793);
            IntAt(rawEnemy, 4 * 2791) = 0;
            IntAt(rawEnemy, 4 * 2790) = -999;
            break;

        case 62:
            FloatAt(rawEnemy, 4 * 2959) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 4 * 2960) = ReadFloat(rawEnemy, instruction, 1);
            FloatAt(rawEnemy, 4 * 2961) = ReadFloat(rawEnemy, instruction, 2);
            FloatAt(rawEnemy, 4 * 2962) = ReadFloat(rawEnemy, instruction, 3);
            rawEnemy->bytes[11817] |= 0x80;
            break;

        case 63:
            rawEnemy->bytes[11817] &= ~0x80;
            break;

        case 64:
        case 65:
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
        case 71:
        case 72:
            SpawnBulletVariant(rawEnemy, instruction);
            break;

        // 0x49--0x51 update the target's autonomous shooting state.  Their
        // timer and flag offsets are direct target stores in the cached body.
        case 73:
            IntAt(rawEnemy, 4 * 2858) = ReadInt(rawEnemy, instruction, 0);
            if (IntAt(rawEnemy, 4 * 2858))
            {
                IntAt(rawEnemy, 4 * 2858) = IntAt(rawEnemy, 4 * 2858) +
                                               IntAt(rawEnemy, 4 * 2858) / 5 +
                                               g_TargetRank62F8A4 * (-IntAt(rawEnemy, 4 * 2858) / 5 -
                                                                      IntAt(rawEnemy, 4 * 2858) / 5) / 32;
                IntAt(rawEnemy, 4 * 2861) = 0;
                IntAt(rawEnemy, 4 * 2860) = 0;
                IntAt(rawEnemy, 4 * 2859) = -999;
            }
            break;

        case 74:
            IntAt(rawEnemy, 4 * 2858) = ReadInt(rawEnemy, instruction, 0);
            if (IntAt(rawEnemy, 4 * 2858))
            {
                IntAt(rawEnemy, 4 * 2858) = IntAt(rawEnemy, 4 * 2858) +
                                               IntAt(rawEnemy, 4 * 2858) / 5 +
                                               g_TargetRank62F8A4 * (-IntAt(rawEnemy, 4 * 2858) / 5 -
                                                                      IntAt(rawEnemy, 4 * 2858) / 5) / 32;
                IntAt(rawEnemy, 4 * 2861) = EclOperands::g_TargetRng49FE20.RandomU32() %
                                             IntAt(rawEnemy, 4 * 2858);
                IntAt(rawEnemy, 4 * 2860) = 0;
                IntAt(rawEnemy, 4 * 2859) = -999;
            }
            break;

        case 75:
            rawEnemy->bytes[ENEMY_MOVEMENT_FLAGS] |= 0x20;
            break;
        case 76:
            rawEnemy->bytes[ENEMY_MOVEMENT_FLAGS] &= ~0x20;
            break;
        case 77:
            FloatAt(rawEnemy, 4 * 2806) = FloatAt(rawEnemy, 4 * 2755) + FloatAt(rawEnemy, 4 * 2781);
            FloatAt(rawEnemy, 4 * 2807) = FloatAt(rawEnemy, 4 * 2756) + FloatAt(rawEnemy, 4 * 2782);
            FloatAt(rawEnemy, 4 * 2808) = FloatAt(rawEnemy, 4 * 2757) + FloatAt(rawEnemy, 4 * 2783);
            TargetSpawnBullet(g_TargetBulletManager62F958, rawEnemy->bytes + 2 * 5610);
            break;
        case 78:
            FloatAt(rawEnemy, 4 * 2781) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 4 * 2782) = ReadFloat(rawEnemy, instruction, 1);
            FloatAt(rawEnemy, 4 * 2783) = ReadFloat(rawEnemy, instruction, 2);
            break;
        case 79:
        {
            u8 *effect = rawEnemy->bytes + 24 * 2813 + 24 * ReadInt(rawEnemy, instruction, 0);
            *(i32 *)(effect + 16) = ReadInt(rawEnemy, instruction, 1);
            *(i32 *)(effect + 20) = ReadInt(rawEnemy, instruction, 2);
            *(i32 *)(effect + 8) = ReadInt(rawEnemy, instruction, 3);
            *(i32 *)(effect + 12) = ReadInt(rawEnemy, instruction, 4);
            *(f32 *)(effect + 0) = ReadFloat(rawEnemy, instruction, 5);
            *(f32 *)(effect + 4) = ReadFloat(rawEnemy, instruction, 6);
            break;
        }

        case 80:
            TargetClearBullets(g_TargetBulletManager62F958, 1);
            break;

        // The pair 0x52/0x53 share target request construction and differ
        // only in the request type word written at +0xc0.
        case 82:
        case 83:
            SpawnLaserVariant(rawEnemy, instruction);
            break;
        case 84:
            IntAt(rawEnemy, 4 * 2947) = ReadInt(rawEnemy, instruction, 0);
            break;
        case 85:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (laser)
                *(f32 *)((u8 *)laser + 1188) = TargetAddNormalizeAngle(*(f32 *)((u8 *)laser + 1188),
                                                                          ReadFloat(rawEnemy, instruction, 1));
            break;
        }
        case 86:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (laser)
                *(f32 *)((u8 *)laser + 1188) =
                    EclOperands::g_TargetPlayer4BDAD8.AngleToPlayer((const EclOperands::Vector3 *)((u8 *)laser + 1176)) +
                    ReadFloat(rawEnemy, instruction, 1);
            break;
        }
        case 87:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (laser)
            {
                *(f32 *)((u8 *)laser + 1176) = ReadFloat(rawEnemy, instruction, 1) + FloatAt(rawEnemy, 4 * 2755);
                *(f32 *)((u8 *)laser + 1180) = ReadFloat(rawEnemy, instruction, 2) + FloatAt(rawEnemy, 4 * 2756);
                *(f32 *)((u8 *)laser + 1184) = ReadFloat(rawEnemy, instruction, 3) + FloatAt(rawEnemy, 4 * 2757);
            }
            break;
        }
        case 88:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (!laser || !*(i32 *)((u8 *)laser + 1236))
                IntAt(rawEnemy, 4 * 572) = 1;
            else
                IntAt(rawEnemy, 4 * 572) = 0;
            break;
        }
        case 89:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (laser && *(i32 *)((u8 *)laser + 1236) && *(u8 *)((u8 *)laser + 1256) < 2)
            {
                *(u8 *)((u8 *)laser + 1256) = 2;
                *(i32 *)((u8 *)laser + 1248) = 0;
                *(i32 *)((u8 *)laser + 1244) = 0;
                *(i32 *)((u8 *)laser + 1240) = -999;
                *(i32 *)((u8 *)laser + 1204) = *(i32 *)((u8 *)laser + 1208);
            }
            break;
        }

        // These two lifecycle opcodes are fully owned by the independently
        // reconstructed SpellLifecycle helpers.  The target calls them at
        // 0x00414DDC and 0x00414DEC with the raw Enemy/instruction pair.
        case 90:
            SpellLifecycle::StartSpellcard((SpellLifecycle::EnemyOverlay *)rawEnemy,
                                           (const SpellLifecycle::SpellStartInstruction *)instruction);
            break;
        case 91:
            SpellLifecycle::FinishSpellcard((SpellLifecycle::EnemyOverlay *)rawEnemy, instruction);
            break;

        // 0x5e--0x70 are target-observed state and presentation controls.
        // Cases which create ANM instances or touch the shared stage slots
        // remain below until their owning layouts are attested.
        case 94:
            TargetEffect423090(8000, 0);
            break;
        case 95:
        {
            const i32 scriptId = ReadInt(rawEnemy, instruction, 0) + 2304;
            *(i16 *)(rawEnemy->bytes + 472) = (i16)scriptId;
            ((SpellLifecycle::AnmManagerOverlay *)SpellLifecycle::g_TargetAnmManager4B9E44)
                ->SetAndExecuteScript(rawEnemy,
                                     *(void **)(SpellLifecycle::g_TargetAnmManager4B9E44 +
                                                0x28EF0 + 4 * scriptId));
            break;
        }
        case 96:
            *(i16 *)(rawEnemy->bytes + 2 * 5912) = *(const i16 *)((const u8 *)instruction + 12);
            *(i16 *)(rawEnemy->bytes + 2 * 5913) = *(const i16 *)((const u8 *)instruction + 14);
            *(i16 *)(rawEnemy->bytes + 2 * 5914) = *(const i16 *)((const u8 *)instruction + 16);
            *(i16 *)(rawEnemy->bytes + 2 * 5915) = *(const i16 *)((const u8 *)instruction + 18);
            *(i16 *)(rawEnemy->bytes + 2 * 5916) = *(const i16 *)((const u8 *)instruction + 20);
            ByteAt(rawEnemy, 11822) = 0xFF;
            break;
        case 97:
        {
            const i32 slot = ReadInt(rawEnemy, instruction, 0);
            const i32 scriptId = ReadInt(rawEnemy, instruction, 1);
            if (slot >= 2)
                DebugPrint("error : sub anim overflow\r\n");

            if (scriptId < 0)
            {
                *(i16 *)(rawEnemy->bytes + 4 * (147 * slot + 265)) = -1;
            }
            else
            {
                const i32 targetScript = scriptId + 2304;
                *(i16 *)(rawEnemy->bytes + 4 * (147 * slot + 265)) = (i16)targetScript;
                ((SpellLifecycle::AnmManagerOverlay *)SpellLifecycle::g_TargetAnmManager4B9E44)
                    ->SetAndExecuteScript(rawEnemy->bytes + 4 * (147 * slot + 147),
                                         *(void **)(SpellLifecycle::g_TargetAnmManager4B9E44 +
                                                    0x28EF0 + 4 * targetScript));
            }
            break;
        }
        case 98:
            ByteAt(rawEnemy, 11796) = *((const u8 *)instruction + 12);
            ByteAt(rawEnemy, 11797) = *((const u8 *)instruction + 13);
            ByteAt(rawEnemy, 11798) = *((const u8 *)instruction + 14);
            break;
        case 99:
        {
            const i32 bossId = ReadInt(rawEnemy, instruction, 0);
            if (bossId < 0)
            {
                const u8 currentBossId = ByteAt(rawEnemy, 11799);
                if (currentBossId < 4)
                    g_TargetBossPresent49FC14 = 0;
                SpellLifecycle::g_TargetSpellBosses12FE098[currentBossId] = 0;
                *(u8 *)(rawEnemy->bytes + 11817) &= ~0x40;
                g_TargetBossUi134DB5A[294 * currentBossId] = 2;
                ((SpellLifecycle::EnemyOverlay *)rawEnemy)->UnregisterBoss();
            }
            else
            {
                SpellLifecycle::g_TargetSpellBosses12FE098[bossId] =
                    (SpellLifecycle::EnemyOverlay *)rawEnemy;
                g_TargetBossPresent49FC14 = 1;
                g_TargetBossHealth49FC18 = 1.0f;
                *(u8 *)(rawEnemy->bytes + 11817) |= 0x40;
                const u8 assignedBossId = (u8)ReadInt(rawEnemy, instruction, 0);
                ByteAt(rawEnemy, 11799) = assignedBossId;
                g_TargetBossUi134DB5A[294 * assignedBossId] = 1;
            }
            break;
        }
        case 100:
        {
            void *effect = g_EffectManager.SpawnParticles(
                13, (D3DXVECTOR3 *)(rawEnemy->bytes + 4 * 2755), 1,
                (i32)g_EffectsColor[RawI32(instruction, 12)]);
            *(i32 *)((u8 *)effect + 660) = RawI32(instruction, 16);
            *(i32 *)((u8 *)effect + 664) = RawI32(instruction, 20);
            *(i32 *)((u8 *)effect + 668) = RawI32(instruction, 24);
            IntAt(rawEnemy, 4 * 2990) = RawI32(instruction, 28);
            IntAt(rawEnemy, 4 * 2989) = IntAt(rawEnemy, 4 * 2989) + 1;
            break;
        }
        case 101:
            FloatAt(rawEnemy, 4 * 2767) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 4 * 2768) = ReadFloat(rawEnemy, instruction, 1);
            FloatAt(rawEnemy, 4 * 2769) = ReadFloat(rawEnemy, instruction, 2);
            break;
        case 102:
            ByteAt(rawEnemy, 11817) = (ByteAt(rawEnemy, 11817) & 0xFD) |
                                      (2 * (*((const u8 *)instruction + 12) & 1));
            break;
        case 103:
            ByteAt(rawEnemy, 11817) = (ByteAt(rawEnemy, 11817) & 0xFB) |
                                      (4 * (*((const u8 *)instruction + 12) & 1));
            break;
        case 104:
            ByteAt(rawEnemy, 11817) = (ByteAt(rawEnemy, 11817) & 0xEF) |
                                      (16 * (*((const u8 *)instruction + 12) & 1));
            break;
        case 105:
            Target44C930(ReadInt(rawEnemy, instruction, 0), 0);
            break;
        case 106:
            ByteAt(rawEnemy, 11818) = (ByteAt(rawEnemy, 11818) & 0xF8) |
                                      (*((const u8 *)instruction + 12) & 7);
            break;
        case 107:
            IntAt(rawEnemy, 4 * 2721) = *((const u8 *)instruction + 12);
            break;
        case 108:
            IntAt(rawEnemy, 4 * 2722 + ReadInt(rawEnemy, instruction, 1)) =
                ReadInt(rawEnemy, instruction, 0);
            break;
        case 109:
            IntAt(rawEnemy, 4 * 2754) = ReadInt(rawEnemy, instruction, 0);
            // Target case 109 falls directly into the pending-interrupt
            // continuation, rather than returning through the common advance.
            // The entry test above reaches this same label for a previously
            // pending interrupt.
run_script_interrupt:
            Advance(instruction);
            CurrentInstruction(rawEnemy) = instruction;
            if ((ByteAt(rawEnemy, ENEMY_DISABLE_CALL_STACK) & 0x20) == 0)
                SaveEclContext(rawEnemy);

            g_TargetEclManager1347938.CallEclSub(
                (EnemyEclContext *)(rawEnemy->bytes + ENEMY_CURRENT_INSTRUCTION),
                (i16)IntAt(rawEnemy, ENEMY_INTERRUPT_TABLE +
                                      4 * IntAt(rawEnemy, ENEMY_RUN_INTERRUPT)));
            if (IntAt(rawEnemy, ENEMY_CONTEXT_STACK_DEPTH) < 15)
                IntAt(rawEnemy, ENEMY_CONTEXT_STACK_DEPTH) =
                    IntAt(rawEnemy, ENEMY_CONTEXT_STACK_DEPTH) + 1;
            IntAt(rawEnemy, ENEMY_RUN_INTERRUPT) = -1;
            goto run_ecl_top;
        case 110:
        {
            const i32 value = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 2799) = value;
            IntAt(rawEnemy, 4 * 2798) = value;
            if (ByteAt(rawEnemy, 11799) == 0 && (ByteAt(rawEnemy, 11817) & 0x40))
            {
                for (i32 slot = 0; slot < 8; ++slot)
                {
                    g_TargetEclSlotFloatY49FC44[slot] = 0;
                    g_TargetEclSlotFloatX49FC24[slot] = 0;
                }
            }
            break;
        }
        case 111:
            IntAt(rawEnemy, 4 * 2803) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 2802) = 0;
            IntAt(rawEnemy, 4 * 2801) = -999;
            break;
        case 112:
            IntAt(rawEnemy, 4 * 2991) = ReadInt(rawEnemy, instruction, 0);
            break;
        case 113:
            IntAt(rawEnemy, 4 * 2995) = ReadInt(rawEnemy, instruction, 0);
            break;
        case 114:
            IntAt(rawEnemy, 4 * 2999) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 2803) = 0;
            IntAt(rawEnemy, 4 * 2802) = 0;
            IntAt(rawEnemy, 4 * 2801) = -999;
            break;
        case 115:
            IntAt(rawEnemy, 4 * 3000) = ReadInt(rawEnemy, instruction, 0);
            break;
        case 116:
            ByteAt(rawEnemy, 11817) = (ByteAt(rawEnemy, 11817) & 0xFE) |
                                      (*((const u8 *)instruction + 12) & 1);
            break;
        case 117:
            g_EffectManager.SpawnParticles(
                ReadInt(rawEnemy, instruction, 0),
                (D3DXVECTOR3 *)(rawEnemy->bytes + 4 * 2755),
                ReadInt(rawEnemy, instruction, 1),
                *EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[2],
                                                instruction->operandFlags, 2));
            break;
        case 120:
            ByteAt(rawEnemy, 11818) = (ByteAt(rawEnemy, 11818) & 0xEF) |
                                      (16 * (*((const u8 *)instruction + 12) & 1));
            break;
        case 119:
        {
            const i32 itemCount = ReadInt(rawEnemy, instruction, 0);
            for (i32 item = 0; item < itemCount; ++item)
            {
                BulletUpdateVec3 position;
                position.x = EclOperands::g_TargetRng49FE20.RandomF32() * 128.0f - 64.0f +
                             FloatAt(rawEnemy, 4 * 2755);
                position.y = EclOperands::g_TargetRng49FE20.RandomF32() * 128.0f - 64.0f +
                             FloatAt(rawEnemy, 4 * 2756);
                position.z = FloatAt(rawEnemy, 4 * 2757);
                if ((i32)(unsigned __int64)*(f32 *)(g_TargetScoreState626278 + 124) >= 128)
                    g_ItemManager.Spawn(&position, 1, 0);
                else if (item)
                    g_ItemManager.Spawn(&position, 0, 0);
                else
                    g_ItemManager.Spawn(&position, 2, 0);
            }
            break;
        }
        case 128:
            *(i16 *)(rawEnemy->bytes + 2 * 227) =
                (i16)ReadInt(rawEnemy, instruction, 0);
            break;
        case 129:
            *(i16 *)(rawEnemy->bytes + 4 * (147 * RawI32(instruction, 12) + 260) + 2) =
                *(const i16 *)((const u8 *)instruction + 16);
            break;
        case 130:
            ByteAt(rawEnemy, 11818) = (ByteAt(rawEnemy, 11818) & 0xDF) |
                                      (32 * (*((const u8 *)instruction + 12) & 1));
            break;
        case 131:
            FloatAt(rawEnemy, 4 * 2794) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 4 * 2795) = ReadFloat(rawEnemy, instruction, 1);
            *(i16 *)(rawEnemy->bytes + 2 * 5592) = ReadShortInt(rawEnemy, instruction, 20, 2);
            *(i16 *)(rawEnemy->bytes + 2 * 5593) = ReadShortInt(rawEnemy, instruction, 24, 3);
            *(i16 *)(rawEnemy->bytes + 2 * 5594) = ReadShortInt(rawEnemy, instruction, 28, 4);
            *(i16 *)(rawEnemy->bytes + 2 * 5595) = ReadShortInt(rawEnemy, instruction, 32, 5);
            break;
        case 132:
            ByteAt(rawEnemy, 11817) = (ByteAt(rawEnemy, 11817) & 0xF7) |
                                      (8 * (*((const u8 *)instruction + 12) & 1));
            break;
        case 133:
            IntAt(rawEnemy, 4 * 3000) = IntAt(rawEnemy, 4 * 2721);
            IntAt(rawEnemy, 4 * 2803) = 0;
            IntAt(rawEnemy, 4 * 2802) = 0;
            IntAt(rawEnemy, 4 * 2801) = -999;
            break;
        case 134:
            for (i32 slot = 0; slot < 32; ++slot)
                IntAt(rawEnemy, 4 * (2915 + slot)) = 0;
            break;
        case 135:
            ByteAt(rawEnemy, 11818) = (ByteAt(rawEnemy, 11818) & 0xBF) |
                                      ((*((const u8 *)instruction + 12) & 1) << 6);
            break;
        case 136:
            ByteAt(rawEnemy, 11817) = (ByteAt(rawEnemy, 11817) & 0xDF) |
                                      (32 * (*((const u8 *)instruction + 12) & 1));
            ByteAt(rawEnemy, 11823) = 2;
            break;
        case 137:
            ByteAt(rawEnemy, 11818) = (ByteAt(rawEnemy, 11818) & 0x7F) |
                                      ((*((const u8 *)instruction + 12) & 1) << 7);
            break;
        case 138:
        {
            ByteAt(rawEnemy, 20272) = *((const u8 *)instruction + 12);
            *(i16 *)(rawEnemy->bytes + 20274) = (i16)ReadInt(rawEnemy, instruction, 1);
            *(i16 *)(rawEnemy->bytes + 20276) = (i16)ReadInt(rawEnemy, instruction, 2);
            *(i16 *)(rawEnemy->bytes + 20278) = (i16)ReadInt(rawEnemy, instruction, 3);
            if (IntAt(rawEnemy, 4 * 5068) & 8)
            {
                ((SpellLifecycle::AnmManagerOverlay *)SpellLifecycle::g_TargetAnmManager4B9E44)->ConfigureBoss(
                    (SpellLifecycle::EnemyOverlay *)rawEnemy, rawEnemy->bytes + 4 * 3710,
                    2 * (*(i16 *)(rawEnemy->bytes + 20274) / *(i16 *)(rawEnemy->bytes + 20278)));
            }
            break;
        }
        case 139:
        {
            const i32 slot = ReadInt(rawEnemy, instruction, 0);
            i32 yValue;
            if (instruction->operandFlags & 2)
                yValue = EclOperands::ResolveInt(rawEnemy, instruction->operand[1]);
            else
                yValue = instruction->operand[1];
            i32 xValue;
            if (instruction->operandFlags & 4)
                xValue = EclOperands::ResolveInt(rawEnemy, instruction->operand[2]);
            else
                xValue = instruction->operand[2];
            g_TargetEclSlotFloatY49FC44[slot] = (f32)yValue / IntAt(rawEnemy, 4 * 2799);
            g_TargetEclSlotFloatX49FC24[slot] = (f32)xValue / IntAt(rawEnemy, 4 * 2799);
            g_TargetEclSlotValue49FC64[slot] = ReadInt(rawEnemy, instruction, 3);
            break;
        }
        case 123:
            Target439401(ReadInt(rawEnemy, instruction, 0));
            break;
        case 124:
            g_ItemManager.Spawn((BulletUpdateVec3 *)(rawEnemy->bytes + 4 * 2755),
                                ReadInt(rawEnemy, instruction, 0), 0);
            break;
        case 125:
            g_TargetEclControl134CBF4 = ReadInt(rawEnemy, instruction, 0);
            break;
        case 126:
            g_TargetEclControl49FC08 = ReadInt(rawEnemy, instruction, 0);
            g_TargetEclTimer62F898 += 1800;
            break;
        case 140:
            g_TargetSpellFloat12FE25C = ReadFloat(rawEnemy, instruction, 0);
            g_TargetSpellFloat12FE260 = ReadFloat(rawEnemy, instruction, 1);
            g_TargetSpellFloat12FE264 = ReadFloat(rawEnemy, instruction, 2);
            g_TargetSpellFloat12FE268 = ReadFloat(rawEnemy, instruction, 3);
            break;
        case 142:
            IntAt(rawEnemy, 4 * 5072) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 5071) = 0;
            IntAt(rawEnemy, 4 * 5070) = -999;
            break;
        case 143:
            Target424C00((f32 *)(rawEnemy->bytes + 4 * 2755), ReadFloat(rawEnemy, instruction, 0));
            break;
        case 144:
            IntAt(rawEnemy, 4 * 3030) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, 4 * 3029) = 0;
            IntAt(rawEnemy, 4 * 3028) = -999;
            IntAt(rawEnemy, 4 * 3001) = ReadInt(rawEnemy, instruction, 1);
            IntAt(rawEnemy, 4 * 3033) = 0;
            IntAt(rawEnemy, 4 * 3032) = 0;
            IntAt(rawEnemy, 4 * 3031) = -999;
            memcpy(rawEnemy->bytes + 4 * 3002, rawEnemy->bytes + 4 * 447, 0x68);
            break;
        case 145:
            if (SpellLifecycle::g_TargetSpellBosses12FE098[ReadInt(rawEnemy, instruction, 0)])
            {
                *(i32 *)(SpellLifecycle::g_TargetSpellBosses12FE098[ReadInt(rawEnemy, instruction, 0)]
                             ->bytes +
                         11016) = ReadInt(rawEnemy, instruction, 1);
            }
            break;
        case 146:
            TargetClearBullets(g_TargetBulletManager62F958, 0);
            break;
        case 147:
            g_TargetSpellControl12FE0F0 = ReadInt(rawEnemy, instruction, 0);
            break;
        case 148:
            IntAt(rawEnemy, 4 * (2991 + ReadInt(rawEnemy, instruction, 0))) =
                ReadInt(rawEnemy, instruction, 1);
            IntAt(rawEnemy, 4 * (2995 + ReadInt(rawEnemy, instruction, 0))) =
                ReadInt(rawEnemy, instruction, 2);
            break;
        case 149:
        {
            const i32 enabled = ReadInt(rawEnemy, instruction, 0) & 1;
            ByteAt(rawEnemy, 11819) = (ByteAt(rawEnemy, 11819) & 0xFD) | (2 * enabled);
            if (!(ByteAt(rawEnemy, 11819) & 2))
            {
                u8 *target = (u8 *)IntAt(rawEnemy, 4 * 2988);
                *(f32 *)(target + 588) = ReadFloat(rawEnemy, instruction, 1);
                *(f32 *)(target + 592) = ReadFloat(rawEnemy, instruction, 2);
                *(f32 *)(target + 596) = ReadFloat(rawEnemy, instruction, 3);
            }
            break;
        }
        case 150:
            FloatAt(rawEnemy, 8) = ReadFloat(rawEnemy, instruction, 0);
            break;
        case 151:
        {
            const f32 angle = ReadFloat(rawEnemy, instruction, 2);
            const f32 magnitude = ReadFloat(rawEnemy, instruction, 3);
            *WriteFloatAt(rawEnemy, instruction, 1) = (f32)(sin(angle) * magnitude);
            *WriteFloat(rawEnemy, instruction) = (f32)(cos(angle) * magnitude);
            break;
        }
        case 152:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (laser)
                *(f32 *)((u8 *)laser + 1188) = ReadFloat(rawEnemy, instruction, 1);
            break;
        }
        case 153:
            FloatAt(rawEnemy, 4 * 2770) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 4 * 2771) = ReadFloat(rawEnemy, instruction, 1);
            FloatAt(rawEnemy, 4 * 2772) = ReadFloat(rawEnemy, instruction, 2);
            break;
        case 154:
        {
            const i32 itemCount = ReadInt(rawEnemy, instruction, 0);
            for (i32 item = 0; item < itemCount; ++item)
            {
                BulletUpdateVec3 position;
                position.x = EclOperands::g_TargetRng49FE20.RandomF32() * 128.0f - 64.0f +
                             FloatAt(rawEnemy, 4 * 2755);
                position.y = EclOperands::g_TargetRng49FE20.RandomF32() * 128.0f - 64.0f +
                             FloatAt(rawEnemy, 4 * 2756);
                position.z = FloatAt(rawEnemy, 4 * 2757);
                g_ItemManager.Spawn(&position, 1, 0);
            }
            break;
        }
        case 156:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (laser)
                *((u8 *)laser + 1257) = (u8)ReadInt(rawEnemy, instruction, 1);
            break;
        }
        case 157:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (laser)
                *(f32 *)((u8 *)laser + 1200) = ReadFloat(rawEnemy, instruction, 1);
            break;
        }
        case 158:
        {
            laser = LaserSlot(rawEnemy, ReadInt(rawEnemy, instruction, 0));
            if (laser)
            {
                *(f32 *)((u8 *)laser + 1192) = ReadFloat(rawEnemy, instruction, 1);
                *(f32 *)((u8 *)laser + 1196) = ReadFloat(rawEnemy, instruction, 2);
            }
            break;
        }
        case 159:
            *WriteFloat(rawEnemy, instruction) =
                (ReadFloat(rawEnemy, instruction, 1) - ReadFloat(rawEnemy, instruction, 2)) *
                    ReadFloat(rawEnemy, instruction, 3) +
                ReadFloat(rawEnemy, instruction, 2);
            break;
        case 161:
            ByteAt(rawEnemy, 11819) = (ByteAt(rawEnemy, 11819) & 0xF7) |
                                      (8 * (ReadInt(rawEnemy, instruction, 0) & 1));
            break;
        case 155:
            if ((EclOperands::g_TargetFloat4BE408 >= FloatAt(rawEnemy, 4 * 2755) ||
                 FloatAt(rawEnemy, 4 * 2755) <= 96.0f) &&
                FloatAt(rawEnemy, 4 * 2755) <= 288.0f)
            {
                *WriteFloat(rawEnemy, instruction) =
                    EclOperands::g_TargetRng49FE20.RandomF32() * 1.5707964f - 0.78539819f;
            }
            else
            {
                *WriteFloat(rawEnemy, instruction) = TargetAddNormalizeAngle(
                    EclOperands::g_TargetRng49FE20.RandomF32() * 1.5707964f + 2.3561945f, 0.0f);
            }
            break;
        case 160:
            Target42F5A2(ReadInt(rawEnemy, instruction, 0));
            break;

        default:
            // The exact target falls through to the common instruction
            // advance for opcode values with no switch case.
            break;
        }

advance_instruction:
        Advance(instruction);
    }

post_ecl_dispatch:
    // Observed at 0x00416FC3--0x004172A7.  Living enemies with a configured
    // pose set select the primary ANM script from horizontal movement.  The
    // target stores the selected +0x900 script index at +0x1d8 and invokes
    // the manager-owned 0x0044EA20 member ABI for each of the five choices.
    if (IntAt(rawEnemy, 0x2BB8) > 0 && *(i16 *)(rawEnemy->bytes + 0x2E36) >= 0)
    {
        i32 pose = 0;
        if (ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) & 0x40)
        {
            if (FloatAt(rawEnemy, 0x2B18) < -0.01f)
                pose = 1;
            else if (FloatAt(rawEnemy, 0x2B18) > 0.01f)
                pose = 2;
        }
        else if (FloatAt(rawEnemy, 0x2B18) < -0.01f)
        {
            pose = 2;
        }
        else if (FloatAt(rawEnemy, 0x2B18) > 0.01f)
        {
            pose = 1;
        }

        if (ByteAt(rawEnemy, 0x2E2E) != pose)
        {
            i16 script;
            if (pose == 0)
            {
                if (ByteAt(rawEnemy, 0x2E2E) == 0xFF)
                    script = *(i16 *)(rawEnemy->bytes + 0x2E30);
                else if (ByteAt(rawEnemy, 0x2E2E) == 1)
                    script = *(i16 *)(rawEnemy->bytes + 0x2E32);
                else
                    script = *(i16 *)(rawEnemy->bytes + 0x2E34);
            }
            else if (pose == 1)
            {
                script = *(i16 *)(rawEnemy->bytes + 0x2E36);
            }
            else
            {
                script = *(i16 *)(rawEnemy->bytes + 0x2E38);
            }

            const i32 scriptId = script + 2304;
            *(i16 *)(rawEnemy->bytes + 472) = (i16)scriptId;
            ((SpellLifecycle::AnmManagerOverlay *)SpellLifecycle::g_TargetAnmManager4B9E44)
                ->SetAndExecuteScript(rawEnemy,
                                      *(void **)(SpellLifecycle::g_TargetAnmManager4B9E44 +
                                                 0x28EF0 + 4 * scriptId));
            ByteAt(rawEnemy, 0x2E2E) = (u8)pose;
        }
    }

    // Observed at 0x004172B0--0x004172C8: the post-pose enemy callback uses
    // ECX for the enemy and EDX for the adjacent callback argument.
    if (IntAt(rawEnemy, 0x6F4))
    {
        ((void(__fastcall *)(EclOperands::EnemyOverlay *, i32))IntAt(rawEnemy, 0x6F4))(
            rawEnemy, IntAt(rawEnemy, 0x6F8));
    }

    // Observed at 0x004172CE--0x004175EC.  Eight independently configured
    // interpolation entries start at Enemy+0x770; each is 0x30 bytes and
    // delegates frame advancement to the shared timer manager.  The target
    // preserves the position when an entry uses one of the three position
    // interpolation callbacks, then derives the resulting movement angle.
    i32 usedPositionInterpolation = 0;
    const f32 positionX = FloatAt(rawEnemy, 0x2B0C);
    const f32 positionY = FloatAt(rawEnemy, 0x2B10);
    const f32 positionZ = FloatAt(rawEnemy, 0x2B14);
    for (i32 entryIndex = 0; entryIndex < 8; ++entryIndex)
    {
        u8 *const entry = rawEnemy->bytes + 0x770 + 0x30 * entryIndex;
        if (*(i32 *)(entry + 0))
        {
            *(f32 *)(entry + 4) = *(f32 *)(entry + 12);
            g_EclTailTimerManager.Advance((i32 *)(entry + 12), (i32 *)(entry + 8));
            if (*(i32 *)(entry + 12) >= *(i32 *)(entry + 16))
            {
                *(i32 *)(entry + 12) = *(i32 *)(entry + 16);
                *(f32 *)(entry + 8) = 0.0f;
                *(i32 *)(entry + 4) = 0xFFC00000;
            }

            f32 fraction = ((f32)*(i32 *)(entry + 12) + *(f32 *)(entry + 8)) / *(i32 *)(entry + 16);
            switch (*(i32 *)(entry + 24))
            {
            case 1:
                fraction = fraction * fraction;
                break;
            case 2:
                fraction = fraction * fraction * fraction;
                break;
            case 3:
                fraction = fraction * fraction * fraction * fraction;
                break;
            case 4:
                fraction = 1.0f - fraction;
                fraction = 1.0f - fraction * fraction;
                break;
            case 5:
                fraction = 1.0f - fraction;
                fraction = 1.0f - fraction * fraction * fraction;
                break;
            case 6:
                fraction = 1.0f - fraction;
                fraction = 1.0f - fraction * fraction * fraction * fraction;
                break;
            }

            ((void(__fastcall *)(EclOperands::EnemyOverlay *, u8 *, f32)) * (i32 *)(entry + 0))(
                rawEnemy, entry, fraction);
            if (*(i32 *)(entry + 12) >= *(i32 *)(entry + 16))
                *(i32 *)(entry + 0) = 0;

            if (*(f32 *)(entry + 44) == 10018.0f || *(f32 *)(entry + 44) == 10019.0f ||
                *(f32 *)(entry + 44) == 10020.0f)
            {
                usedPositionInterpolation = 1;
            }
        }
    }

    if (usedPositionInterpolation)
    {
        FloatAt(rawEnemy, 0x2B18) = FloatAt(rawEnemy, 0x2B0C) - positionX;
        FloatAt(rawEnemy, 0x2B1C) = FloatAt(rawEnemy, 0x2B10) - positionY;
        FloatAt(rawEnemy, 0x2B54) = (f32)atan2(FloatAt(rawEnemy, 0x2B1C), FloatAt(rawEnemy, 0x2B18));
        FloatAt(rawEnemy, 0x2B0C) = positionX;
        FloatAt(rawEnemy, 0x2B10) = positionY;
        FloatAt(rawEnemy, 0x2B14) = positionZ;
    }

    // Common tail at 0x00417688--0x004176C7.  The instruction pointer is
    // committed before the per-enemy script timer advances.
    CurrentInstruction(rawEnemy) = instruction;
    IntAt(rawEnemy, 0x6E8) = IntAt(rawEnemy, 0x6F0);
    g_EclTailTimerManager.Advance((i32 *)(rawEnemy->bytes + 0x6F0),
                                  (i32 *)(rawEnemy->bytes + 0x6EC));

    // Target-observed spell bonus clock at 0x00417700--0x0041778E.  The
    // subframe word at +0x12FE0E4 is deliberately reinterpreted as a float;
    // its adjacent whole-frame word is the first argument to Advance.
    if ((ByteAt(rawEnemy, 0x2E29) & 0x40) != 0 && !ByteAt(rawEnemy, 0x2E17) &&
        g_TargetSpellActive12FE0C8 && g_TargetCaptureEligible12FE0C4)
    {
        if ((ByteAt(rawEnemy, 0x2E2A) & 0x40) == 0)
        {
            g_TargetSpellBaseScore12FE0CC = (i32)(g_TargetSpellScores49F1B8[g_TargetSpellId12FE0D8] -
                                                   g_TargetSpellPerTick12FE0D4 *
                                                       (g_TargetSpellTimerSubframe12FE0E8 +
                                                        *(f32 *)&g_TargetSpellTimerCurrent12FE0E4) /
                                                       60.0f);
            g_TargetSpellBaseScore12FE0CC -= g_TargetSpellBaseScore12FE0CC % 10;
        }
        g_TargetSpellTimerPrevious12FE0E0 = g_TargetSpellTimerSubframe12FE0E8;
        g_EclTailTimerManager.Advance(&g_TargetSpellTimerSubframe12FE0E8,
                                      &g_TargetSpellTimerCurrent12FE0E4);
    }

    // Final presentation-control tail, 0x00417793--0x0041782E.
    if ((ByteAt(rawEnemy, 0x2E29) & 0x40) != 0 && g_TargetDifficulty62F85C >= 7)
    {
        if (g_TargetControl4D44F8 && g_TargetSpellActive12FE0C8 && g_TargetSpellId12FE0D8 >= 118)
        {
            rawEnemy->bytes[0x2E2B] |= 4;
            *(u16 *)(rawEnemy->bytes + 0x2E2C) = 1;
        }
        else if (*(u16 *)(rawEnemy->bytes + 0x2E2C) > 0)
        {
            --*(u16 *)(rawEnemy->bytes + 0x2E2C);
        }
        else
        {
            rawEnemy->bytes[0x2E2B] &= 0xFB;
        }
    }

    return ZUN_SUCCESS;
}
#undef rawEnemy

} // namespace th07
