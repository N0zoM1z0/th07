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

struct EffectManagerOverlay
{
    EffectOverlay *SpawnParticlesColored(i32 effectType, const void *position, i32 arg2, i32 arg3, i32 arg4);
};

struct SpellVmOverlay
{
    u8 bytes[1];
};

struct AnmManagerOverlay
{
    u8 unknown00[0x28EF0];
    void *scripts[1];

    void SetAndExecuteScript(void *vm, void *script);
};

struct GuiEndSubstate
{
    u8 unknown000[0x1C6];
    i16 word1C6;
};

struct GuiOverlay
{
    u8 unknown00[8];
    u8 *state;

    void ShowSpellcard(i32 portraitScript, const char *name);
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

struct ScoreStateOverlay
{
    i32 unknown00;
    i32 score;
    u8 unknown08[0x14];
    i32 capturedSpellcards;
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
extern ScoreStateOverlay *g_TargetScoreState626278;
extern EffectManagerOverlay g_TargetEffectManager12FE250;

void GuiOverlay::EndSpellcardDisplay()
{
    GuiEndSubstate *first;
    GuiEndSubstate *second;

    *reinterpret_cast<i16 *>(state + 0x6926) = 1;
    first = reinterpret_cast<GuiEndSubstate *>(state + 0x6BF8);
    *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(first) + 0x1C6) = 2;
    second = reinterpret_cast<GuiEndSubstate *>(state + 0x7774);
    *reinterpret_cast<i16 *>(reinterpret_cast<u8 *>(second) + 0x1C6) = 2;
}

// Observed target ABI: 0x40FC90 accepts Enemy in ECX and raw ECL instruction
// in EDX.  The functional names in this file are inferred only.
#pragma var_order(i, name, record, recordIndex, checksum, originalChecksum, scriptId, vm, anm, effectTimerA, enemyTimer, effectTimerB, enemy, instruction)
u32 __fastcall StartSpellcard(EnemyOverlay *enemy, const SpellStartInstruction *instruction)
{
    u8 name[48];
    u32 i;
    u8 *record;
    i32 recordIndex;
    i32 checksum;
    i32 originalChecksum;
    i32 scriptId;
    SpellVmOverlay *vm;
    AnmManagerOverlay *anm;
    i32 *effectTimerA;
    i32 enemyTimer;
    i32 *effectTimerB;

    memcpy(name, instruction->encryptedName, sizeof(name));
    for (i = 0; i < 48; ++i)
        name[i] ^= 0xAA;

    g_TargetGui49FBF0.ShowSpellcard(instruction->portraitScript, (const char *)name);
    g_TargetUiState62F958.SetSpellUiState(1);
    g_TargetStartVisual1348014 = 1;
    g_TargetStartVisual1348018 = 0;

    for (i = 0; (i32)i < g_TargetStartVmCount1348020; ++i)
    {
        scriptId = (i32)i + g_TargetStartVmBase1348024 + 732;
        vm = (SpellVmOverlay *)(g_TargetStartVms1348028 + 588 * i);
        anm = reinterpret_cast<AnmManagerOverlay *>(g_TargetAnmManager4B9E44);

        U16_AT(vm->bytes, 472) = scriptId;
        anm->SetAndExecuteScript(vm, anm->scripts[scriptId]);
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
    I32_AT(enemy->bytes, 0x2EB0) = (i32)g_TargetEffectManager12FE250.SpawnParticlesColored(25, enemy->bytes + 0x2B0C, 1, 1, -1);
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
    __asm
    {
        mov edx, DWORD PTR[enemy]
        mov eax, DWORD PTR[edx + 0x2EB0]
        mov ecx, DWORD PTR[eax + 0x18]
        mov edx, DWORD PTR[eax + 0x1C]
        mov eax, DWORD PTR[enemy]
        mov eax, DWORD PTR[eax + 0x2EB0]
        mov DWORD PTR[eax + 0x218], ecx
        mov DWORD PTR[eax + 0x21C], edx
    }
    __asm
    {
        mov ecx, DWORD PTR[enemy]
        mov edx, DWORD PTR[ecx + 0x2EB0]
        mov DWORD PTR[edx + 0x220], 0x3E000000
        mov eax, DWORD PTR[enemy]
        mov ecx, DWORD PTR[eax + 0x2EB0]
        mov DWORD PTR[ecx + 0x224], 0x3E000000
    }
    __asm
    {
        mov edx, DWORD PTR[enemy]
        add edx, 0x2B0C
        mov eax, DWORD PTR[enemy]
        mov ecx, DWORD PTR[eax + 0x2EB0]
        add ecx, 0x24C
        mov eax, DWORD PTR[edx]
        mov DWORD PTR[ecx], eax
        mov eax, DWORD PTR[edx + 4]
        mov DWORD PTR[ecx + 4], eax
        mov edx, DWORD PTR[edx + 8]
        mov DWORD PTR[ecx + 8], edx
    }
    __asm
    {
        mov eax, DWORD PTR[enemy]
        mov cl, BYTE PTR[eax + 0x2E2B]
        and cl, 0xFD
        mov edx, DWORD PTR[enemy]
        mov BYTE PTR[edx + 0x2E2B], cl
    }

    __asm
    {
        mov eax, DWORD PTR[g_TargetReplayFlags62F648]
        shr eax, 3
        and eax, 1
        test eax, eax
        jnz spell_start_return
    }

    __asm
    {
        mov ecx, DWORD PTR[g_TargetSpellId12FE0D8]
        imul ecx, 120
        add ecx, OFFSET g_TargetSpellRecords626288
        mov DWORD PTR[record], ecx
    }
    checksum = 0;
    strcpy((char *)record + 43, (const char *)name);
    recordIndex = strlen((const char *)record + 43);
    while (recordIndex > 0)
        checksum += ((i8 *)record)[43 + --recordIndex];
    originalChecksum = checksum;
    for (recordIndex = 0; recordIndex < 7; ++recordIndex)
    {
        checksum += U16_AT(record, 106 + 2 * recordIndex);
        checksum += U16_AT(record, 92 + 2 * recordIndex);
        checksum += I32_AT(record, 12 + 4 * recordIndex);
    }
    if (record[42] != (u8)checksum)
    {
        for (recordIndex = 0; recordIndex < 7; ++recordIndex)
        {
            U16_AT(record, 106 + 2 * recordIndex) = 0;
            U16_AT(record, 92 + 2 * recordIndex) = 0;
            I32_AT(record, 12 + 4 * recordIndex) = 0;
        }
    }
    if (U16_AT(record, 92 + 2 * g_TargetCharacterIndex62F647) < 9999)
        ++U16_AT(record, 92 + 2 * g_TargetCharacterIndex62F647);
    if (U16_AT(record, 104) < 9999)
        ++U16_AT(record, 104);
    for (recordIndex = 0; recordIndex < 7; ++recordIndex)
    {
        originalChecksum += U16_AT(record, 106 + 2 * recordIndex);
        originalChecksum += U16_AT(record, 92 + 2 * recordIndex);
        originalChecksum += I32_AT(record, 12 + 4 * recordIndex);
    }
    record[42] = (u8)originalChecksum;
    __asm
    {
    spell_start_return:
    }
}

// Observed target ABI: 0x4101A0 saves ECX and EDX despite not reading either,
// then returns with plain ret.  This matches a fastcall ECL opcode helper whose
// Enemy/instruction parameters are unused by its global lifecycle work.
// Capture/record role labels below are semantic inferences corroborated by TH06.
#pragma var_order(value, record, i, characterIndex, checksum, originalChecksum, bossIndex, unusedEnemy, unusedInstruction)
void __fastcall FinishSpellcard(EnemyOverlay *unusedEnemy, const void *unusedInstruction)
{
    i32 value;
    u8 *record;
    i32 i;
    i32 characterIndex;
    i32 checksum;
    i32 originalChecksum;
    i32 bossIndex;

    if (g_TargetSpellActive12FE0C8)
    {
        g_TargetGui49FBF0.EndSpellcardDisplay();
        if (g_TargetSpellActive12FE0C8 == 1)
        {
            value = g_TargetBulletManager62F958.DespawnBullets(8000, 1);
            value = g_TargetUiState9A9B00.CalculateBonus(8000, value);
            if (value)
            {
                __asm
                {
                    mov ecx, DWORD PTR[g_TargetScoreState626278]
                    mov eax, DWORD PTR[value]
                    cdq
                    mov esi, 10
                    idiv esi
                    add eax, DWORD PTR[ecx + 4]
                    mov edx, DWORD PTR[g_TargetScoreState626278]
                    mov DWORD PTR[edx + 4], eax
                }
                __asm
                {
                    mov eax, DWORD PTR[value]
                    push eax
                    mov ecx, OFFSET g_TargetGui49FBF0
                    call GuiOverlay::ShowBonusScore
                }
            }

            if (g_TargetCaptureEligible12FE0C4)
            {
                __asm
                {
                    mov ecx, DWORD PTR[g_TargetSpellId12FE0D8]
                    imul ecx, 120
                    add ecx, OFFSET g_TargetSpellRecords626288
                    mov DWORD PTR[record], ecx
                    mov edx, DWORD PTR[g_TargetSpellBaseScore12FE0CC]
                    add edx, DWORD PTR[g_TargetSpellBonusAccum12FE0D0]
                    mov DWORD PTR[value], edx
                }
                __asm
                {
                    mov eax, DWORD PTR[value]
                    push eax
                    mov ecx, OFFSET g_TargetGui49FBF0
                    call GuiOverlay::ShowSpellcardBonus
                }
                __asm
                {
                    mov ecx, DWORD PTR[g_TargetScoreState626278]
                    mov eax, DWORD PTR[value]
                    cdq
                    mov esi, 10
                    idiv esi
                    add eax, DWORD PTR[ecx + 4]
                    mov edx, DWORD PTR[g_TargetScoreState626278]
                    mov DWORD PTR[edx + 4], eax
                }

                __asm
                {
                    mov eax, DWORD PTR[g_TargetReplayFlags62F648]
                    shr eax, 3
                    and eax, 1
                    test eax, eax
                    jnz skip_record_update
                }
                {
                    checksum = 0;
                    i = strlen((const char *)record + 43);
                    while (i > 0)
                        checksum += ((i8 *)record)[43 + --i];
                    originalChecksum = checksum;
                    for (i = 0; i < 7; ++i)
                    {
                        checksum += U16_AT(record, 106 + 2 * i);
                        checksum += U16_AT(record, 92 + 2 * i);
                        checksum += I32_AT(record, 12 + 4 * i);
                    }
                    if (record[42] != (u8)checksum)
                    {
                        for (i = 0; i < 7; ++i)
                        {
                            U16_AT(record, 106 + 2 * i) = 0;
                            U16_AT(record, 92 + 2 * i) = 0;
                            I32_AT(record, 12 + 4 * i) = 0;
                        }
                    }
                    characterIndex = (u8)g_TargetCharacterIndex62F647;
                    if ((u32)I32_AT(record, 12 + 4 * characterIndex) < (u32)value)
                        I32_AT(record, 12 + 4 * characterIndex) = value;
                    if ((u32)I32_AT(record, 36) < (u32)value)
                        I32_AT(record, 36) = value;
                    if (U16_AT(record, 106 + 2 * characterIndex) < 9999)
                        ++U16_AT(record, 106 + 2 * characterIndex);
                    if (U16_AT(record, 118) < 9999)
                        ++U16_AT(record, 118);
                    for (i = 0; i < 7; ++i)
                    {
                        originalChecksum += U16_AT(record, 106 + 2 * i);
                        originalChecksum += U16_AT(record, 92 + 2 * i);
                        originalChecksum += I32_AT(record, 12 + 4 * i);
                    }
                    record[42] = (u8)originalChecksum;
                }
            skip_record_update:
                ++g_TargetScoreState626278->capturedSpellcards;
            }
        }

        g_TargetSpellActive12FE0C8 = 0;
        for (bossIndex = 0; bossIndex < 8; ++bossIndex)
        {
            if (g_TargetSpellBosses12FE098[bossIndex] &&
                I32_AT(g_TargetSpellBosses12FE098[bossIndex]->bytes, 0x2EB0))
            {
                U8_AT((u8 *)I32_AT(g_TargetSpellBosses12FE098[bossIndex]->bytes, 0x2EB0), 716) = 0;
                I32_AT(g_TargetSpellBosses12FE098[bossIndex]->bytes, 0x2EB0) = 0;
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
