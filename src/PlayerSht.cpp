#include "inttypes.hpp"

namespace th07 {

struct PlayerShtInstruction {
    i16 opcode;
    u8 unknown02[0x22];
    void *callback0;
    void *callback1;
    void *callback2;
    void *callback3;
};

struct PlayerShtHeader {
    u16 unknown00;
    u16 count;
    u8 unknown04[0x30];
    struct Entry {
        PlayerShtInstruction *instructions;
        u32 unknown04;
    } entries[1];
};

struct PlayerShtFileSystem {
    static void *__fastcall OpenPath(const char *path, i32 mode);
};

extern void *g_PlayerShtCallbacks0[];
extern void *g_PlayerShtCallbacks1[];
extern void *g_PlayerShtCallbacks2[];
extern void *g_PlayerShtCallbacks3[];

struct PlayerSht {
    static i32 __fastcall LoadShtFile(PlayerShtHeader **header, const char *path);
};

i32 __fastcall PlayerSht::LoadShtFile(PlayerShtHeader **header, const char *path)
{
    i32 index;
    PlayerShtInstruction *instruction;

    *header = static_cast<PlayerShtHeader *>(PlayerShtFileSystem::OpenPath(path, 0));
    if (!*header) {
        return -1;
    }
    for (index = 0; index < (*header)->count; ++index) {
        reinterpret_cast<u32 &>((*header)->entries[index].instructions) += reinterpret_cast<u32>(*header);
        instruction = (*header)->entries[index].instructions;
        while (instruction->opcode >= 0) {
            instruction->callback0 = g_PlayerShtCallbacks0[reinterpret_cast<u32>(instruction->callback0)];
            instruction->callback1 = g_PlayerShtCallbacks1[reinterpret_cast<u32>(instruction->callback1)];
            instruction->callback2 = g_PlayerShtCallbacks2[reinterpret_cast<u32>(instruction->callback2)];
            instruction->callback3 = g_PlayerShtCallbacks3[reinterpret_cast<u32>(instruction->callback3)];
            ++instruction;
        }
    }
    return 0;
}

} // namespace th07
