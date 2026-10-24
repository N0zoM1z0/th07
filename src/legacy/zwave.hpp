#pragma once

#include <dsound.h>
#include <mmsystem.h>
#include <windows.h>

namespace th07
{
struct ThBgmFormat
{
    char name[16];
    LONG startOffset;
    DWORD preloadAllocSize;
    LONG introLength;
    LONG totalLength;
    WAVEFORMATEX format;
};

class CWaveFile
{
  public:
    HMMIO m_hmmio;
    MMCKINFO m_ck;
    MMCKINFO m_ckRiff;
    DWORD m_dwSize;
    MMIOINFO m_mmioinfoOut;
    DWORD m_dwFlags;
    BOOL m_bIsReadingFromMemory;
    BYTE *m_pbData;
    BYTE *m_pbDataCur;
    ULONG m_ulDataSize;
    HANDLE m_hWaveFile;
    ThBgmFormat *m_pzwf;

    ~CWaveFile();
    HRESULT Close();
    HRESULT Read(BYTE *pBuffer, DWORD dwSizeToRead, DWORD *pdwSizeRead);
    HRESULT ResetFile(bool bLoop);
};

class CSoundManager
{
    friend class CStreamingSound;

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
    LONG m_iCurFadeProgress;
    LONG m_iTotalFade;
    DWORD m_iFadeType;
    DWORD m_dwPriority;
    DWORD m_dwFlags;
    DWORD m_unk28;
    DWORD m_unk2C;
    BOOL m_bIsPlaying;
    DSBUFFERDESC m_dsbd;
    CSoundManager *m_pSoundManager;

  public:
    CSound(LPDIRECTSOUNDBUFFER *apDSBuffer, DWORD dwDSBufferSize, DWORD dwNumBuffers, CWaveFile *pWaveFile);
    virtual ~CSound();

    HRESULT FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB, BOOL bRepeatWavIfBufferLarger);
    HRESULT RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL *pbWasRestored);
    LPDIRECTSOUNDBUFFER GetFreeBuffer();
    LPDIRECTSOUNDBUFFER GetBuffer(DWORD dwIndex);
    HRESULT Stop();
    HRESULT Pause();
    HRESULT Unpause();
    HRESULT Reset();
};

class CStreamingSound : public CSound
{
  protected:
    DWORD m_dwLastPlayPos;
    DWORD m_dwPlayProgress;
    DWORD m_dwNextWriteOffset;
    BOOL m_bFillNextNotificationWithSilence;

  public:
    DWORD m_dwNotifySize;
    HANDLE m_hNotifyEvent;

  public:
    CStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer, DWORD dwDSBufferSize, CWaveFile *pWaveFile,
                    DWORD dwNotifySize);
    ~CStreamingSound();

    HRESULT InitSoundBuffers();
    HRESULT UpdateFadeOut();
    HRESULT Reset();
};
} // namespace th07
