// Emit the shared constructor with its normal inline initialization profile.
// VC7 dllexport ensures emission of this required COMDAT even though the
// allocation call is compiled in ResultRegister.cpp. This adds one PE export;
// whole-file identity is outside the function-byte reconstruction claim.
#define TH07_RESULT_CTOR_EXPORT
#pragma optimize("s", on)
#include "Global.hpp"
#pragma function(memset)
#include "ResultScreen.hpp"
