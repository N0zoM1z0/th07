#pragma once

#include "inttypes.hpp"

#include <stddef.h>

namespace th07
{

// Observed in TH07 CallEclSub at context+0x04 and context+0x80.  Each
// InitializeForPopup call writes subFrame, current, then previous (-999).
struct EclTimer
{
    i32 previous;
    i32 current;
    i32 subFrame;

    void InitializeForPopup()
    {
        subFrame = 0;
        current = 0;
        previous = -999;
    }
};
typedef char EclTimerSizeCheck[sizeof(EclTimer) == 0xC ? 1 : -1];

// The instruction representation is deliberately incomplete: these two
// functions only transfer an instruction pointer and never inspect its bytes.
struct EclRawInstr;

// Observed portions of the TH07 per-enemy ECL context.  The unknown regions
// preserve target-attested offsets for the operand-resolver and dispatcher
// lanes; they are not claims about their individual fields.
struct EnemyEclContext
{
    EclRawInstr *currentInstr; // +0x000, observed
    EclTimer time;             // +0x004, observed
    u8 unknown10[0x70];
    EclTimer unknown80; // +0x080, observed reset by CallEclSub
    u8 unknown8C[0x188];
    i16 subId; // +0x214, observed
};
typedef char EnemyEclContextSubIdOffsetCheck[offsetof(EnemyEclContext, subId) == 0x214 ? 1 : -1];

// Inferred from TH06's same-role file format.  No member of this header is
// read by the implemented TH07 functions; the opaque file pointer is enough
// for Unload while the separately observed subroutine table drives CallEclSub.
struct EclRawHeader;
struct Enemy;

enum ZunResult
{
    ZUN_SUCCESS = 0,
    ZUN_ERROR = -1,
};

struct EclManager
{
    void Unload();
    ZunResult CallEclSub(EnemyEclContext *context, i16 subId);
    ZunResult RunEcl(Enemy *enemy);

    EclRawHeader *eclFile; // +0x0, observed
    EclRawInstr **subTable; // +0x4, observed
};

} // namespace th07
