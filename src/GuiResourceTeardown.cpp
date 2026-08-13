#include "inttypes.hpp"
namespace th07
{
struct GuiResourceTeardown { void Target45BF15(); void Destroy(); };
__declspec(naked) void GuiResourceTeardown::Destroy() { __asm {
    push ebp
    mov ebp, esp
    push ecx
    mov DWORD PTR[ebp - 4], ecx
    mov ecx, DWORD PTR[ebp - 4]
    call GuiResourceTeardown::Target45BF15
    leave
    ret
} }
} // namespace th07
