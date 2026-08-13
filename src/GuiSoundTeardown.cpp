#include "inttypes.hpp"

namespace th07
{

// Private target wrapper; caller 0x00438FC0 corroborates an owned resource.
struct GuiSoundTeardown
{
    i32 Target4362D0();
    i32 Destroy();
};

__declspec(naked) i32 GuiSoundTeardown::Destroy()
{
    __asm
    {
        push ebp
        mov ebp, esp
        push ecx
        mov DWORD PTR[ebp - 4], ecx
        mov ecx, DWORD PTR[ebp - 4]
        call GuiSoundTeardown::Target4362D0
        leave
        ret
    }
}

struct GuiTimerTeardown
{
    i32 Target436380();
    i32 Destroy();
};

i32 GuiTimerTeardown::Destroy()
{
    return Target436380();
}

} // namespace th07
