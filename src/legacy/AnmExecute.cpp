#include "AnmManager.hpp"
#include "Rng.hpp"

#include <math.h>
#include <string.h>

#pragma intrinsic(fmod)

namespace th07
{
struct D3DXMATRIX
{
    float m[4][4];
};

D3DXMATRIX *__fastcall D3DXMatrixIdentity(D3DXMATRIX *matrix)
{
    matrix->m[3][2] = 0.0f;
    matrix->m[3][1] = 0.0f;
    matrix->m[3][0] = 0.0f;
    matrix->m[2][3] = 0.0f;
    matrix->m[2][1] = 0.0f;
    matrix->m[2][0] = 0.0f;
    matrix->m[1][3] = 0.0f;
    matrix->m[1][2] = 0.0f;
    matrix->m[1][0] = 0.0f;
    matrix->m[0][3] = 0.0f;
    matrix->m[0][2] = 0.0f;
    matrix->m[0][1] = 0.0f;
    matrix->m[3][3] = 1.0f;
    matrix->m[2][2] = 1.0f;
    matrix->m[1][1] = 1.0f;
    matrix->m[0][0] = 1.0f;
    return matrix;
}

// This translation unit deliberately keeps the target VM private.  The offsets
// below are observations from 0x00450D60, not a claim that this is the shared
// ANM layout: the common header remains coordinator-owned.
struct AnmVm
{
    u8 raw[0x24C];

    void Initialize();
    i32 GetIntVar(i32 value);
    float GetFloatVar(float value);
    i32 *GetIntVarPtr(i32 *value, u16 mask, u32 index);
    float *GetFloatVarPtr(float *value, u16 mask, u32 index);
};

struct AnmRawInstr
{
    i16 opcode;
    u16 instructionSize;
    i16 time;
    u16 varMask;
    union {
        i32 i[10];
        float f[10];
        u8 b[40];
    } args;
};

struct AnmTimer
{
    i32 previous;
    union {
        i32 subFrameBits;
        float subFrame;
    };
    i32 current;

    u32 TickImpl();
    void Decrement(i32 amount);
};

struct AnmTimerManager
{
    void Advance(i32 *current, i32 *subFrame);
};

struct AnmManagerSpriteOverlay
{
    void SetSprite(AnmVm *vm, i32 sprite);
};

struct AnmVmFlipOverlay
{
    u8 beforeFlip[0x1C0];
    u32 unknownBit0 : 1;
    u32 unknownLowFlags : 3;
    u32 unknownBit4 : 1;
    u32 unknownMidFlags : 2;
    u32 usePosOffset : 1;
    u32 flipFlags : 2;
    u32 unknownHighFlags : 2;
    u32 unknownBit12 : 1;
    u32 unknownBit13 : 1;
    u32 unknownBit14 : 1;
};
struct AnmVmIntPair
{
    i32 x;
    i32 y;
};
struct AnmVmVec3
{
    float x;
    float y;
    float z;

    AnmVmVec3(float x, float y, float z) : x(x), y(y), z(z) {}
};
struct AnmRngOverlay { u32 RandomU32(); float RandomF32(); };
extern AnmRngOverlay g_AnmRng;
extern AnmTimerManager g_AnmTimerManager;
extern float g_FrameMultiplier;

#define VM_I(vm, off) (*reinterpret_cast<i32 *>((vm)->raw + (off)))
#define VM_F(vm, off) (*reinterpret_cast<float *>((vm)->raw + (off)))
#define VM_S(vm, off) (*reinterpret_cast<i16 *>((vm)->raw + (off)))
#define VM_P(vm, off) (*reinterpret_cast<AnmRawInstr **>((vm)->raw + (off)))

#define GetInt(vm, instruction, index) \
    (((instruction)->varMask & (1 << (index))) ? (vm)->GetIntVar((instruction)->args.i[index]) : (instruction)->args.i[index])
#define GetFloat(vm, instruction, index) \
    (((instruction)->varMask & (1 << (index))) ? (vm)->GetFloatVar((instruction)->args.f[index]) : (instruction)->args.f[index])
#define GetIntPtr(vm, instruction, index) (vm)->GetIntVarPtr(&(instruction)->args.i[index], (instruction)->varMask, index)
#define GetFloatPtr(vm, instruction, index) (vm)->GetFloatVarPtr(&(instruction)->args.f[index], (instruction)->varMask, index)

static __forceinline void WrapUnit(float *value)
{
    if (*value >= 1.0f)
        *value -= 1.0f;
    else if (*value < 0.0f)
        *value += 1.0f;
}

static __forceinline AnmTimer *TimerAt(AnmVm *vm, i32 offset)
{
    return reinterpret_cast<AnmTimer *>(vm->raw + offset);
}

static __forceinline void ResetTimer(AnmTimer *timer, i32 current)
{
    timer->current = current;
    timer->subFrame = 0.0f;
    timer->previous = -999;
}

static __forceinline void AdvanceTimer(AnmTimer *timer)
{
    timer->previous = timer->current;
    g_AnmTimerManager.Advance(&timer->current, &timer->subFrameBits);
}

u32 AnmTimer::TickImpl()
{
    previous = current;
    g_AnmTimerManager.Advance(&current, &subFrameBits);
    return current;
}

void AnmVm::Initialize()
{
    AnmTimer *timer;

    memset(this, 0, 0x1C8);
    VM_F(this, 0x18) = 1.0f;
    VM_F(this, 0x1C) = 1.0f;
    VM_I(this, 0x1B8) = -1;
    D3DXMatrixIdentity(reinterpret_cast<D3DXMATRIX *>(raw + 0xF8));
    *reinterpret_cast<u16 *>(raw + 0x1C0) = 7;
    timer = TimerAt(this, 0x30);
    timer->current = 0;
    timer->previous = -999;
    timer->subFrame = 0.0f;
}

#pragma var_order(unknownLocal, timer)
void AnmManager::SetAndExecuteScript(AnmVm *vm, AnmRawInstr *script)
{
    AnmTimer *timer;
    // Target [ebp-4] is a reserved, never-read source slot.  Its semantic
    // name is not recovered; keeping the scalar is required for the observed
    // VC7 frame and is tracked as an unknown rather than invented behavior.
    i32 unknownLocal;

    if (script == 0)
    {
        memset(vm, 0, sizeof(*vm));
    }
    else
    {
        VM_I(vm, 0x1C0) &= ~0x300;
        vm->Initialize();
        VM_P(vm, 0x1DC) = script;
        VM_P(vm, 0x1E0) = VM_P(vm, 0x1DC);

        timer = TimerAt(vm, 0x30);
        timer->current = 0;
        timer->subFrame = 0.0f;
        timer->previous = -999;

        VM_I(vm, 0x1C0) &= ~1;
        ExecuteScript(vm);
        ++executedScriptCount;
    }
}

#pragma var_order(instruction, fallback, i, interp, currentScriptTime, spriteScriptTime, repeatCounter, currentTimer, endTimer, \
                 resetTimer, startTimer, scriptTimer, angleTimer, endTime, jumpTime)
i32 AnmManager::ExecuteScript(AnmVm *vm)
{
    AnmRawInstr *instruction;
    AnmRawInstr *fallback;
    i32 i;
    i32 currentScriptTime;
    i32 *repeatCounter;
    float interp;
    AnmTimer *currentTimer;
    AnmTimer *endTimer;
    AnmTimer *resetTimer;
    AnmTimer *startTimer;
    AnmTimer *scriptTimer;
    AnmTimer *angleTimer;
    i32 endTime;
    i32 jumpTime;

    if (VM_P(vm, 0x1e0) == 0)
        return 1;

    if (VM_S(vm, 0x1c6) != 0)
        goto handle_interrupt;

    while (instruction = VM_P(vm, 0x1e0), currentScriptTime = VM_I(vm, 0x38),
           instruction->time <= currentScriptTime)
    {
        switch (instruction->opcode)
        {
        case -1:
        case 1:
            VM_I(vm, 0x1c0) &= ~1;
        case 2:
            VM_P(vm, 0x1e0) = 0;
            return 1;
        case 3: {
            i32 spriteScriptTime;
            VM_I(vm, 0x1c0) |= 1;
            reinterpret_cast<AnmManagerSpriteOverlay *>(this)->SetSprite(
                vm,
                GetInt(vm, instruction, 0) +
                    reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 0x2B6F0)[VM_S(vm, 0x1D8)]);
            spriteScriptTime = VM_I(vm, 0x38);
            VM_I(vm, 0x23c) = spriteScriptTime;
            break;
        }
        case 7:
            VM_F(vm, 0x18) = GetFloat(vm, instruction, 0); VM_F(vm, 0x1c) = GetFloat(vm, instruction, 1); VM_I(vm, 0x1c0) |= 8;
            break;
        case 8: vm->raw[0x1bb] = (u8)(instruction->args.i[0] & 0xff); break;
        case 9: VM_I(vm, 0x1b8) = (VM_I(vm, 0x1b8) & 0xff000000) | (instruction->args.i[0] & 0x00ffffff); break;
        case 4:
            jumpTime = instruction->args.i[1];
            scriptTimer = TimerAt(vm, 0x30);
            scriptTimer->current = jumpTime;
            scriptTimer->subFrame = 0.0f;
            scriptTimer->previous = -999;
            VM_P(vm, 0x1e0) = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(VM_P(vm, 0x1dc)) + instruction->args.i[0]);
            continue;
        case 5:
            repeatCounter = GetIntPtr(vm, instruction, 0);
            --*repeatCounter;
            if (GetInt(vm, instruction, 0) > 0) {
                jumpTime = instruction->args.i[2];
                scriptTimer = TimerAt(vm, 0x30);
                scriptTimer->current = jumpTime;
                scriptTimer->subFrame = 0.0f;
                scriptTimer->previous = -999;
                VM_P(vm, 0x1e0) = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(VM_P(vm, 0x1dc)) + instruction->args.i[1]);
                continue;
            }
            break;
        case 10:
            reinterpret_cast<AnmVmFlipOverlay *>(vm)->flipFlags ^= 1;
            VM_F(vm, 0x18) *= -1.0f;
            VM_I(vm, 0x1c0) |= 8;
            break;
        case 24:
            reinterpret_cast<AnmVmFlipOverlay *>(vm)->usePosOffset = instruction->args.i[0];
            break;
        case 11:
            reinterpret_cast<AnmVmFlipOverlay *>(vm)->flipFlags ^= 2;
            VM_F(vm, 0x1c) *= -1.0f;
            VM_I(vm, 0x1c0) |= 8;
            break;
        case 12:
            VM_F(vm, 0) = GetFloat(vm, instruction, 0);
            VM_F(vm, 4) = GetFloat(vm, instruction, 1);
            VM_F(vm, 8) = GetFloat(vm, instruction, 2);
            VM_I(vm, 0x1c0) |= 4;
            break;
        case 13:
            VM_F(vm, 12) = GetFloat(vm, instruction, 0); VM_F(vm, 16) = GetFloat(vm, instruction, 1); VM_F(vm, 20) = GetFloat(vm, instruction, 2); VM_I(vm, 0x1c0) |= 4;
            break;
        case 14: VM_F(vm, 32) = GetFloat(vm, instruction, 0); VM_F(vm, 36) = GetFloat(vm, instruction, 1); break;
        case 29:
            angleTimer = TimerAt(vm, 0x78);
            angleTimer->current = 0;
            angleTimer->subFrame = 0.0f;
            angleTimer->previous = -999;
            (endTimer = TimerAt(vm, 0xb4))->current = GetInt(vm, instruction, 2);
            endTimer->subFrame = 0.0f;
            endTimer->previous = -999;
            vm->raw[0xc4] = 0;
            *reinterpret_cast<AnmVmIntPair *>(vm->raw + 0x218) = *reinterpret_cast<AnmVmIntPair *>(vm->raw + 0x18);
            VM_F(vm, 0x220) = GetFloat(vm, instruction, 0); VM_F(vm, 0x224) = GetFloat(vm, instruction, 1); break;
        case 15:
            vm->raw[0x22b] = vm->raw[0x1bb];
            vm->raw[0x22f] = instruction->args.b[0];
            currentTimer = TimerAt(vm, 0x60);
            currentTimer->current = 0;
            currentTimer->subFrame = 0.0f;
            currentTimer->previous = -999;
            (endTimer = TimerAt(vm, 0x9c))->current = GetInt(vm, instruction, 1);
            endTimer->subFrame = 0.0f;
            endTimer->previous = -999;
            vm->raw[0xc2] = 0;
            break;
        case 16: reinterpret_cast<AnmVmFlipOverlay *>(vm)->unknownBit4 = instruction->args.i[0]; break;
        case 6:
            if (!reinterpret_cast<AnmVmFlipOverlay *>(vm)->usePosOffset) {
                *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x1c8) =
                    AnmVmVec3(GetFloat(vm, instruction, 0), GetFloat(vm, instruction, 1), GetFloat(vm, instruction, 2));
            } else {
                *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x230) =
                    AnmVmVec3(GetFloat(vm, instruction, 0), GetFloat(vm, instruction, 1), GetFloat(vm, instruction, 2));
            }
            break;
        case 19: vm->raw[0xc0] = 6; goto pos_time;
        case 18: vm->raw[0xc0] = 4; goto pos_time;
        case 17: vm->raw[0xc0] = 0;
        pos_time:
            if (!reinterpret_cast<AnmVmFlipOverlay *>(vm)->usePosOffset)
                *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x1e8) = *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x1c8);
            else
                *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x1e8) = *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x230);
            *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x1f4) =
                AnmVmVec3(GetFloat(vm, instruction, 0), GetFloat(vm, instruction, 1), GetFloat(vm, instruction, 2));
            (endTimer = TimerAt(vm, 0x84))->current = GetInt(vm, instruction, 3);
            endTimer->subFrame = 0.0f;
            endTimer->previous = -999;
            currentTimer = TimerAt(vm, 0x48);
            currentTimer->current = 0;
            currentTimer->subFrame = 0.0f;
            currentTimer->previous = -999;
            break;
        case 79:
            if (TimerAt(vm, 0x3c)->current == 0 ? 1 : 0)
            {
                (currentTimer = TimerAt(vm, 0x3c))->current = GetInt(vm, instruction, 0);
                currentTimer->subFrame = 0.0f;
                currentTimer->previous = -999;
            }
            else
                TimerAt(vm, 0x3c)->Decrement(1);
            if (TimerAt(vm, 0x3c)->current <= 0 ? 1 : 0) {
                currentTimer = TimerAt(vm, 0x3c);
                currentTimer->current = 0;
                currentTimer->subFrame = 0.0f;
                currentTimer->previous = -999;
                break;
            }
            TimerAt(vm, 0x30)->Decrement(1);
            goto advance;
        case 23:
            VM_I(vm, 0x1c0) &= ~1;
        case 20:
            if (VM_S(vm, 0x1c6) == 0) { VM_I(vm, 0x1c0) |= 0x2000; TimerAt(vm, 0x30)->Decrement(1); goto advance; }
        handle_interrupt:
            fallback = 0; instruction = VM_P(vm, 0x1dc);
            while (!((instruction->opcode == 21) && VM_S(vm, 0x1c6) == instruction->args.i[0]) && instruction->opcode != -1) {
                if (instruction->opcode == 21 && instruction->args.i[0] == -1) fallback = instruction;
                instruction = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(instruction) + instruction->instructionSize);
            }
            VM_S(vm, 0x1c6) = 0; VM_I(vm, 0x1c0) &= ~0x2000;
            if (instruction->opcode != 21) { if (!fallback) { TimerAt(vm, 0x30)->Decrement(1); goto advance; } instruction = fallback; }
            instruction = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(instruction) + instruction->instructionSize);
            VM_P(vm, 0x1e0) = instruction;
            jumpTime = VM_P(vm, 0x1e0)->time;
            currentTimer = TimerAt(vm, 0x30);
            currentTimer->current = jumpTime;
            currentTimer->subFrame = 0.0f;
            currentTimer->previous = -999;
            VM_I(vm, 0x1c0) |= 1;
            continue;
        case 28: reinterpret_cast<AnmVmFlipOverlay *>(vm)->unknownBit0 = instruction->args.i[0]; break;
        case 22: VM_I(vm, 0x1c0) |= 0xc00; break;
        case 25: VM_S(vm, 0x1c4) = (i16)instruction->args.i[0]; break;
        case 26:
            VM_F(vm, 40) += GetFloat(vm, instruction, 0);
            if (VM_F(vm, 40) >= 1.0f)
                VM_F(vm, 40) -= 1.0f;
            else if (VM_F(vm, 40) < 0.0f)
                VM_F(vm, 40) += 1.0f;
            break;
        case 27:
            VM_F(vm, 44) += GetFloat(vm, instruction, 0);
            if (VM_F(vm, 44) >= 1.0f)
                VM_F(vm, 44) -= 1.0f;
            else if (VM_F(vm, 44) < 0.0f)
                VM_F(vm, 44) += 1.0f;
            break;
        case 80: VM_F(vm, 0xf0) = GetFloat(vm, instruction, 0); break;
        case 81: VM_F(vm, 0xf4) = GetFloat(vm, instruction, 0); break;
        case 30: reinterpret_cast<AnmVmFlipOverlay *>(vm)->unknownBit12 = instruction->args.i[0]; break;
        case 31: reinterpret_cast<AnmVmFlipOverlay *>(vm)->unknownBit14 = instruction->args.i[0]; break;
        case 32:
            currentTimer = TimerAt(vm, 0x48);
            currentTimer->current = 0;
            currentTimer->subFrame = 0.0f;
            currentTimer->previous = -999;
            (endTimer = TimerAt(vm, 0x84))->current = GetInt(vm, instruction, 0);
            endTimer->subFrame = 0.0f;
            endTimer->previous = -999;
            vm->raw[0xc0] = instruction->args.b[4];
            if (!reinterpret_cast<AnmVmFlipOverlay *>(vm)->usePosOffset)
                *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x1e8) = *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x1c8);
            else
                *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x1e8) = *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x230);
            VM_F(vm, 0x1f4) = GetFloat(vm, instruction, 2);
            VM_F(vm, 0x1f8) = GetFloat(vm, instruction, 3);
            VM_F(vm, 0x1fc) = GetFloat(vm, instruction, 4);
            break;
        case 33:
            currentTimer = TimerAt(vm, 0x54);
            currentTimer->current = 0;
            currentTimer->subFrame = 0.0f;
            currentTimer->previous = -999;
            (endTimer = TimerAt(vm, 0x90))->current = GetInt(vm, instruction, 0);
            endTimer->subFrame = 0.0f;
            endTimer->previous = -999;
            vm->raw[0xc1] = instruction->args.b[4];
            vm->raw[0x22a] = vm->raw[0x1ba];
            vm->raw[0x229] = vm->raw[0x1b9];
            vm->raw[0x228] = vm->raw[0x1b8];
            vm->raw[0x22e] = instruction->args.b[10];
            vm->raw[0x22d] = instruction->args.b[9];
            vm->raw[0x22c] = instruction->args.b[8];
            break;
        case 34:
            currentTimer = TimerAt(vm, 0x60);
            currentTimer->current = 0;
            currentTimer->subFrame = 0.0f;
            currentTimer->previous = -999;
            (endTimer = TimerAt(vm, 0x9c))->current = GetInt(vm, instruction, 0);
            endTimer->subFrame = 0.0f;
            endTimer->previous = -999;
            vm->raw[0xc2] = instruction->args.b[4];
            vm->raw[0x22b] = vm->raw[0x1bb];
            vm->raw[0x22f] = instruction->args.b[8];
            break;
        case 35:
            currentTimer = TimerAt(vm, 0x6c);
            currentTimer->current = 0;
            currentTimer->subFrame = 0.0f;
            currentTimer->previous = -999;
            (endTimer = TimerAt(vm, 0xa8))->current = GetInt(vm, instruction, 0);
            endTimer->subFrame = 0.0f;
            endTimer->previous = -999;
            vm->raw[0xc3] = instruction->args.b[4];
            *reinterpret_cast<AnmVmVec3 *>(vm->raw + 0x200) = *reinterpret_cast<AnmVmVec3 *>(vm->raw);
            VM_F(vm, 0x20c) = GetFloat(vm, instruction, 2);
            VM_F(vm, 0x210) = GetFloat(vm, instruction, 3);
            VM_F(vm, 0x214) = GetFloat(vm, instruction, 4);
            VM_I(vm, 0x1c0) |= 4;
            break;
        case 36:
            currentTimer = TimerAt(vm, 0x78);
            currentTimer->current = 0;
            currentTimer->subFrame = 0.0f;
            currentTimer->previous = -999;
            (endTimer = TimerAt(vm, 0xb4))->current = GetInt(vm, instruction, 0);
            endTimer->subFrame = 0.0f;
            endTimer->previous = -999;
            vm->raw[0xc4] = instruction->args.b[4];
            *reinterpret_cast<AnmVmIntPair *>(vm->raw + 0x218) = *reinterpret_cast<AnmVmIntPair *>(vm->raw + 0x18);
            VM_F(vm, 0x220) = GetFloat(vm, instruction, 2);
            VM_F(vm, 0x224) = GetFloat(vm, instruction, 3);
            VM_I(vm, 0x1c0) |= 8;
            break;
        case 37: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1); break;
        case 38: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1); break;
        case 49: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) + GetInt(vm, instruction, 2); break;
        case 50: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1) + GetFloat(vm, instruction, 2); break;
        case 51: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) - GetInt(vm, instruction, 2); break;
        case 52: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1) - GetFloat(vm, instruction, 2); break;
        case 53: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) * GetInt(vm, instruction, 2); break;
        case 54: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1) * GetFloat(vm, instruction, 2); break;
        case 55: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) / GetInt(vm, instruction, 2); break;
        case 56: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1) / GetFloat(vm, instruction, 2); break;
        case 57: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) % GetInt(vm, instruction, 2); break;
        case 58:
        {
            float fmodResult = fmod(GetFloat(vm, instruction, 1), GetFloat(vm, instruction, 2));
            *GetFloatPtr(vm, instruction, 0) = fmodResult;
            break;
        }
        case 39: *GetIntPtr(vm, instruction, 0) += GetInt(vm, instruction, 1); break;
        case 40: *GetFloatPtr(vm, instruction, 0) += GetFloat(vm, instruction, 1); break;
        case 41: *GetIntPtr(vm, instruction, 0) -= GetInt(vm, instruction, 1); break;
        case 42: *GetFloatPtr(vm, instruction, 0) -= GetFloat(vm, instruction, 1); break;
        case 43: *GetIntPtr(vm, instruction, 0) *= GetInt(vm, instruction, 1); break;
        case 44: *GetFloatPtr(vm, instruction, 0) *= GetFloat(vm, instruction, 1); break;
        case 45: *GetIntPtr(vm, instruction, 0) /= GetInt(vm, instruction, 1); break;
        case 46: *GetFloatPtr(vm, instruction, 0) /= GetFloat(vm, instruction, 1); break;
        case 47: *GetIntPtr(vm, instruction, 0) %= GetInt(vm, instruction, 1); break;
        case 48:
        {
            float fmodResult = (float)fmod(GetFloat(vm, instruction, 0), GetFloat(vm, instruction, 1));
            *GetFloatPtr(vm, instruction, 0) = fmodResult;
            break;
        }
        case 59: { i32 range = GetInt(vm, instruction, 1); *GetIntPtr(vm, instruction, 0) = range ? g_AnmRng.RandomU32() % range : 0; break; }
        case 60: *GetFloatPtr(vm, instruction, 0) = g_AnmRng.RandomF32() * GetFloat(vm, instruction, 1); break;
        case 61: *GetFloatPtr(vm, instruction, 0) = (float)sin(GetFloat(vm, instruction, 1)); break;
        case 62: *GetFloatPtr(vm, instruction, 0) = (float)cos(GetFloat(vm, instruction, 1)); break;
        case 63: *GetFloatPtr(vm, instruction, 0) = (float)tan(GetFloat(vm, instruction, 1)); break;
        case 64: *GetFloatPtr(vm, instruction, 0) = (float)acos(GetFloat(vm, instruction, 1)); break;
        case 65: *GetFloatPtr(vm, instruction, 0) = (float)atan(GetFloat(vm, instruction, 1)); break;
        case 66: *GetFloatPtr(vm, instruction, 0) = AddNormalizeAngle(GetFloat(vm, instruction, 0), 0.0f); break;
        case 67: if (GetInt(vm, instruction, 0) == GetInt(vm, instruction, 1)) goto conditional_jump; break;
        case 68: if (GetFloat(vm, instruction, 0) == GetFloat(vm, instruction, 1)) goto conditional_jump; break;
        case 69: if (GetInt(vm, instruction, 0) != GetInt(vm, instruction, 1)) goto conditional_jump; break;
        case 70: if (GetFloat(vm, instruction, 0) != GetFloat(vm, instruction, 1)) goto conditional_jump; break;
        case 71: if (GetInt(vm, instruction, 0) < GetInt(vm, instruction, 1)) goto conditional_jump; break;
        case 72: if (GetFloat(vm, instruction, 0) < GetFloat(vm, instruction, 1)) goto conditional_jump; break;
        case 73: if (GetInt(vm, instruction, 0) <= GetInt(vm, instruction, 1)) goto conditional_jump; break;
        case 74: if (GetFloat(vm, instruction, 0) <= GetFloat(vm, instruction, 1)) goto conditional_jump; break;
        case 75: if (GetInt(vm, instruction, 0) > GetInt(vm, instruction, 1)) goto conditional_jump; break;
        case 76: if (GetFloat(vm, instruction, 0) > GetFloat(vm, instruction, 1)) goto conditional_jump; break;
        case 77: if (GetInt(vm, instruction, 0) >= GetInt(vm, instruction, 1)) goto conditional_jump; break;
        case 78: if (GetFloat(vm, instruction, 0) >= GetFloat(vm, instruction, 1)) goto conditional_jump; break;
        conditional_jump:
            VM_I(vm, 0x38) = instruction->args.i[3]; VM_I(vm, 0x34) = 0; VM_I(vm, 0x30) = -999; VM_P(vm, 0x1e0) = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(VM_P(vm, 0x1dc)) + instruction->args.i[2]);
            continue;
        default: break;
        }
        VM_P(vm, 0x1e0) = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(instruction) + instruction->instructionSize);
    }

advance:
    if (VM_F(vm, 12) != 0.0f) { VM_F(vm, 0) = AddNormalizeAngle(VM_F(vm, 0), g_FrameMultiplier * VM_F(vm, 12)); VM_I(vm, 0x1c0) |= 4; }
    if (VM_F(vm, 16) != 0.0f) { VM_F(vm, 4) = AddNormalizeAngle(VM_F(vm, 4), g_FrameMultiplier * VM_F(vm, 16)); VM_I(vm, 0x1c0) |= 4; }
    if (VM_F(vm, 20) != 0.0f) { VM_F(vm, 8) = AddNormalizeAngle(VM_F(vm, 8), g_FrameMultiplier * VM_F(vm, 20)); VM_I(vm, 0x1c0) |= 4; }

    for (i = 0; i < 5; ++i) {
        if (TimerAt(vm, 0x84 + i * 12)->current > 0 ? 1 : 0) {
            currentTimer = TimerAt(vm, 0x48 + i * 12);
            AdvanceTimer(currentTimer);
            endTime = TimerAt(vm, 0x84 + i * 12)->current;
            if (currentTimer->current >= endTime) {
                interp = 1.0f;
                resetTimer = TimerAt(vm, 0x84 + i * 12);
                ResetTimer(resetTimer, 0);
            } else {
                startTimer = TimerAt(vm, 0x48 + i * 12);
                endTimer = TimerAt(vm, 0x84 + i * 12);
                interp = ((float)startTimer->current + startTimer->subFrame) /
                         ((float)endTimer->current + endTimer->subFrame);
            }
            switch (vm->raw[0xc0 + i]) { case 1: interp *= interp; break; case 2: interp = interp * interp * interp; break; case 3: interp = interp * interp * interp; break; case 4: interp = 1.0f - interp; interp *= interp; interp = 1.0f - interp; break; case 5: interp = 1.0f - interp; interp = interp * interp * interp; interp = 1.0f - interp; break; case 6: interp = 1.0f - interp; interp = interp * interp * interp; interp = 1.0f - interp; break; }
            switch (i) {
            case 0: {
                if (!((VM_I(vm, 0x1c0) >> 7) & 1)) {
                    VM_F(vm, 0x1c8) = interp * (VM_F(vm, 0x1f4) - VM_F(vm, 0x1e8)) + VM_F(vm, 0x1e8);
                    VM_F(vm, 0x1cc) = interp * (VM_F(vm, 0x1f8) - VM_F(vm, 0x1ec)) + VM_F(vm, 0x1ec);
                    VM_F(vm, 0x1d0) = interp * (VM_F(vm, 0x1fc) - VM_F(vm, 0x1f0)) + VM_F(vm, 0x1f0);
                } else {
                    VM_F(vm, 0x230) = interp * (VM_F(vm, 0x1f4) - VM_F(vm, 0x1e8)) + VM_F(vm, 0x1e8);
                    VM_F(vm, 0x234) = interp * (VM_F(vm, 0x1f8) - VM_F(vm, 0x1ec)) + VM_F(vm, 0x1ec);
                    VM_F(vm, 0x238) = interp * (VM_F(vm, 0x1fc) - VM_F(vm, 0x1f0)) + VM_F(vm, 0x1f0);
                }
                break;
            }
            case 1:
            {
                vm->raw[0x1ba] = (u8)(interp * (vm->raw[0x22e] - vm->raw[0x22a]) + vm->raw[0x22a]);
                vm->raw[0x1b9] = (u8)(interp * (vm->raw[0x22d] - vm->raw[0x229]) + vm->raw[0x229]);
                vm->raw[0x1b8] = (u8)(interp * (vm->raw[0x22c] - vm->raw[0x228]) + vm->raw[0x228]);
                break;
            }
            case 2:
            {
                vm->raw[0x1bb] = (u8)(interp * (vm->raw[0x22f] - vm->raw[0x22b]) + vm->raw[0x22b]);
                break;
            }
            case 3:
                VM_F(vm, 0) = AddNormalizeAngle((VM_F(vm, 0x20c) - VM_F(vm, 0x200)) * interp, VM_F(vm, 0x200));
                VM_F(vm, 4) = AddNormalizeAngle((VM_F(vm, 0x210) - VM_F(vm, 0x204)) * interp, VM_F(vm, 0x204));
                VM_F(vm, 8) = AddNormalizeAngle((VM_F(vm, 0x214) - VM_F(vm, 0x208)) * interp, VM_F(vm, 0x208));
                VM_I(vm, 0x1c0) |= 4;
                break;
            case 4:
                VM_F(vm, 0x18) = interp * (VM_F(vm, 0x220) - VM_F(vm, 0x218)) + VM_F(vm, 0x218);
                VM_F(vm, 0x1c) = interp * (VM_F(vm, 0x224) - VM_F(vm, 0x21c)) + VM_F(vm, 0x21c);
                VM_I(vm, 0x1c0) |= 8;
                break;
            }
        }
    }
    if (VM_F(vm, 36) != 0.0f) { VM_F(vm, 28) += g_FrameMultiplier * VM_F(vm, 36); VM_I(vm, 0x1c0) |= 8; }
    if (VM_F(vm, 32) != 0.0f) { VM_F(vm, 24) += g_FrameMultiplier * VM_F(vm, 32); VM_I(vm, 0x1c0) |= 8; VM_I(vm, 0x1c0) |= 4; }
    VM_F(vm, 40) += VM_F(vm, 0xf0);
    if (VM_F(vm, 40) >= 1.0f)
        VM_F(vm, 40) -= 1.0f;
    else if (VM_F(vm, 40) < 0.0f)
        VM_F(vm, 40) += 1.0f;
    VM_F(vm, 44) += VM_F(vm, 0xf4);
    if (VM_F(vm, 44) >= 1.0f)
        VM_F(vm, 44) -= 1.0f;
    else if (VM_F(vm, 44) < 0.0f)
        VM_F(vm, 44) += 1.0f;
    AdvanceTimer(TimerAt(vm, 0x30));
    ++*reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + 12);
    return 0;
}

#undef VM_P
#undef VM_S
#undef VM_F
#undef VM_I
#undef GetFloatPtr
#undef GetIntPtr
#undef GetFloat
#undef GetInt

} // namespace th07
