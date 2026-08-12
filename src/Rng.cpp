#include "Rng.hpp"

namespace th07
{

u16 Rng::GetRandomU16()
{
    u16 value = (this->seed ^ 0x9630) - 0x6553;
    this->seed = (((value & 0xC000) >> 14) + value * 4) & 0xFFFF;
    this->generationCount++;
    return this->seed;
}

u32 Rng::GetRandomU32()
{
    return GetRandomU16() << 16 | GetRandomU16();
}

f32 Rng::GetRandomF32()
{
    return (f32)GetRandomU32() / (f32)0xFFFFFFFF;
}

f32 __stdcall AddNormalizeAngle(f32 angle, f32 delta)
{
    i32 iterations = 0;
    angle += delta;
    while (angle > 3.14159265358979323846f)
    {
        angle -= 6.28318530717958647692f;
        if (iterations++ > 16)
            break;
    }
    while (angle < -3.14159265358979323846f)
    {
        angle += 6.28318530717958647692f;
        if (iterations++ > 16)
            break;
    }
    return angle;
}

} // namespace th07
