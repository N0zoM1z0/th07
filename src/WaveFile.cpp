#include "inttypes.hpp"

namespace th07
{
struct WaveFile
{
    u8 unknown00[0x2C];
    i32 size;

    i32 GetSize();
};

i32 WaveFile::GetSize()
{
    return size;
}
} // namespace th07
