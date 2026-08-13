#include "Midi.hpp"

namespace th07
{
// Target caller 0x00438DCD supplies the global MIDI output as ECX.  The
// wrapper configures its inherited MidiTimer with the observed default period
// and lets StartTimer select its default callback.
UINT MidiOutput::StartDefaultTimer()
{
    return StartTimer(6, NULL, 0);
}
} // namespace th07
