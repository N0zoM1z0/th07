#pragma once

#include "inttypes.hpp"

namespace th07
{
// Target 0x0045EF00 receives its compressed-data pointer and size in ECX/EDX,
// then takes the optional output buffer and its size on the stack.
void *__fastcall LzssDecode(void *compressedData, u32 compressedSize, void *outBuffer, u32 decompressedSize);
} // namespace th07
