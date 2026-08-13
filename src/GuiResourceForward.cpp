#include "inttypes.hpp"

namespace th07
{
#pragma optimize("s", on)

// The target vtable references this forwarding slot.  The called member's
// result is retained by the wrapper, so its ABI is i32 rather than void.
struct GuiResourceTeardown
{
    i32 Target45BF15();
    i32 Destroy();
};

i32 GuiResourceTeardown::Destroy()
{
    return Target45BF15();
}

#pragma optimize("s", off)
} // namespace th07
