#include "inttypes.hpp"

namespace th07
{

// TH06 has a uniquely related TextHelper destructor.  This is intentionally
// private until the TH07 owner around the target vtable is fully recovered.
struct GuiTextTeardown
{
    void Target428B19();
    void Destroy();
};

__declspec(naked) void GuiTextTeardown::Destroy()
{
    __asm
    {
        push ebp
        mov ebp, esp
        push ecx
        mov DWORD PTR[ebp - 4], ecx
        mov ecx, DWORD PTR[ebp - 4]
        call GuiTextTeardown::Target428B19
        leave
        ret
    }
}

} // namespace th07
