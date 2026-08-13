#include "Midi.hpp"

namespace th07
{
// The window handle belongs to the supervisor/window reconstruction lane.
extern HWND g_GameWindow;

MidiDevice::MidiDevice()
{
    handle = NULL;
    deviceId = 0;
}

MidiDevice::~MidiDevice()
{
    Close();
}

BOOL MidiDevice::OpenDevice(UINT newDeviceId)
{
    if (handle != NULL)
    {
        if (deviceId != newDeviceId)
        {
            Close();
        }
        else
        {
            return FALSE;
        }
    }

    deviceId = newDeviceId;
    return midiOutOpen(&handle, newDeviceId, (DWORD_PTR)g_GameWindow, NULL, CALLBACK_WINDOW) != MMSYSERR_NOERROR;
}

ZunResult MidiDevice::Close()
{
    if (handle == NULL)
    {
        return ZUN_ERROR;
    }

    midiOutReset(handle);
    midiOutClose(handle);
    handle = NULL;
    return ZUN_SUCCESS;
}

BOOL MidiDevice::SendLongMsg(LPMIDIHDR header)
{
    if (handle == NULL)
    {
        return FALSE;
    }

    if (midiOutPrepareHeader(handle, header, sizeof(*header)) != MMSYSERR_NOERROR)
    {
        return TRUE;
    }

    return midiOutLongMsg(handle, header, sizeof(*header)) != MMSYSERR_NOERROR;
}

union MidiShortMsg
{
    struct
    {
        u8 midiStatus;
        i8 firstByte;
        i8 secondByte;
        i8 unused;
    } msg;
    DWORD value;
};

BOOL MidiDevice::SendShortMsg(u8 midiStatus, u8 firstByte, u8 secondByte)
{
    MidiShortMsg packet;

    if (handle == NULL)
    {
        return FALSE;
    }

    packet.msg.midiStatus = midiStatus;
    packet.msg.firstByte = firstByte;
    packet.msg.secondByte = secondByte;
    return midiOutShortMsg(handle, packet.value) != MMSYSERR_NOERROR;
}

MidiTimer::MidiTimer()
{
    timeGetDevCaps(&timeCaps, sizeof(TIMECAPS));
    timerId = 0;
}

MidiTimer::~MidiTimer()
{
    StopTimer();
    timeEndPeriod(timeCaps.wPeriodMin);
}

void CALLBACK MidiTimer::DefaultTimerCallback(UINT timerId, UINT message, DWORD_PTR user, DWORD_PTR dw1, DWORD_PTR dw2)
{
    MidiTimer *timer = (MidiTimer *)user;
    timer->OnTimerElapsed();
}

MidiOutput::~MidiOutput()
{
    StopPlayback();
    ClearTracks();
    for (i32 index = 0; index < 32; index++)
    {
        ReleaseFileData(index);
    }
}

ZunResult MidiOutput::LoadFile(char *midiPath)
{
    if (ReadFileData(0x1F, midiPath) != ZUN_SUCCESS)
    {
        return ZUN_ERROR;
    }

    ParseFile(0x1F);
    ReleaseFileData(0x1F);
    return ZUN_SUCCESS;
}

ZunResult MidiOutput::SetFadeOut(u32 milliseconds)
{
    fadeOutVolumeMultiplier = 0.0f;
    fadeOutInterval = milliseconds;
    fadeOutElapsedMS = 0;
    unk2dc = 0;
    fadeOutFlag = 1;
    return ZUN_SUCCESS;
}
} // namespace th07
