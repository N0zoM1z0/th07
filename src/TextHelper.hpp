#pragma once

#include "inttypes.hpp"

#include <d3d8.h>
#include <windows.h>

namespace th07
{
struct FormatInfo
{
    D3DFORMAT format;
    i32 bitCount;
    u32 alphaMask;
    u32 redMask;
    u32 greenMask;
    u32 blueMask;
};

class TextHelper
{
  public:
    TextHelper();
    ~TextHelper();

    bool ReleaseBuffer();
    bool AllocateBufferWithFallback(i32 width, i32 height, D3DFORMAT format);
    bool TryAllocateBuffer(i32 width, i32 height, D3DFORMAT format);
    FormatInfo *GetFormatInfo(D3DFORMAT format);

  private:
    D3DFORMAT format;
    i32 width;
    i32 height;
    u32 imageSizeInBytes;
    i32 imageWidthInBytes;
    HDC hdc;
    HGDIOBJ gdiObj;
    HGDIOBJ gdiObj2;
    u8 *buffer;
};

C_ASSERT(sizeof(TextHelper) == 0x24);
}; // namespace th07
