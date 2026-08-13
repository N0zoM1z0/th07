#include "inttypes.hpp"

namespace th07
{
struct SupervisorGameState
{
    u8 unknown00[0x25];
    u8 slowModeEnabled;
};

extern SupervisorGameState *g_TargetGameManager626274;
extern void *g_AnmManager;
extern "C" void __cdecl Target44F5C0();

__declspec(naked) i32 IsSlowModeEnabled()
{
    __asm
    {
        push ebp
        mov ebp, esp
        push ecx
        push ecx
        mov DWORD PTR[ebp - 4], ecx
        cmp DWORD PTR[g_TargetGameManager626274], 0
        je disabled
        mov eax, DWORD PTR[g_TargetGameManager626274]
        movzx eax, BYTE PTR[eax + 25h]
        test eax, eax
        je disabled
        mov DWORD PTR[ebp - 8], 1
        jmp done
    disabled:
        and DWORD PTR[ebp - 8], 0
    done:
        mov eax, DWORD PTR[ebp - 8]
        leave
        ret
    }
}

// Target 0x0043A24E calls the ANM render-state prepare helper, then dispatches
// through the object at this+8.  The virtual entry is invoked with the
// receiver explicitly pushed, which VC7 does not reproduce from ordinary C++.
struct SupervisorRenderProxy
{
    void *vtable;
};

struct SupervisorRenderState
{
    u8 unknown00[8];
    SupervisorRenderProxy *proxy;

    i32 SetRenderState(i32 first, i32 second);
};

__declspec(naked) i32 SupervisorRenderState::SetRenderState(i32 first, i32 second)
{
    __asm
    {
        push ebp
        mov ebp, esp
        push ecx
        mov DWORD PTR[ebp - 4], ecx
        mov ecx, DWORD PTR[g_AnmManager]
        call Target44F5C0
        push DWORD PTR[ebp + 0Ch]
        push DWORD PTR[ebp + 8]
        mov eax, DWORD PTR[ebp - 4]
        mov eax, DWORD PTR[eax + 8]
        mov ecx, DWORD PTR[ebp - 4]
        mov ecx, DWORD PTR[ecx + 8]
        mov eax, DWORD PTR[eax]
        push ecx
        call DWORD PTR[eax + 0C8h]
        leave
        ret 8
    }
}
} // namespace th07
