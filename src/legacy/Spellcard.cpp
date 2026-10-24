#include "Spellcard.hpp"

namespace th07
{

extern i32 g_GuiUpdateState;

void SpellcardGui::ShowSpellcardBonus(u32 score)
{
    impl->spellcardBonus.position = D3DXVECTOR3(224.0f, 16.0f, 0.0f);
    impl->spellcardBonus.isShown = 1;
    impl->spellcardBonus.timer.InitializeForPopup();
    impl->spellcardBonus.value = score;
    g_GuiUpdateState = 2;
}

} // namespace th07
