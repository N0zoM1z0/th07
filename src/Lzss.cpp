#include "Lzss.hpp"

#include <windows.h>

namespace th07
{
enum
{
    LZSS_DICTSIZE = 0x2000,
    LZSS_DICTSIZE_MASK = LZSS_DICTSIZE - 1,
};

// The target directly reads and updates its 0x2000-byte persistent decode
// dictionary at 0x004B7E40.  It is intentionally not reset per decode.
u8 g_LzssDictionary[LZSS_DICTSIZE];

#define LZSS_DICTPOS_MOD(position, amount) (((position) + (amount)) & LZSS_DICTSIZE_MASK)

#define DECODE_ADVANCE_READ_HEAD                                                                                       \
    inBitMask >>= 1;                                                                                                   \
    if (inBitMask == 0)                                                                                                \
    {                                                                                                                  \
        inBitMask = 0x80;                                                                                              \
    }

#define DECODE_WRITE_BYTE(data)                                                                                        \
    *outCursor++ = data;                                                                                               \
    g_LzssDictionary[dictHead] = data;                                                                                 \
    dictHead = LZSS_DICTPOS_MOD(dictHead, 1);

#define DECODE_HANDLE_FETCH                                                                                            \
    if (inBitMask == 0x80)                                                                                             \
    {                                                                                                                  \
        currByte = *inCursor;                                                                                          \
        if (inCursor - (u8 *)compressedData >= size)                                                                  \
        {                                                                                                              \
            currByte = 0;                                                                                              \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            inCursor++;                                                                                                \
        }                                                                                                              \
        checksum += currByte;                                                                                          \
    }

#define DECODE_UNPACK_BIT                                                                                              \
    DECODE_HANDLE_FETCH;                                                                                               \
    inBits = currByte & inBitMask;                                                                                     \
    DECODE_ADVANCE_READ_HEAD;

#define DECODE_UNPACK_BITS(bitCount)                                                                                   \
    outBitMask = 0x1 << ((bitCount)-1);                                                                                \
    inBits = 0;                                                                                                        \
    while (outBitMask != 0)                                                                                            \
    {                                                                                                                  \
        DECODE_HANDLE_FETCH;                                                                                           \
        if ((currByte & inBitMask) != 0)                                                                               \
        {                                                                                                              \
            inBits |= outBitMask;                                                                                      \
        }                                                                                                              \
        outBitMask >>= 1;                                                                                              \
        DECODE_ADVANCE_READ_HEAD;                                                                                      \
    }

#pragma var_order(currByte, outCursor, matchOffset, i, inBitMask, inCursor, inBits, size, matchLength, checksum,       \
                  dictValue, outBitMask, dictHead)
void *__fastcall LzssDecode(void *compressedData, u32 compressedSize, void *outBuffer, u32 decompressedSize)
{
    u8 inBitMask;
    u32 currByte;
    u32 checksum;
    i32 size;
    u8 *inCursor;
    u8 *outCursor;
    u32 dictHead;
    u32 inBits;
    i32 matchOffset;
    i32 matchLength;
    i32 i;
    u32 dictValue;
    u32 outBitMask;
    inBitMask = 0x80;
    currByte = 0;
    checksum = 0;
    size = compressedSize;

    if (outBuffer == NULL)
    {
        outBuffer = GlobalAlloc(0, decompressedSize);
        if (outBuffer == NULL)
        {
            return NULL;
        }
    }

    inCursor = (u8 *)compressedData;
    outCursor = (u8 *)outBuffer;
    dictHead = 1;

    for (;;)
    {
        DECODE_UNPACK_BIT;

        if (inBits != 0)
        {
            DECODE_UNPACK_BITS(8);
            DECODE_WRITE_BYTE(inBits);
        }
        else
        {
            DECODE_UNPACK_BITS(13);
            matchOffset = inBits;
            if (matchOffset == 0)
            {
                break;
            }

            DECODE_UNPACK_BITS(4);
            matchLength = inBits + 2;
            for (i = 0; i <= matchLength; i++)
            {
                dictValue = g_LzssDictionary[LZSS_DICTPOS_MOD(matchOffset, i)];
                DECODE_WRITE_BYTE(dictValue);
            }
        }
    }

    while (inBitMask != 0x80)
    {
        DECODE_UNPACK_BIT;
    }

    return outBuffer;

#undef DECODE_ADVANCE_READ_HEAD
#undef DECODE_WRITE_BYTE
#undef DECODE_HANDLE_FETCH
#undef DECODE_UNPACK_BIT
#undef DECODE_UNPACK_BITS
}
} // namespace th07
