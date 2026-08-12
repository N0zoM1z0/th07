#pragma once

#include "inttypes.hpp"

#include <windows.h>
#include <mmsystem.h>

namespace th07
{
enum ZunResult
{
    ZUN_SUCCESS = 0,
    ZUN_ERROR = -1,
};

struct MidiTimer
{
    MidiTimer();
    ~MidiTimer();

    virtual void OnTimerElapsed() = 0;

    UINT StartTimer(u32 delay, LPTIMECALLBACK callback, DWORD_PTR data);
    BOOL StopTimer();

    static void CALLBACK DefaultTimerCallback(UINT timerId, UINT message, DWORD_PTR user, DWORD_PTR dw1,
                                              DWORD_PTR dw2);

    UINT timerId;
    TIMECAPS timeCaps;
};

struct MidiTrack;

struct MidiDevice
{
    MidiDevice();
    ~MidiDevice();

    BOOL OpenDevice(UINT deviceId);
    ZunResult Close();
    BOOL SendLongMsg(LPMIDIHDR header);
    BOOL SendShortMsg(u8 midiStatus, u8 firstByte, u8 secondByte);

    HMIDIOUT handle;
    u32 deviceId;
};

struct MidiChannel
{
    u8 keyPressedFlags[16];
    u8 instrument;
    u8 instrumentBank;
    u8 pan;
    u8 effectOneDepth;
    u8 effectThreeDepth;
    u8 channelVolume;
    u8 modifiedVolume;
};

struct MidiOutput : MidiTimer
{
    MidiOutput();
    ~MidiOutput();

    virtual void OnTimerElapsed();

    ZunResult StopPlayback();
    void ClearTracks();
    ZunResult ReadFileData(u32 fileIndex, char *path);
    void ReleaseFileData(u32 fileIndex);
    ZunResult ParseFile(i32 fileIndex);
    ZunResult LoadFile(char *midiPath);
    ZunResult SetFadeOut(u32 milliseconds);

    LPMIDIHDR midiHeaders[32];
    i32 midiHeadersCursor;
    u8 *midiFileData[32];
    i32 numTracks;
    u32 format;
    i32 divisions;
    i32 tempo;
    u32 unk124;
    unsigned __int64 volume;
    __int64 unk130;
    MidiTrack *tracks;
    MidiDevice midiOutDev;
    u8 unk144[16];
    MidiChannel channels[16];
    i8 unk2c4;
    f32 fadeOutVolumeMultiplier;
    u32 fadeOutLastSetVolume;
    u32 unk2d0;
    u32 unk2d4;
    u32 unk2d8;
    u32 unk2dc;
    u32 fadeOutFlag;
    i32 fadeOutInterval;
    i32 fadeOutElapsedMS;
    u32 unk2ec;
    unsigned __int64 unk2f0;
    unsigned __int64 unk2f8;
};

typedef char MidiTimerSizeMustBe0x10[(sizeof(MidiTimer) == 0x10) ? 1 : -1];
typedef char MidiDeviceSizeMustBe0x8[(sizeof(MidiDevice) == 0x8) ? 1 : -1];
typedef char MidiOutputSizeMustBe0x300[(sizeof(MidiOutput) == 0x300) ? 1 : -1];
} // namespace th07
