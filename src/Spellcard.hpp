#pragma once

#include "inttypes.hpp"

#include <d3dx8math.h>

namespace th07
{

// Target-observed formatted-text record used by Gui::ShowSpellcardBonus.
struct GuiFormattedText
{
    D3DXVECTOR3 position;
    u32 value;
    i32 isShown;
    struct GuiTimer
    {
        i32 previous;
        f32 subFrame;
        i32 current;

        void InitializeForPopup()
        {
            current = 0;
            subFrame = 0.0f;
            previous = -999;
        }
    } timer;
};
typedef char GuiFormattedTextSizeCheck[sizeof(GuiFormattedText) == 0x20 ? 1 : -1];

struct SpellcardGuiImpl
{
    u8 unknown00000[0x20A00];
    GuiFormattedText spellcardBonus;
};

class SpellcardGui
{
  public:
    void ShowSpellcardBonus(u32 score);

    u8 unknown00[8];
    SpellcardGuiImpl *impl;
};

} // namespace th07
