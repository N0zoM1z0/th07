#include "EclManager.hpp"

#include <d3dx8math.h>
#include <math.h>
#include <string.h>

namespace th07
{
// The active difficulty bitmask is read at 0x004106EC.  Its owner and public
// name belong to the game-state lane, so retain the target-address label.
extern u8 g_TargetDifficultyMask626284;
extern EclManager g_TargetEclManager1347938;
extern i32 g_TargetSpellActive12FE0C8;
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

    void SetAndExecuteScript(void *script);
};
struct AnmManagerOverlay
{
    u8 bytes[1];

    void ConfigureBoss(EnemyOverlay *enemy, void *source, i32 value);
};
struct SpellStartInstruction;
u32 __fastcall StartSpellcard(EnemyOverlay *enemy, const SpellStartInstruction *instruction);
void __fastcall FinishSpellcard(EnemyOverlay *enemy, const void *instruction);
extern u8 *g_TargetAnmManager4B9E44;
extern EnemyOverlay *g_TargetSpellBosses12FE098[8];
} // namespace SpellLifecycle

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
    ENEMY_SCRIPT_TIMER_PREVIOUS = 0x764,
    ENEMY_SCRIPT_TIMER_CURRENT = 0x768,
    ENEMY_SCRIPT_TIMER_LIMIT = 0x76C,
    ENEMY_CONTEXT_STACK_BASE = 0x8FC,
    ENEMY_CONTEXT_STACK_DEPTH = 0x2A7C,
    ENEMY_INTERRUPT_TABLE = 0x2A88,
    ENEMY_RUN_INTERRUPT = 0x2B08,
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

static __forceinline void EnterInterrupt(EclOperands::EnemyOverlay *enemy)
{
    if (IntAt(enemy, ENEMY_RUN_INTERRUPT) < 0)
        return;

    if ((ByteAt(enemy, ENEMY_DISABLE_CALL_STACK) & 0x20) == 0)
        SaveEclContext(enemy);

    g_TargetEclManager1347938.CallEclSub((EnemyEclContext *)(enemy->bytes + ENEMY_CURRENT_INSTRUCTION),
                                         (i16)IntAt(enemy, ENEMY_INTERRUPT_TABLE +
                                                           4 * IntAt(enemy, ENEMY_RUN_INTERRUPT)));
    if (IntAt(enemy, ENEMY_CONTEXT_STACK_DEPTH) < 15)
        ++IntAt(enemy, ENEMY_CONTEXT_STACK_DEPTH);
    IntAt(enemy, ENEMY_RUN_INTERRUPT) = -1;
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
    EclOperands::EnemyOverlay *const rawEnemy = (EclOperands::EnemyOverlay *)enemy;
    EnterInterrupt(rawEnemy);
    RunEclInstruction *instruction = CurrentInstruction(rawEnemy);
    void *laser;

    for (;;)
    {
        if (instruction == 0)
            return ZUN_ERROR;

        // 0x004106EC tests the instruction difficulty byte before dispatch.
        // A clear bit means that this instruction is skipped but still
        // consumes its encoded extent.
        if ((instruction->difficultyMask & g_TargetDifficultyMask626284) == 0)
        {
            Advance(instruction);
            CurrentInstruction(rawEnemy) = instruction;
            continue;
        }

        if (instruction->time != ScriptTime(rawEnemy))
        {
            CurrentInstruction(rawEnemy) = instruction;
            return ZUN_SUCCESS;
        }

        switch (instruction->opcode)
        {
        case 1: // target 0x00410744
            return ZUN_ERROR;

        case 2: // jump: time at +0x0c, byte displacement at +0x10
            Jump(rawEnemy, instruction);
            continue;

        case 3: // decrement an integer lvalue, then jump while it remains positive
        {
            i32 *const value = EclOperands::ResolveIntLValue(rawEnemy, &instruction->operand[2],
                                                              instruction->operandFlags, 2);
            --*value;
            if (ReadInt(rawEnemy, instruction, 2) > 0)
            {
                Jump(rawEnemy, instruction);
                continue;
            }
            break;
        }

        case 4: // integer assignment
            *WriteInt(rawEnemy, instruction) = ReadInt(rawEnemy, instruction, 1);
            break;

        case 5: // float assignment
            *WriteFloat(rawEnemy, instruction) = ReadFloat(rawEnemy, instruction, 1);
            break;

        case 6:
        {
            const i32 limit = ReadInt(rawEnemy, instruction, 1);
            *WriteInt(rawEnemy, instruction) = limit ? EclOperands::g_TargetRng49FE20.RandomU32() % limit : 0;
            break;
        }
        case 7:
        {
            const i32 limit = ReadInt(rawEnemy, instruction, 1);
            *WriteInt(rawEnemy, instruction) =
                ReadInt(rawEnemy, instruction, 2) +
                (limit ? EclOperands::g_TargetRng49FE20.RandomU32() % limit : 0);
            break;
        }
        case 8:
            *WriteFloat(rawEnemy, instruction) =
                EclOperands::g_TargetRng49FE20.RandomF32() * ReadFloat(rawEnemy, instruction, 1);
            break;
        case 9:
            *WriteFloat(rawEnemy, instruction) = EclOperands::g_TargetRng49FE20.RandomF32() *
                                                     ReadFloat(rawEnemy, instruction, 1) +
                                                 ReadFloat(rawEnemy, instruction, 2);
            break;
        case 10:
            *WriteInt(rawEnemy, instruction) =
                ReadInt(rawEnemy, instruction, 1) *
                ((EclOperands::g_TargetRng49FE20.RandomU32() & 1) ? 1 : -1);
            break;
        case 11:
            *WriteFloat(rawEnemy, instruction) =
                ((EclOperands::g_TargetRng49FE20.RandomU32() & 1) ? 1.0f : -1.0f) *
                ReadFloat(rawEnemy, instruction, 1);
            break;

        // 0x0c--0x10: target integer ALU block.  The order of operands is
        // taken from the target stores, rather than from the TH06 enum.
        case 12:
            *WriteInt(rawEnemy, instruction) = ReadInt(rawEnemy, instruction, 1) + ReadInt(rawEnemy, instruction, 2);
            break;
        case 13:
            *WriteInt(rawEnemy, instruction) = ReadInt(rawEnemy, instruction, 1) - ReadInt(rawEnemy, instruction, 2);
            break;
        case 14:
            *WriteInt(rawEnemy, instruction) = ReadInt(rawEnemy, instruction, 1) * ReadInt(rawEnemy, instruction, 2);
            break;
        case 15:
            *WriteInt(rawEnemy, instruction) = ReadInt(rawEnemy, instruction, 1) / ReadInt(rawEnemy, instruction, 2);
            break;
        case 16:
            *WriteInt(rawEnemy, instruction) = ReadInt(rawEnemy, instruction, 1) % ReadInt(rawEnemy, instruction, 2);
            break;
        case 17:
            ++*WriteInt(rawEnemy, instruction);
            break;
        case 18:
            --*WriteInt(rawEnemy, instruction);
            break;

        // 0x13--0x19: target float ALU block.  The direct Ecl_ResolveFloat
        // and Ecl_ResolveFloatLValue calls are recorded in the cached packet.
        case 19:
            *WriteFloat(rawEnemy, instruction) = ReadFloat(rawEnemy, instruction, 1) + ReadFloat(rawEnemy, instruction, 2);
            break;
        case 20:
            *WriteFloat(rawEnemy, instruction) = ReadFloat(rawEnemy, instruction, 1) - ReadFloat(rawEnemy, instruction, 2);
            break;
        case 21:
            *WriteFloat(rawEnemy, instruction) = ReadFloat(rawEnemy, instruction, 1) * ReadFloat(rawEnemy, instruction, 2);
            break;
        case 22:
            *WriteFloat(rawEnemy, instruction) = ReadFloat(rawEnemy, instruction, 1) / ReadFloat(rawEnemy, instruction, 2);
            break;
        case 23:
            *WriteFloat(rawEnemy, instruction) = (f32)fmod(ReadFloat(rawEnemy, instruction, 1),
                                                            ReadFloat(rawEnemy, instruction, 2));
            break;
        case 24:
            *WriteFloat(rawEnemy, instruction) = (f32)sin(ReadFloat(rawEnemy, instruction, 1));
            break;
        case 25:
            *WriteFloat(rawEnemy, instruction) = (f32)cos(ReadFloat(rawEnemy, instruction, 1));
            break;

        // The cached target at 0x004114DA resolves the four float operands
        // before calling the atan helper.  Its exact helper ABI is still an
        // owner-lane dependency, so do not substitute a host-library atan2.
        case 26:
            return ZUN_ERROR;

        // Conditional jumps 0x1c--0x27 update the instruction time from +0x14
        // and branch by the signed +0x18 displacement.  Their comparison
        // polarity is target-observed at 0x00411945--0x00411F1B.
        case 28:
            if (ReadInt(rawEnemy, instruction, 0) == ReadInt(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 29:
            if (ReadFloat(rawEnemy, instruction, 0) == ReadFloat(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 30:
            if (ReadInt(rawEnemy, instruction, 0) != ReadInt(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 31:
            if (ReadFloat(rawEnemy, instruction, 0) != ReadFloat(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 32:
            if (ReadInt(rawEnemy, instruction, 0) < ReadInt(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 33:
            if (ReadFloat(rawEnemy, instruction, 0) < ReadFloat(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 34:
            if (ReadInt(rawEnemy, instruction, 0) > ReadInt(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 35:
            if (ReadFloat(rawEnemy, instruction, 0) > ReadFloat(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 36:
            if (ReadInt(rawEnemy, instruction, 0) <= ReadInt(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 37:
            if (ReadFloat(rawEnemy, instruction, 0) <= ReadFloat(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 38:
            if (ReadInt(rawEnemy, instruction, 0) >= ReadInt(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;
        case 39:
            if (ReadFloat(rawEnemy, instruction, 0) >= ReadFloat(rawEnemy, instruction, 1)) { ConditionalJump(rawEnemy, instruction); continue; }
            break;

        case 40:
            *WriteFloat(rawEnemy, instruction) =
                TargetAddNormalizeAngle(ReadFloat(rawEnemy, instruction, 0), 0.0f);
            break;

        // 0x2d--0x3f are target-observed direct Enemy state transitions.  No
        // complete Enemy type is implied: each store below is an attested
        // offset in this dispatcher.
        case 45:
            IntAt(rawEnemy, ENEMY_SCRIPT_TIMER_LIMIT) = ReadInt(rawEnemy, instruction, 0);
            IntAt(rawEnemy, ENEMY_SCRIPT_TIMER_CURRENT) = 0;
            IntAt(rawEnemy, ENEMY_SCRIPT_TIMER_PREVIOUS) = -999;
            break;

        case 46:
            FloatAt(rawEnemy, 0x2B0C) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 0x2B10) = ReadFloat(rawEnemy, instruction, 1);
            FloatAt(rawEnemy, 0x2B14) = ReadFloat(rawEnemy, instruction, 2);
            TargetClampEnemy(rawEnemy);
            break;

        case 47:
            FloatAt(rawEnemy, 0x2B18) = ReadFloat(rawEnemy, instruction, 0);
            FloatAt(rawEnemy, 0x2B1C) = ReadFloat(rawEnemy, instruction, 1);
            FloatAt(rawEnemy, 0x2B20) = ReadFloat(rawEnemy, instruction, 2);
            FloatAt(rawEnemy, 0x2B54) = FloatAt(rawEnemy, 0x2B1C);
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) &= 0xFC;
            break;

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
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) |= 3;
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
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) |= 3;
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
            ByteAt(rawEnemy, 11817) |= 0x80;
            break;

        case 63:
            ByteAt(rawEnemy, 11817) &= ~0x80;
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
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) |= 0x20;
            break;
        case 76:
            ByteAt(rawEnemy, ENEMY_MOVEMENT_FLAGS) &= ~0x20;
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
            IntAt(rawEnemy, 4 * 572) = !laser || !*(i32 *)((u8 *)laser + 1236);
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
            ((SpellLifecycle::SpellVmOverlay *)rawEnemy)->SetAndExecuteScript(
                *(void **)(SpellLifecycle::g_TargetAnmManager4B9E44 + 0x28EF0 + 4 * scriptId));
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
                ((SpellLifecycle::SpellVmOverlay *)(rawEnemy->bytes + 4 * (147 * slot + 147)))
                    ->SetAndExecuteScript(*(void **)(SpellLifecycle::g_TargetAnmManager4B9E44 +
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
            break;
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
                const i32 itemType = (i32)(unsigned __int64)*(f32 *)(g_TargetScoreState626278 + 124) >= 128
                                         ? 1
                                         : (item ? 0 : 2);
                g_ItemManager.Spawn(&position, itemType, 0);
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

        Advance(instruction);
        CurrentInstruction(rawEnemy) = instruction;
    }
}

} // namespace th07
