#include "zwave.hpp"

namespace th07
{
// The target destructor is a normal resource-owner teardown: it forwards the
// receiver to the target-attested CWaveFile::Close method and leaves that
// HRESULT in EAX, as VC7 does for this void destructor.
CWaveFile::~CWaveFile()
{
    Close();
}
} // namespace th07
