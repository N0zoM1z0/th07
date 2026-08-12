#include "inttypes.hpp"

#include <string.h>

namespace th07
{
namespace SpellLifecycle
{

// Observed target layout: only these instruction bytes are read by 0x40FC90.
struct SpellStartInstruction
{
    u8 unknown00[0xC];
    i16 portraitScript;
    u16 spellId;
    u8 encryptedName[48];
};

// Private byte overlays retain observed offsets without asserting the complete
// TH07 Enemy, effect, GUI, or score-record layouts.
struct EnemyOverlay
{
    u8 bytes[1];
};

struct EffectOverlay
{
    u8 bytes[1];
};

struct SpellVmOverlay
{
    u8 bytes[1];

    void SetAndExecuteScript(void *script);
};

struct GuiOverlay
{
    void ShowSpellcard(i16 portraitScript, const char *name);
    void EndSpellcardDisplay();
    void ShowBonusScore(i32 score);
    void ShowSpellcardBonus(i32 score);
};

struct UiStateOverlay
{
    void SetSpellUiState(i32 enabled);
    i32 CalculateBonus(i32 score, i32 remaining);
};

struct BulletManagerOverlay
{
    i32 DespawnBullets(i32 score, i32 mode);
};

struct SoundOverlay
{
    void PlaySound(i32 soundId, i32 arg);
};

#define I32_AT(base, offset) (*(i32 *)((base) + (offset)))
#define U32_AT(base, offset) (*(u32 *)((base) + (offset)))
#define U16_AT(base, offset) (*(u16 *)((base) + (offset)))
#define U8_AT(base, offset) (*(u8 *)((base) + (offset)))
#define F32_AT(base, offset) (*(f32 *)((base) + (offset)))

// These externally owned data symbols deliberately carry target-address
// labels rather than speculative game-wide names.  Their mappings are left to
// the coordinator/owning global lanes.
extern GuiOverlay g_TargetGui49FBF0;
extern UiStateOverlay g_TargetUiState62F958;
extern UiStateOverlay g_TargetUiState9A9B00;
extern BulletManagerOverlay g_TargetBulletManager62F958;
extern SoundOverlay g_TargetSound4BA0D8;
extern i32 g_TargetStartVisual1348014;
extern i32 g_TargetStartVisual1348018;
extern i32 g_TargetStartVmCount1348020;
extern i32 g_TargetStartVmBase1348024;
extern u8 g_TargetStartVms1348028[];
extern u8 *g_TargetAnmManager4B9E44;
extern i32 g_TargetSpellActive12FE0C8;
extern i32 g_TargetCaptureEligible12FE0C4;
extern i32 g_TargetSpellBaseScore12FE0CC;
extern i32 g_TargetSpellBonusAccum12FE0D0;
extern i32 g_TargetSpellPerTick12FE0D4;
extern i32 g_TargetSpellId12FE0D8;
extern i32 g_TargetSpellTimerPrevious12FE0E0;
extern i32 g_TargetSpellTimerCurrent12FE0E4;
extern i32 g_TargetSpellTimerSubframe12FE0E8;
extern EnemyOverlay *g_TargetSpellBosses12FE098[8];
extern i32 g_TargetSpellScores49F1B8[];
extern u8 g_TargetSpellRecords626288[];
extern u8 g_TargetCharacterIndex62F647;
extern u32 g_TargetReplayFlags62F648;
extern u8 *g_TargetScoreState626278;

extern EffectOverlay *__cdecl TargetCreateEffect(i32 effectType, const void *position, i32 arg2, i32 arg3, i32 arg4);

// Observed score-record access offsets.  The field meanings are inferred from
// the target update pattern and TH06's capture history, not from a recovered
// TH07 struct definition.
static __forceinline i32 NameLength(const char *text)
{
    i32 length = 0;

    while (text[length])
        ++length;
    return length;
}

static __forceinline i32 RecordChecksum(const u8 *record)
{
    i32 checksum = 0;
    i32 i;
    i32 length = NameLength((const char *)record + 43);

    while (length > 0)
        checksum += ((const char *)record)[43 + --length];
    for (i = 0; i < 7; ++i)
        checksum += I32_AT(record, 12 + 4 * i) + U16_AT(record, 92 + 2 * i) + U16_AT(record, 106 + 2 * i);
    return checksum;
}

static __forceinline void ResetRecordCounts(u8 *record)
{
    i32 i;

    for (i = 0; i < 7; ++i)
    {
        I32_AT(record, 12 + 4 * i) = 0;
        U16_AT(record, 92 + 2 * i) = 0;
        U16_AT(record, 106 + 2 * i) = 0;
    }
}

static __forceinline u8 *SpellRecord(i32 spellId)
{
    return g_TargetSpellRecords626288 + 120 * spellId;
}

// Observed target ABI: 0x40FC90 accepts Enemy in ECX and raw ECL instruction
// in EDX.  The functional names in this file are inferred only.
u32 __fastcall StartSpellcard(EnemyOverlay *enemy, const SpellStartInstruction *instruction)
{
    u8 name[48];
    u32 i;
    u8 *record;
    i32 checksum;

    memcpy(name, instruction->encryptedName, sizeof(name));
    for (i = 0; i < 48; ++i)
        name[i] ^= 0xAA;

    g_TargetGui49FBF0.ShowSpellcard(instruction->portraitScript, (const char *)name);
    g_TargetUiState62F958.SetSpellUiState(1);
    g_TargetStartVisual1348014 = 1;
    g_TargetStartVisual1348018 = 0;

    for (i = 0; (i32)i < g_TargetStartVmCount1348020; ++i)
    {
        i32 scriptId = (i32)i + g_TargetStartVmBase1348024 + 732;
        SpellVmOverlay *vm = (SpellVmOverlay *)(g_TargetStartVms1348028 + 588 * i);
        u8 *anm = g_TargetAnmManager4B9E44;

        U16_AT(vm->bytes, 472) = scriptId;
        vm->SetAndExecuteScript(*(void **)(anm + 0x28EF0 + 4 * scriptId));
    }

    g_TargetSpellActive12FE0C8 = 1;
    g_TargetCaptureEligible12FE0C4 = 1;
    g_TargetSpellId12FE0D8 = instruction->spellId;
    g_TargetSpellBaseScore12FE0CC = g_TargetSpellScores49F1B8[g_TargetSpellId12FE0D8];
    g_TargetSpellBonusAccum12FE0D0 = 0;
    g_TargetSpellPerTick12FE0D4 = g_TargetSpellBaseScore12FE0CC / (I32_AT(enemy->bytes, 0x2EDC) / 60 + 10);
    g_TargetSpellTimerSubframe12FE0E8 = 0;
    g_TargetSpellTimerCurrent12FE0E4 = 0;
    g_TargetSpellTimerPrevious12FE0E0 = -999;

    F32_AT(enemy->bytes, 0x2BA8) = -0.5f;
    F32_AT(enemy->bytes, 0x2BAC) = 0.5f;
    U16_AT(enemy->bytes, 0x2BB0) = 0;
    U16_AT(enemy->bytes, 0x2BB2) = 0;
    U16_AT(enemy->bytes, 0x2BB4) = 0;
    U16_AT(enemy->bytes, 0x2BB6) = 0;
    {
        i32 *effectTimerA;
        i32 enemyTimer;
        i32 *effectTimerB;

        I32_AT(enemy->bytes, 0x2EB0) = (i32)TargetCreateEffect(25, enemy->bytes + 0x2B0C, 1, 1, -1);
        effectTimerA = (i32 *)(I32_AT(enemy->bytes, 0x2EB0) + 120);
        effectTimerA[2] = 0;
        effectTimerA[1] = 0;
        effectTimerA[0] = -999;
        enemyTimer = I32_AT(enemy->bytes, 0x2EDC);
        effectTimerB = (i32 *)(I32_AT(enemy->bytes, 0x2EB0) + 180);
        effectTimerB[2] = enemyTimer;
        effectTimerB[1] = 0;
        effectTimerB[0] = -999;
        U8_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 196) = 0;
        I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 536) = I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 24);
        I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 540) = I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 28);
        I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 544) = 0x3E000000;
        I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 548) = 0x3E000000;
        I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 588) = I32_AT(enemy->bytes, 0x2B0C);
        I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 592) = I32_AT(enemy->bytes, 0x2B10);
        I32_AT((u8 *)I32_AT(enemy->bytes, 0x2EB0), 596) = I32_AT(enemy->bytes, 0x2B14);
    }
    U8_AT(enemy->bytes, 0x2E2B) &= ~2;

    if ((g_TargetReplayFlags62F648 >> 3) & 1)
        return 1;

    record = SpellRecord(g_TargetSpellId12FE0D8);
    {
        u8 *recordName = record + 43;

        for (i = 0; ; ++i)
        {
            recordName[i] = name[i];
            if (!name[i])
                break;
        }
    }
    checksum = RecordChecksum(record);
    if ((u8)checksum != record[42])
        ResetRecordCounts(record);
    if (U16_AT(record, 92 + 2 * g_TargetCharacterIndex62F647) < 9999)
        ++U16_AT(record, 92 + 2 * g_TargetCharacterIndex62F647);
    if (U16_AT(record, 104) < 9999)
        ++U16_AT(record, 104);
    record[42] = (u8)RecordChecksum(record);
    return (u32)record;
}

// Observed target ABI: 0x4101A0 saves ECX and EDX despite not reading either,
// then returns with plain ret.  This matches a fastcall ECL opcode helper whose
// Enemy/instruction parameters are unused by its global lifecycle work.
// Capture/record role labels below are semantic inferences corroborated by TH06.
void __fastcall FinishSpellcard(EnemyOverlay *unusedEnemy, const void *unusedInstruction)
{
    i32 value;

    if (g_TargetSpellActive12FE0C8)
    {
        g_TargetGui49FBF0.EndSpellcardDisplay();
        if (g_TargetSpellActive12FE0C8 == 1)
        {
            value = g_TargetBulletManager62F958.DespawnBullets(8000, 1);
            value = g_TargetUiState9A9B00.CalculateBonus(8000, value);
            if (value)
            {
                I32_AT(g_TargetScoreState626278, 4) += value / 10;
                g_TargetGui49FBF0.ShowBonusScore(value);
            }

            if (g_TargetCaptureEligible12FE0C4)
            {
                u8 *record = SpellRecord(g_TargetSpellId12FE0D8);
                value = g_TargetSpellBonusAccum12FE0D0 + g_TargetSpellBaseScore12FE0CC;
                g_TargetGui49FBF0.ShowSpellcardBonus(value);
                I32_AT(g_TargetScoreState626278, 4) += value / 10;

                if (!((g_TargetReplayFlags62F648 >> 3) & 1))
                {
                    if ((u8)RecordChecksum(record) != record[42])
                        ResetRecordCounts(record);
                    if (I32_AT(record, 12 + 4 * g_TargetCharacterIndex62F647) < value)
                        I32_AT(record, 12 + 4 * g_TargetCharacterIndex62F647) = value;
                    if (I32_AT(record, 36) < value)
                        I32_AT(record, 36) = value;
                    if (U16_AT(record, 106 + 2 * g_TargetCharacterIndex62F647) < 9999)
                        ++U16_AT(record, 106 + 2 * g_TargetCharacterIndex62F647);
                    if (U16_AT(record, 118) < 9999)
                        ++U16_AT(record, 118);
                    record[42] = (u8)RecordChecksum(record);
                }
                ++I32_AT(g_TargetScoreState626278, 28);
            }
        }

        g_TargetSpellActive12FE0C8 = 0;
        for (i32 i = 0; i < 8; ++i)
        {
            if (g_TargetSpellBosses12FE098[i] && I32_AT(g_TargetSpellBosses12FE098[i]->bytes, 0x2EB0))
            {
                U8_AT((u8 *)I32_AT(g_TargetSpellBosses12FE098[i]->bytes, 0x2EB0), 716) = 0;
                I32_AT(g_TargetSpellBosses12FE098[i]->bytes, 0x2EB0) = 0;
            }
        }
        g_TargetSound4BA0D8.PlaySound(15, 0);
    }
    g_TargetStartVisual1348014 = 0;
}

#undef F32_AT
#undef U8_AT
#undef U16_AT
#undef U32_AT
#undef I32_AT

} // namespace SpellLifecycle
} // namespace th07
