#pragma once

#include <dsound.h>
#include <windows.h>

namespace th07
{
class CWaveFile;

class CSoundManager
{
  protected:
    LPDIRECTSOUND8 m_pDS;

  public:
    CSoundManager();
    ~CSoundManager();

    HRESULT Initialize(HWND hWnd, DWORD dwCoopLevel, DWORD dwPrimaryChannels, DWORD dwPrimaryFreq,
                       DWORD dwPrimaryBitRate);
    HRESULT SetPrimaryBufferFormat(DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate);
};

class CSound
{
  protected:
    // Observed in the target methods in this lane: offsets +0x04 through +0x10.
    LPDIRECTSOUNDBUFFER *m_apDSBuffer;
    DWORD m_dwDSBufferSize;
    CWaveFile *m_pWaveFile;
    DWORD m_dwNumBuffers;

  public:
    virtual ~CSound();

    HRESULT RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL *pbWasRestored);
    LPDIRECTSOUNDBUFFER GetFreeBuffer();
    LPDIRECTSOUNDBUFFER GetBuffer(DWORD dwIndex);
    HRESULT Reset();
};

class CStreamingSound : public CSound
{
  public:
    ~CStreamingSound();
};
} // namespace th07
