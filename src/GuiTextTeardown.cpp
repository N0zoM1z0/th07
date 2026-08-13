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

void GuiTextTeardown::Destroy()
{
    Target428B19();
}

} // namespace th07
