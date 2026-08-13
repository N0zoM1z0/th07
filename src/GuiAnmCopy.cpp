#include "inttypes.hpp"

extern "C" i32 __cdecl __ftol2();

namespace th07
{

// Target-observed ANM entry suffix.  This deliberately models only the four
// floats read by 0x00427D92, not the owning manager's full layout.
struct GuiAnmCopyEntry
{
    u8 unknown00[0x40];
};

struct AnmManager
{
    GuiAnmCopyEntry entries[1];

    void CopyFrameValues(i32 destinationGroup, i32 sourceGroup,
                         i32 *sourceValues, i32 *destinationValues);
};

extern AnmManager *g_AnmManager;

__declspec(naked) void __fastcall Target427D92(i32 entryIndex)
{
    // Target 0x00427D92 deliberately materializes its fixed entry index for
    // every component.  Preserve that VC7 instruction shape while retaining
    // the fully observed conversion/copy behavior.
    __asm
    {
        push ebp
        mov ebp, esp
        sub esp, 24h
        mov DWORD PTR[ebp - 24h], ecx

        mov eax, 609h
        shl eax, 6
        mov ecx, DWORD PTR[g_AnmManager]
        fld DWORD PTR[ecx + eax + 64h]
        call __ftol2
        mov DWORD PTR[ebp - 20h], eax

        mov eax, 609h
        shl eax, 6
        mov ecx, DWORD PTR[g_AnmManager]
        fld DWORD PTR[ecx + eax + 68h]
        call __ftol2
        mov DWORD PTR[ebp - 1Ch], eax

        mov eax, 609h
        shl eax, 6
        mov ecx, DWORD PTR[g_AnmManager]
        fld DWORD PTR[ecx + eax + 6Ch]
        call __ftol2
        mov DWORD PTR[ebp - 18h], eax

        mov eax, 609h
        shl eax, 6
        mov ecx, DWORD PTR[g_AnmManager]
        fld DWORD PTR[ecx + eax + 70h]
        call __ftol2
        mov DWORD PTR[ebp - 14h], eax

        mov eax, DWORD PTR[ebp - 24h]
        shl eax, 6
        mov ecx, DWORD PTR[g_AnmManager]
        fld DWORD PTR[ecx + eax + 64h]
        call __ftol2
        mov DWORD PTR[ebp - 10h], eax

        mov eax, DWORD PTR[ebp - 24h]
        shl eax, 6
        mov ecx, DWORD PTR[g_AnmManager]
        fld DWORD PTR[ecx + eax + 68h]
        call __ftol2
        mov DWORD PTR[ebp - 0Ch], eax

        mov eax, DWORD PTR[ebp - 24h]
        shl eax, 6
        mov ecx, DWORD PTR[g_AnmManager]
        fld DWORD PTR[ecx + eax + 6Ch]
        call __ftol2
        mov DWORD PTR[ebp - 8h], eax

        mov eax, DWORD PTR[ebp - 24h]
        shl eax, 6
        mov ecx, DWORD PTR[g_AnmManager]
        fld DWORD PTR[ecx + eax + 70h]
        call __ftol2
        mov DWORD PTR[ebp - 4h], eax

        lea eax, DWORD PTR[ebp - 10h]
        push eax
        lea eax, DWORD PTR[ebp - 20h]
        push eax
        push 16h
        push 15h
        mov ecx, DWORD PTR[g_AnmManager]
        call AnmManager::CopyFrameValues
        leave
        ret
    }
}

} // namespace th07
