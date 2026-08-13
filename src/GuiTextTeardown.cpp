#include "inttypes.hpp"

namespace th07
{

#pragma optimize("s", on)

// TH06 has a uniquely related TextHelper destructor.  This is intentionally
// private until the TH07 owner around the target vtable is fully recovered.
struct GuiTextTeardown
{
    i32 Target428B19();
    i32 Destroy();
};

i32 GuiTextTeardown::Destroy()
{
    return Target428B19();
}

#pragma optimize("s", off)

} // namespace th07
