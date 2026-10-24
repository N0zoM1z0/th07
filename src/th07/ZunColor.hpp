#pragma once

#include <d3d8.h>

#include "inttypes.hpp"

union ZunColor {
    static u8 Multiply(u8 src, u8 factor)
    {
        u32 tmp = (u32)src * factor >> 7;
        if (tmp >= 256)
        {
            tmp = 255;
        }
        return tmp;
    }

    D3DCOLOR color;
    struct ColorBytes
    {
        u8 b;
        u8 g;
        u8 r;
        u8 a;
    } bytes;
    u8 raw[4];
};
