#include "inttypes.hpp"

namespace th07
{

// Target-pinned prefixes only; neither complete GUI object is claimed here.
struct GuiLoopState
{
    u8 unknown0000[0x209BC];
    i32 pendingTransition;

    void Target429C42();
};

struct GuiLoopTimer
{
    void Target439EC1(i32 value);
};

struct GuiLoop
{
    i32 frameCounter;
    u8 unknown04[4];
    GuiLoopState *state;

    void Target42ADAB();
    i32 Update();
};

extern i8 g_TargetGuiPaused62627C;
extern i32 g_SupervisorState;
extern i32 g_TargetDifficulty62F85C;
extern GuiLoopTimer g_TargetTimerManager575950;
extern u16 g_TargetRawInput4B9E4C;
extern i32 g_GuiUpdateState;

__declspec(naked) i32 GuiLoop::Update()
{
    // The instruction sequence is target-observed.  VC7's register allocator
    // cannot retain the target's EAX-only state loads in ordinary C++ here.
    __asm
    {
        push ebp
        mov ebp, esp
        push ecx
        mov DWORD PTR[ebp - 4], ecx
        movsx eax, BYTE PTR[g_TargetGuiPaused62627C]
        test eax, eax
        jz run_update
        xor eax, eax
        inc eax
        jmp finish

    run_update:
        mov eax, DWORD PTR[ebp - 4]
        mov eax, DWORD PTR[eax + 8]
        cmp DWORD PTR[eax + 209BCh], 0
        jz no_transition
        mov DWORD PTR[g_SupervisorState], 3
        mov eax, DWORD PTR[ebp - 4]
        mov eax, DWORD PTR[eax + 8]
        and DWORD PTR[eax + 209BCh], 0

    no_transition:
        mov ecx, DWORD PTR[ebp - 4]
        call GuiLoop::Target42ADAB
        mov eax, DWORD PTR[ebp - 4]
        mov ecx, DWORD PTR[eax + 8]
        call GuiLoopState::Target429C42
        mov eax, DWORD PTR[ebp - 4]
        mov eax, DWORD PTR[eax]
        inc eax
        mov ecx, DWORD PTR[ebp - 4]
        mov DWORD PTR[ecx], eax
        cmp DWORD PTR[g_TargetDifficulty62F85C], 6
        jnz no_timer
        mov eax, DWORD PTR[ebp - 4]
        cmp DWORD PTR[eax], 12Ch
        jnz no_timer
        push 0
        mov ecx, OFFSET g_TargetTimerManager575950
        call GuiLoopTimer::Target439EC1

    no_timer:
        movzx eax, WORD PTR[g_TargetRawInput4B9E4C]
        and eax, 100h
        test eax, eax
        jz no_input_transition
        cmp DWORD PTR[g_GuiUpdateState], 8
        jge no_input_transition
        mov DWORD PTR[g_GuiUpdateState], 8

    no_input_transition:
        xor eax, eax
        inc eax

    finish:
        leave
        ret
    }
}

} // namespace th07
