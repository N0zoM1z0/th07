#include "PbgFile.hpp"

namespace th07
{
// Target 0x0045E500 is the complete virtual destructor.  It restores the
// IPbgFile vptr and returns its receiver; deletion is emitted separately at
// 0x0045E520 by VC7.
IPbgFile::~IPbgFile()
{
}
} // namespace th07
