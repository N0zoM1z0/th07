#include "inttypes.hpp"

namespace th07
{

// Private target wrapper; caller 0x00438FC0 corroborates an owned resource.
struct GuiSoundTeardown
{
    i32 Target4362D0();
    i32 Destroy();
};

i32 GuiSoundTeardown::Destroy()
{
    return Target4362D0();
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
