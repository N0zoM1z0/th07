#include "EclManager.hpp"
#include "Timer.hpp"

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
            const i32 limit = ReadInt(rawEnemy, instruction, 1);
            *WriteInt(rawEnemy, instruction) =
                ReadInt(rawEnemy, instruction, 2) +
                (limit ? EclOperands::g_TargetRng49FE20.RandomU32() % limit : 0);
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

        // 0x2d--0x3f are target-observed direct Enemy state transitions.  No
        // complete Enemy type is implied: each store below is an attested
        // offset in this dispatcher.
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
            rawEnemy->bytes[ENEMY_MOVEMENT_FLAGS] &= 0xFC;
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
