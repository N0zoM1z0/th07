#include "Timer.hpp"

namespace th07
{

extern u32 g_TimerFlags;
extern f32 g_FrameMultiplier;

#pragma optimize("s", on)
void ZunTimer::Increment(i32 value)
{
    if ((g_TimerFlags >> 5) & 1)
    {
        this->current++;
        this->subFrame = 0.0f;
        this->previous = -999;
    }

    if (g_FrameMultiplier > 0.99f)
    {
        this->current += value;
        return;
    }

    if (value < 0)
    {
        Decrement(-value);
        return;
    }

    this->previous = this->current;
    this->subFrame = (f32)value * g_FrameMultiplier + this->subFrame;
    while (this->subFrame >= 1.0f)
    {
        this->current++;
        this->subFrame -= 1.0f;
    }
}

void ZunTimer::Decrement(i32 value)
{
    if ((g_TimerFlags >> 5) & 1)
    {
        this->current--;
        this->subFrame = 0.0f;
        this->previous = -999;
    }

    if (g_FrameMultiplier > 0.99f)
    {
        this->current -= value;
        return;
    }

    if (value < 0)
    {
        Increment(-value);
        return;
    }

    this->previous = this->current;
    this->subFrame -= (f32)value * g_FrameMultiplier;
    while (this->subFrame < 0.0f)
    {
        this->current--;
        this->subFrame += 1.0f;
    }
}
void TimerManager::Advance(i32 *current, f32 *subFrame)
{
    if (this->effectiveFrameMultiplier <= 0.99f)
    {
        *subFrame += this->effectiveFrameMultiplier;
        if (*subFrame >= 1.0f)
        {
            *current = *current + 1;
            *subFrame -= 1.0f;
        }
    }
    else
    {
        *current = *current + 1;
    }
}
#pragma optimize("s", off)

} // namespace th07
