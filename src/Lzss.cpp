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

struct LzssTreeNode
{
    i32 parent;
    i32 left;
    i32 right;
};

// Target encoder helpers use an 8192-entry dictionary plus this 8193-node
// binary-search tree.  The final node is the target-observed root sentinel.
LzssTreeNode g_LzssTree[LZSS_DICTSIZE + 1];

#define LZSS_DICTPOS_MOD(position, amount) (((position) + (amount)) & LZSS_DICTSIZE_MASK)

void __fastcall LzssInitTree(i32 root)
{
    g_LzssTree[LZSS_DICTSIZE].right = root;
    g_LzssTree[root].parent = LZSS_DICTSIZE;
    g_LzssTree[root].right = 0;
    g_LzssTree[root].left = 0;
}

#pragma var_order(i)
void __cdecl LzssInitEncoderState()
{
    i32 i;

    for (i = 0; i < LZSS_DICTSIZE; i++)
    {
        g_LzssDictionary[i] = 0;
    }
    for (i = 0; i < LZSS_DICTSIZE + 1; i++)
    {
        g_LzssTree[i].parent = 0;
        g_LzssTree[i].left = 0;
        g_LzssTree[i].right = 0;
    }
}

void __fastcall LzssContractNode(i32 oldNode, i32 newNode);
void __fastcall LzssReplaceNode(i32 oldNode, i32 newNode);
void __fastcall LzssDeleteString(i32 position);
i32 __fastcall LzssFindNextNode(i32 node);

#pragma var_order(i, child, testNode, matchLength, delta)
i32 __fastcall LzssAddString(i32 newNode, i32 *matchPosition)
{
    i32 i;
    i32 *child;
    i32 delta;

    if (newNode == 0)
    {
        return 0;
    }

    i32 testNode = g_LzssTree[LZSS_DICTSIZE].right;
    i32 matchLength = 0;

    for (;;)
    {
        for (i = 0; i < 18; i++)
        {
            delta = g_LzssDictionary[LZSS_DICTPOS_MOD(newNode, i)]
                  - g_LzssDictionary[LZSS_DICTPOS_MOD(testNode, i)];
            if (delta != 0)
            {
                break;
            }
        }

        if (i >= matchLength)
        {
            matchLength = i;
            *matchPosition = testNode;
            if (matchLength >= 18)
            {
                LzssReplaceNode(testNode, newNode);
                return matchLength;
            }
        }

        if (delta >= 0)
        {
            child = &g_LzssTree[testNode].right;
        }
        else
        {
            child = &g_LzssTree[testNode].left;
        }

        if (*child == 0)
        {
            *child = newNode;
            g_LzssTree[newNode].parent = testNode;
            g_LzssTree[newNode].right = 0;
            g_LzssTree[newNode].left = 0;
            return matchLength;
        }

        testNode = *child;
    }
}

void __fastcall LzssDeleteString(i32 position)
{
    if (g_LzssTree[position].parent == 0)
    {
        return;
    }

    if (g_LzssTree[position].right == 0)
    {
        LzssContractNode(position, g_LzssTree[position].left);
    }
    else if (g_LzssTree[position].left == 0)
    {
        LzssContractNode(position, g_LzssTree[position].right);
    }
    else
    {
        i32 replacement = LzssFindNextNode(position);
        LzssDeleteString(replacement);
        LzssReplaceNode(position, replacement);
    }
}

void __fastcall LzssContractNode(i32 oldNode, i32 newNode)
{
    g_LzssTree[newNode].parent = g_LzssTree[oldNode].parent;

    if (g_LzssTree[g_LzssTree[oldNode].parent].right == oldNode)
    {
        g_LzssTree[g_LzssTree[oldNode].parent].right = newNode;
    }
    else
    {
        g_LzssTree[g_LzssTree[oldNode].parent].left = newNode;
    }
    g_LzssTree[oldNode].parent = 0;
}

void __fastcall LzssReplaceNode(i32 oldNode, i32 newNode)
{
    i32 parent = g_LzssTree[oldNode].parent;

    if (g_LzssTree[parent].left == oldNode)
    {
        g_LzssTree[parent].left = newNode;
    }
    else
    {
        g_LzssTree[parent].right = newNode;
    }
    g_LzssTree[newNode] = g_LzssTree[oldNode];
    g_LzssTree[g_LzssTree[newNode].left].parent = newNode;
    g_LzssTree[g_LzssTree[newNode].right].parent = newNode;
    g_LzssTree[oldNode].parent = 0;
}

i32 __fastcall LzssFindNextNode(i32 node)
{
    i32 next = g_LzssTree[node].left;

    while (g_LzssTree[next].right != 0)
    {
        next = g_LzssTree[next].right;
    }
    return next;
}

#define ENCODE_ADVANCE_WRITE_HEAD                                                                                      \
    outBitMask >>= 1;                                                                                                  \
    if (outBitMask == 0)                                                                                               \
    {                                                                                                                  \
        *outCursor++ = outBits;                                                                                        \
        checksum += outBits;                                                                                           \
        outBits = 0;                                                                                                   \
        outBitMask = 0x80;                                                                                             \
    }

#define ENCODE_PACK_BIT(bit)                                                                                           \
    if (bit)                                                                                                           \
    {                                                                                                                  \
        outBits |= outBitMask;                                                                                         \
    }                                                                                                                  \
    ENCODE_ADVANCE_WRITE_HEAD;

#define ENCODE_PACK_BITS(bitCount, writeOneIf)                                                                         \
    bitfieldMask = 0x1 << ((bitCount)-1);                                                                              \
    while (bitfieldMask != 0)                                                                                          \
    {                                                                                                                  \
        if (writeOneIf)                                                                                                \
        {                                                                                                              \
            outBits |= outBitMask;                                                                                     \
        }                                                                                                              \
        ENCODE_ADVANCE_WRITE_HEAD;                                                                                     \
        bitfieldMask >>= 1;                                                                                            \
    }

#pragma var_order(outBits, out, outCursor, matchOffset, i, bytesToCopyToDict, outBitMask, inCursor, matchLength,       \
                  checksum, maxMatchLength, dictValue, dictHead, bitfieldMask)
u8 *__fastcall LzssEncode(u8 *input, i32 inputSize, i32 *compressedSize)
{
    u8 outBitMask;
    u32 outBits;
    u32 checksum;
    u8 *out;
    u8 *inCursor;
    u8 *outCursor;
    u32 dictHead;
    i32 i;
    i32 maxMatchLength;
    i32 matchLength;
    i32 matchOffset;
    i32 bytesToCopyToDict;
    i32 dictValue;
    u32 bitfieldMask;

    outBitMask = 0x80;
    outBits = 0;
    checksum = 0;

    out = (u8 *)GlobalAlloc(0, inputSize * 2);
    if (out == NULL)
    {
        return NULL;
    }

    inCursor = input;
    outCursor = out;
    *compressedSize = 0;
    LzssInitEncoderState();
    dictHead = 1;

    for (i = 0; i < 18; i++)
    {
        if (inCursor - input >= inputSize)
        {
            dictValue = -1;
        }
        else
        {
            dictValue = *inCursor++;
        }

        if (dictValue == -1)
        {
            break;
        }

        g_LzssDictionary[dictHead + i] = dictValue;
    }

    maxMatchLength = i;
    LzssInitTree(dictHead);
    matchLength = 0;
    matchOffset = 0;

    while (maxMatchLength > 0)
    {
        if (matchLength > maxMatchLength)
        {
            matchLength = maxMatchLength;
        }

        if (matchLength <= 2)
        {
            bytesToCopyToDict = 1;
            ENCODE_PACK_BIT(1);
            ENCODE_PACK_BITS(8, (bitfieldMask & g_LzssDictionary[dictHead]) != 0);
        }
        else
        {
            ENCODE_PACK_BIT(0);
            ENCODE_PACK_BITS(13, (bitfieldMask & matchOffset) != 0);
            ENCODE_PACK_BITS(4, (bitfieldMask & (matchLength - 3)) != 0);
            bytesToCopyToDict = matchLength;
        }

        for (i = 0; i < bytesToCopyToDict; i++)
        {
            LzssDeleteString(LZSS_DICTPOS_MOD(dictHead, 18));

            if (inCursor - input >= inputSize)
            {
                dictValue = -1;
            }
            else
            {
                dictValue = *inCursor++;
            }

            if (dictValue == -1)
            {
                maxMatchLength--;
            }
            else
            {
                g_LzssDictionary[LZSS_DICTPOS_MOD(dictHead, 18)] = dictValue;
            }

            dictHead = LZSS_DICTPOS_MOD(dictHead, 1);
            if (maxMatchLength != 0)
            {
                matchLength = LzssAddString(dictHead, &matchOffset);
            }
        }
    }

    ENCODE_PACK_BIT(0);
    ENCODE_PACK_BITS(13, false);

    *compressedSize = outCursor - out;
    return out;

#undef ENCODE_ADVANCE_WRITE_HEAD
#undef ENCODE_PACK_BIT
#undef ENCODE_PACK_BITS
}

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
