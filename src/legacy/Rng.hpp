#pragma once

#include "inttypes.hpp"

namespace th07
{

class Rng
{
  public:
    u16 GetRandomU16();
    u32 GetRandomU32();
    f32 GetRandomF32();

    f32 GetRandomF32InRange(f32 range)
    {
        return GetRandomF32() * range;
    }

  private:
    u16 seed;
    u16 seedBackup;
    u32 generationCount;
};

f32 __stdcall AddNormalizeAngle(f32 angle, f32 delta);

} // namespace th07
