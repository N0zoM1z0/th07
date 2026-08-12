#pragma once

#include "inttypes.hpp"

namespace th07
{

struct ZunTimer
{
    i32 previous;
    f32 subFrame;
    i32 current;

    void Increment(i32 value);
    void Decrement(i32 value);
};

struct TimerManager
{
    u8 unknown000[0x178];
    f32 effectiveFrameMultiplier;

    void Advance(i32 *current, f32 *subFrame);
};

} // namespace th07
