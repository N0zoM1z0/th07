#include "inttypes.hpp"

#include <windows.h>

namespace th07
{
// Target 0x004378B0 has a thiscall receiver home but accesses no owner field.
// Its only observed effect samples the BSS counter immediately preceding the
// target controller-state storage at 0x0135E218.
struct PerformanceCounterBootstrap
{
    BOOL SamplePerformanceCounter();
};

extern LARGE_INTEGER g_PerformanceCount;

BOOL PerformanceCounterBootstrap::SamplePerformanceCounter()
{
    return QueryPerformanceCounter(&g_PerformanceCount);
}
} // namespace th07
