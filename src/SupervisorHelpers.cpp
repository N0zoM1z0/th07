#include "inttypes.hpp"

namespace th07
{
struct SupervisorGameState
{
    u8 unknown00[0x25];
    u8 slowModeEnabled;
};

extern SupervisorGameState *g_TargetGameManager626274;

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
} // namespace th07
