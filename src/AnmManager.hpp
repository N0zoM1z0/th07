#pragma once

#include "inttypes.hpp"

namespace th07
{

// The dispatcher ABI is target-observed.  The VM layout remains private to
// AnmExecute.cpp until every named offset has been reconciled against TH07.
struct AnmVm;
struct AnmRawInstr;

struct AnmManager
{
    u8 unknown00[8];
    i32 executedScriptCount;

    void SetAndExecuteScript(AnmVm *vm, AnmRawInstr *script);
    i32 ExecuteScript(AnmVm *vm);
};

} // namespace th07
