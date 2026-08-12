#include "AnmManager.hpp"
#include "Rng.hpp"

#include <math.h>

namespace th07
{
// This translation unit deliberately keeps the target VM private.  The offsets
// below are observations from 0x00450D60, not a claim that this is the shared
// ANM layout: the common header remains coordinator-owned.
struct AnmVm
{
    u8 raw[0x240];

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

    void Decrement(i32 amount);
};

struct AnmTimerManager
{
    void Advance(i32 *current, i32 *subFrame);
};

extern void AnmManagerSetSprite(AnmManager *manager, AnmVm *vm, i32 sprite);
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
    timer->previous = -999;
    timer->subFrame = 0.0f;
    timer->current = current;
}

static __forceinline void AdvanceTimer(AnmTimer *timer)
{
    timer->previous = timer->current;
    g_AnmTimerManager.Advance(&timer->current, &timer->subFrameBits);
}

#pragma var_order(instruction, fallback, i, interp)
i32 AnmManager::ExecuteScript(AnmVm *vm)
{
    AnmRawInstr *instruction;
    AnmRawInstr *fallback;
    i32 i;
    float interp;

    if (VM_P(vm, 0x1e0) == 0)
        return 1;

    if (VM_S(vm, 0x1c6) != 0)
        goto handle_interrupt;

    while ((instruction = VM_P(vm, 0x1e0))->time <= VM_I(vm, 0x38))
    {
        switch (instruction->opcode)
        {
        case -1:
        case 1:
            VM_I(vm, 0x1c0) &= ~1;
        case 2:
            VM_P(vm, 0x1e0) = 0;
            return 1;
        case 3:
            VM_I(vm, 0x1c0) |= 1;
            AnmManagerSetSprite(this, vm, GetInt(vm, instruction, 0));
            VM_I(vm, 0x23c) = VM_I(vm, 0x38);
            break;
        case 4:
            VM_I(vm, 0x38) = instruction->args.i[1];
            VM_I(vm, 0x34) = 0;
            VM_I(vm, 0x30) = -999;
            VM_P(vm, 0x1e0) = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(VM_P(vm, 0x1dc)) + instruction->args.i[0]);
            continue;
        case 5:
            --*GetIntPtr(vm, instruction, 0);
            if (GetInt(vm, instruction, 0) > 0) {
                VM_I(vm, 0x38) = instruction->args.i[2];
                VM_I(vm, 0x34) = 0;
                VM_I(vm, 0x30) = -999;
                VM_P(vm, 0x1e0) = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(VM_P(vm, 0x1dc)) + instruction->args.i[1]);
                continue;
            }
            break;
        case 6:
            if (VM_I(vm, 0x1c0) & 0x80) {
                VM_F(vm, 0x230) = GetFloat(vm, instruction, 0); VM_F(vm, 0x234) = GetFloat(vm, instruction, 1); VM_F(vm, 0x238) = GetFloat(vm, instruction, 2);
            } else {
                VM_F(vm, 0x1c8) = GetFloat(vm, instruction, 0); VM_F(vm, 0x1cc) = GetFloat(vm, instruction, 1); VM_F(vm, 0x1d0) = GetFloat(vm, instruction, 2);
            }
            break;
        case 7:
            VM_F(vm, 0x18) = GetFloat(vm, instruction, 0); VM_F(vm, 0x1c) = GetFloat(vm, instruction, 1); VM_I(vm, 0x1c0) |= 8;
            break;
        case 8: VM_F(vm, 0x1bb) = (float)instruction->args.i[0]; break;
        case 9: VM_I(vm, 0x1b8) = (VM_I(vm, 0x1b8) & 0xff000000) | (instruction->args.i[0] & 0x00ffffff); break;
        case 10: VM_I(vm, 0x1c0) ^= 0x100; VM_F(vm, 0x18) = -VM_F(vm, 0x18); VM_I(vm, 0x1c0) |= 8; break;
        case 11: VM_I(vm, 0x1c0) ^= 0x200; VM_F(vm, 0x1c) = -VM_F(vm, 0x1c); VM_I(vm, 0x1c0) |= 8; break;
        case 12:
            VM_F(vm, 0) = GetFloat(vm, instruction, 0); VM_F(vm, 4) = GetFloat(vm, instruction, 1); VM_F(vm, 8) = GetFloat(vm, instruction, 2); VM_I(vm, 0x1c0) |= 4;
            break;
        case 13:
            VM_F(vm, 12) = GetFloat(vm, instruction, 0); VM_F(vm, 16) = GetFloat(vm, instruction, 1); VM_F(vm, 20) = GetFloat(vm, instruction, 2); VM_I(vm, 0x1c0) |= 4;
            break;
        case 14: VM_F(vm, 32) = GetFloat(vm, instruction, 0); VM_F(vm, 36) = GetFloat(vm, instruction, 1); break;
        case 15:
            VM_I(vm, 0x60) = -999; VM_I(vm, 0x64) = 0; VM_I(vm, 0x68) = 0;
            VM_I(vm, 0x9c) = -999; VM_I(vm, 0xa0) = 0; VM_I(vm, 0xa4) = GetInt(vm, instruction, 1);
            VM_I(vm, 0xc2) = 0; VM_F(vm, 0x22b) = VM_F(vm, 0x1bb); VM_F(vm, 0x22f) = GetFloat(vm, instruction, 0);
            break;
        case 16: VM_I(vm, 0x1c0) = (VM_I(vm, 0x1c0) & ~0x10) | ((instruction->args.i[0] & 1) << 4); break;
        case 17: VM_I(vm, 0xc0) = 0; goto pos_time;
        case 18: VM_I(vm, 0xc0) = 4; goto pos_time;
        case 19: VM_I(vm, 0xc0) = 6;
        pos_time:
            VM_F(vm, 0x1e8) = (VM_I(vm, 0x1c0) & 0x80) ? VM_F(vm, 0x230) : VM_F(vm, 0x1c8);
            VM_F(vm, 0x1ec) = (VM_I(vm, 0x1c0) & 0x80) ? VM_F(vm, 0x234) : VM_F(vm, 0x1cc);
            VM_F(vm, 0x1f0) = (VM_I(vm, 0x1c0) & 0x80) ? VM_F(vm, 0x238) : VM_F(vm, 0x1d0);
            VM_F(vm, 0x1f4) = GetFloat(vm, instruction, 0); VM_F(vm, 0x1f8) = GetFloat(vm, instruction, 1); VM_F(vm, 0x1fc) = GetFloat(vm, instruction, 2);
            VM_I(vm, 0x84) = -999; VM_I(vm, 0x88) = 0; VM_I(vm, 0x8c) = GetInt(vm, instruction, 3); break;
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
            VM_P(vm, 0x1e0) = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(instruction) + instruction->instructionSize);
            VM_I(vm, 0x38) = VM_P(vm, 0x1e0)->time; VM_I(vm, 0x34) = 0; VM_I(vm, 0x30) = -999; VM_I(vm, 0x1c0) |= 1;
            continue;
        case 22: VM_I(vm, 0x1c0) |= 0xc00; break;
        case 23:
            VM_I(vm, 0x1c0) &= ~1;
            if (VM_S(vm, 0x1c6) == 0) { VM_I(vm, 0x1c0) |= 0x2000; TimerAt(vm, 0x30)->Decrement(1); goto advance; }
            goto handle_interrupt;
        case 24: VM_I(vm, 0x1c0) = (VM_I(vm, 0x1c0) & ~0x80) | ((instruction->args.i[0] & 1) << 7); break;
        case 25: VM_S(vm, 0x1c4) = (i16)instruction->args.i[0]; break;
        case 26: VM_F(vm, 40) += GetFloat(vm, instruction, 0); WrapUnit(&VM_F(vm, 40)); break;
        case 27: VM_F(vm, 44) += GetFloat(vm, instruction, 0); WrapUnit(&VM_F(vm, 44)); break;
        case 28: VM_I(vm, 0x1c0) = (VM_I(vm, 0x1c0) & ~1) | (instruction->args.i[0] & 1); break;
        case 29:
            VM_I(vm, 0x78) = -999; VM_I(vm, 0x7c) = 0; VM_I(vm, 0x80) = 0;
            VM_I(vm, 0xb4) = -999; VM_I(vm, 0xb8) = 0; VM_I(vm, 0xbc) = GetInt(vm, instruction, 2); VM_I(vm, 0xc4) = 0;
            VM_F(vm, 0x218) = VM_F(vm, 0x18); VM_F(vm, 0x21c) = VM_F(vm, 0x1c); VM_F(vm, 0x220) = GetFloat(vm, instruction, 0); VM_F(vm, 0x224) = GetFloat(vm, instruction, 1); break;
        case 30: VM_I(vm, 0x1c0) = (VM_I(vm, 0x1c0) & ~0x1000) | ((instruction->args.i[0] & 1) << 12); break;
        case 31: VM_I(vm, 0x1c0) = (VM_I(vm, 0x1c0) & ~0x4000) | ((instruction->args.i[0] & 1) << 14); break;
        case 32:
            VM_I(vm, 0x78) = -999; VM_I(vm, 0x7c) = 0; VM_I(vm, 0x80) = 0;
            VM_I(vm, 0xb4) = -999; VM_I(vm, 0xb8) = 0; VM_I(vm, 0xbc) = GetInt(vm, instruction, 0); VM_I(vm, 0xc0) = instruction->args.i[1];
            VM_F(vm, 0x1e8) = (VM_I(vm, 0x1c0) & 0x80) ? VM_F(vm, 0x230) : VM_F(vm, 0x1c8); VM_F(vm, 0x1ec) = (VM_I(vm, 0x1c0) & 0x80) ? VM_F(vm, 0x234) : VM_F(vm, 0x1cc); VM_F(vm, 0x1f0) = (VM_I(vm, 0x1c0) & 0x80) ? VM_F(vm, 0x238) : VM_F(vm, 0x1d0);
            VM_F(vm, 0x1f4) = GetFloat(vm, instruction, 2); VM_F(vm, 0x1f8) = GetFloat(vm, instruction, 3); VM_F(vm, 0x1fc) = GetFloat(vm, instruction, 4); break;
        case 33:
            VM_I(vm, 0x54) = -999; VM_I(vm, 0x58) = 0; VM_I(vm, 0x5c) = 0;
            VM_I(vm, 0x90) = -999; VM_I(vm, 0x94) = 0; VM_I(vm, 0x98) = GetInt(vm, instruction, 0); vm->raw[0xc1] = instruction->args.b[4];
            vm->raw[0x228] = vm->raw[0x1b8]; vm->raw[0x229] = vm->raw[0x1b9]; vm->raw[0x22a] = vm->raw[0x1ba];
            vm->raw[0x22c] = instruction->args.b[8]; vm->raw[0x22d] = instruction->args.b[12]; vm->raw[0x22e] = instruction->args.b[16]; break;
        case 34:
            VM_I(vm, 0x60) = -999; VM_I(vm, 0x64) = 0; VM_I(vm, 0x68) = 0;
            VM_I(vm, 0x9c) = -999; VM_I(vm, 0xa0) = 0; VM_I(vm, 0xa4) = GetInt(vm, instruction, 0); vm->raw[0xc2] = instruction->args.b[4];
            vm->raw[0x22b] = vm->raw[0x1bb]; vm->raw[0x22f] = instruction->args.b[8]; break;
        case 35:
            VM_I(vm, 0x6c) = -999; VM_I(vm, 0x70) = 0; VM_I(vm, 0x74) = 0;
            VM_I(vm, 0xa8) = -999; VM_I(vm, 0xac) = 0; VM_I(vm, 0xb0) = GetInt(vm, instruction, 0); vm->raw[0xc3] = instruction->args.b[4];
            VM_F(vm, 0x200) = VM_F(vm, 0); VM_F(vm, 0x204) = VM_F(vm, 4); VM_F(vm, 0x208) = VM_F(vm, 8);
            VM_F(vm, 0x20c) = GetFloat(vm, instruction, 2); VM_F(vm, 0x210) = GetFloat(vm, instruction, 3); VM_F(vm, 0x214) = GetFloat(vm, instruction, 4); VM_I(vm, 0x1c0) |= 4; break;
        case 36:
            VM_I(vm, 0x78) = -999; VM_I(vm, 0x7c) = 0; VM_I(vm, 0x80) = 0;
            VM_I(vm, 0xb4) = -999; VM_I(vm, 0xb8) = 0; VM_I(vm, 0xbc) = GetInt(vm, instruction, 0); vm->raw[0xc4] = instruction->args.b[4];
            VM_F(vm, 0x218) = VM_F(vm, 0x18); VM_F(vm, 0x21c) = VM_F(vm, 0x1c); VM_F(vm, 0x220) = GetFloat(vm, instruction, 2); VM_F(vm, 0x224) = GetFloat(vm, instruction, 3); VM_I(vm, 0x1c0) |= 8; break;
        case 37: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1); break;
        case 38: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1); break;
        case 39: *GetIntPtr(vm, instruction, 0) += GetInt(vm, instruction, 1); break;
        case 40: *GetFloatPtr(vm, instruction, 0) += GetFloat(vm, instruction, 1); break;
        case 41: *GetIntPtr(vm, instruction, 0) -= GetInt(vm, instruction, 1); break;
        case 42: *GetFloatPtr(vm, instruction, 0) -= GetFloat(vm, instruction, 1); break;
        case 43: *GetIntPtr(vm, instruction, 0) *= GetInt(vm, instruction, 1); break;
        case 44: *GetFloatPtr(vm, instruction, 0) *= GetFloat(vm, instruction, 1); break;
        case 45: *GetIntPtr(vm, instruction, 0) /= GetInt(vm, instruction, 1); break;
        case 46: *GetFloatPtr(vm, instruction, 0) /= GetFloat(vm, instruction, 1); break;
        case 47: *GetIntPtr(vm, instruction, 0) %= GetInt(vm, instruction, 1); break;
        case 48: *GetFloatPtr(vm, instruction, 0) = (float)fmod(GetFloat(vm, instruction, 0), GetFloat(vm, instruction, 1)); break;
        case 49: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) + GetInt(vm, instruction, 2); break;
        case 50: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1) + GetFloat(vm, instruction, 2); break;
        case 51: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) - GetInt(vm, instruction, 2); break;
        case 52: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1) - GetFloat(vm, instruction, 2); break;
        case 53: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) * GetInt(vm, instruction, 2); break;
        case 54: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1) * GetFloat(vm, instruction, 2); break;
        case 55: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) / GetInt(vm, instruction, 2); break;
        case 56: *GetFloatPtr(vm, instruction, 0) = GetFloat(vm, instruction, 1) / GetFloat(vm, instruction, 2); break;
        case 57: *GetIntPtr(vm, instruction, 0) = GetInt(vm, instruction, 1) % GetInt(vm, instruction, 2); break;
        case 58: *GetFloatPtr(vm, instruction, 0) = (float)fmod(GetFloat(vm, instruction, 1), GetFloat(vm, instruction, 2)); break;
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
        case 79:
            if (TimerAt(vm, 0x3c)->current == 0)
                ResetTimer(TimerAt(vm, 0x3c), GetInt(vm, instruction, 0));
            else
                TimerAt(vm, 0x3c)->Decrement(1);
            if (TimerAt(vm, 0x3c)->current > 0) { TimerAt(vm, 0x30)->Decrement(1); goto advance; }
            ResetTimer(TimerAt(vm, 0x3c), 0); break;
        case 80: VM_F(vm, 0xf0) = GetFloat(vm, instruction, 0); break;
        case 81: VM_F(vm, 0xf4) = GetFloat(vm, instruction, 0); break;
        default: break;
        }
        VM_P(vm, 0x1e0) = reinterpret_cast<AnmRawInstr *>(reinterpret_cast<u8 *>(instruction) + instruction->instructionSize);
    }

advance:
    if (VM_F(vm, 12) != 0.0f) { VM_F(vm, 0) = AddNormalizeAngle(VM_F(vm, 0), g_FrameMultiplier * VM_F(vm, 12)); VM_I(vm, 0x1c0) |= 4; }
    if (VM_F(vm, 16) != 0.0f) { VM_F(vm, 4) = AddNormalizeAngle(VM_F(vm, 4), g_FrameMultiplier * VM_F(vm, 16)); VM_I(vm, 0x1c0) |= 4; }
    if (VM_F(vm, 20) != 0.0f) { VM_F(vm, 8) = AddNormalizeAngle(VM_F(vm, 8), g_FrameMultiplier * VM_F(vm, 20)); VM_I(vm, 0x1c0) |= 4; }

    for (i = 0; i < 5; ++i) {
        AnmTimer *currentTimer = TimerAt(vm, 0x48 + i * 12);
        AnmTimer *endTimer = TimerAt(vm, 0x84 + i * 12);
        if (endTimer->current > 0) {
            AdvanceTimer(currentTimer);
            if (currentTimer->current >= endTimer->current) { interp = 1.0f; ResetTimer(endTimer, 0); }
            else interp = ((float)currentTimer->current + currentTimer->subFrame) / ((float)endTimer->current + endTimer->subFrame);
            switch (vm->raw[0xc0 + i]) { case 1: interp *= interp; break; case 2: interp = interp * interp * interp; break; case 3: interp *= interp; interp *= interp; break; case 4: interp = 1.0f - interp; interp *= interp; interp = 1.0f - interp; break; case 5: interp = 1.0f - interp; interp = interp * interp * interp; interp = 1.0f - interp; break; case 6: interp = 1.0f - interp; interp *= interp; interp *= interp; interp = 1.0f - interp; break; }
            if (i == 0) { const i32 out = (VM_I(vm, 0x1c0) & 0x80) ? 0x230 : 0x1c8; VM_F(vm,out)=interp*(VM_F(vm,0x1f4)-VM_F(vm,0x1e8))+VM_F(vm,0x1e8); VM_F(vm,out+4)=interp*(VM_F(vm,0x1f8)-VM_F(vm,0x1ec))+VM_F(vm,0x1ec); VM_F(vm,out+8)=interp*(VM_F(vm,0x1fc)-VM_F(vm,0x1f0))+VM_F(vm,0x1f0); }
            else if (i == 1) { vm->raw[0x1ba] = (u8)(interp * (vm->raw[0x22e] - vm->raw[0x22a]) + vm->raw[0x22a]); vm->raw[0x1b9] = (u8)(interp * (vm->raw[0x22d] - vm->raw[0x229]) + vm->raw[0x229]); vm->raw[0x1b8] = (u8)(interp * (vm->raw[0x22c] - vm->raw[0x228]) + vm->raw[0x228]); }
            else if (i == 2) vm->raw[0x1bb] = (u8)(interp * (vm->raw[0x22f] - vm->raw[0x22b]) + vm->raw[0x22b]);
            else if (i == 3) { VM_F(vm,0)=AddNormalizeAngle((VM_F(vm,0x20c)-VM_F(vm,0x200))*interp,VM_F(vm,0x200)); VM_F(vm,4)=AddNormalizeAngle((VM_F(vm,0x210)-VM_F(vm,0x204))*interp,VM_F(vm,0x204)); VM_F(vm,8)=AddNormalizeAngle((VM_F(vm,0x214)-VM_F(vm,0x208))*interp,VM_F(vm,0x208)); VM_I(vm,0x1c0)|=4; }
            else if (i == 4) { VM_F(vm,0x18)=interp*(VM_F(vm,0x220)-VM_F(vm,0x218))+VM_F(vm,0x218); VM_F(vm,0x1c)=interp*(VM_F(vm,0x224)-VM_F(vm,0x21c))+VM_F(vm,0x21c); VM_I(vm,0x1c0)|=8; }
        }
    }
    if (VM_F(vm, 36) != 0.0f) { VM_F(vm, 28) += g_FrameMultiplier * VM_F(vm, 36); VM_I(vm, 0x1c0) |= 8; }
    if (VM_F(vm, 32) != 0.0f) { VM_F(vm, 24) += g_FrameMultiplier * VM_F(vm, 32); VM_I(vm, 0x1c0) |= 12; }
    VM_F(vm, 40) += VM_F(vm, 0xf0); WrapUnit(&VM_F(vm, 40)); VM_F(vm, 44) += VM_F(vm, 0xf4); WrapUnit(&VM_F(vm, 44));
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
