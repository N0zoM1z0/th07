#include "inttypes.hpp"

#include <math.h>

extern "C" f64 __cdecl _CIatan2();
#pragma intrinsic(atan2)

namespace th07 {

static const f32 kPlayerAimPiOverTwo = 1.57079637f;

struct PlayerAimVec3 {
    f32 x;
    f32 y;
    f32 z;
};

struct PlayerAimOverlay {
    u8 unknown0000[0x930];
    PlayerAimVec3 positionCenter;

    void DrawSecondaryBullets();
    i32 OnDrawSecondaryBullets();
    f32 AngleToPoint(const PlayerAimVec3 *point);
};

i32 PlayerAimOverlay::OnDrawSecondaryBullets()
{
    DrawSecondaryBullets();
    return 1;
}

#pragma var_order(relativeY, relativeX)
f32 PlayerAimOverlay::AngleToPoint(const PlayerAimVec3 *point)
{
    f32 relativeX;
    f32 relativeY;

    relativeX = positionCenter.x - point->x;
    relativeY = positionCenter.y - point->y;

    if (relativeY == 0.0f && relativeX == 0.0f) {
        return kPlayerAimPiOverTwo;
    }
    return static_cast<f32>(atan2(relativeY, relativeX));
}

} // namespace th07
