#pragma once

#include "inttypes.hpp"

namespace th07
{

// The dispatcher ABI is target-observed.  The VM layout remains private to
// AnmExecute.cpp until every named offset has been reconciled against TH07.
struct AnmVm;

struct AnmManager
{
    i32 ExecuteScript(AnmVm *vm);
};

} // namespace th07
