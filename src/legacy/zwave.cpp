#define STRICT

#include "zwave.hpp"

#include <stdlib.h>

namespace th07
{
CSoundManager::CSoundManager()
{
    m_pDS = NULL;
}

CSoundManager::~CSoundManager()
{
    if (m_pDS)
    {
        m_pDS->Release();
        m_pDS = NULL;
    }
}

HRESULT CSoundManager::Initialize(HWND hWnd, DWORD dwCoopLevel, DWORD dwPrimaryChannels, DWORD dwPrimaryFreq,
                                  DWORD dwPrimaryBitRate)
{
    HRESULT hr;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    if (m_pDS)
    {
        m_pDS->Release();
        m_pDS = NULL;
    }

    if (FAILED(hr = DirectSoundCreate8(NULL, &m_pDS, NULL)))
        return hr;

    if (FAILED(hr = m_pDS->SetCooperativeLevel(hWnd, dwCoopLevel)))
        return hr;

    SetPrimaryBufferFormat(dwPrimaryChannels, dwPrimaryFreq, dwPrimaryBitRate);

    return S_OK;
}

HRESULT CSoundManager::SetPrimaryBufferFormat(DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate)
{
    HRESULT hr;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    if (m_pDS == NULL)
        return CO_E_NOTINITIALIZED;

    DSBUFFERDESC dsbd;
    ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat = NULL;

    if (FAILED(hr = m_pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, NULL)))
        return hr;

    WAVEFORMATEX wfx;
    ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = (WORD)dwPrimaryChannels;
    wfx.nSamplesPerSec = dwPrimaryFreq;
    wfx.wBitsPerSample = (WORD)dwPrimaryBitRate;
    wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if (FAILED(hr = pDSBPrimary->SetFormat(&wfx)))
        return hr;

    if (pDSBPrimary)
    {
        pDSBPrimary->Release();
        pDSBPrimary = NULL;
    }

    return S_OK;
}

CSound::CSound(LPDIRECTSOUNDBUFFER *apDSBuffer, DWORD dwDSBufferSize, DWORD dwNumBuffers, CWaveFile *pWaveFile)
{
    DWORD i;

    m_apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
    for (i = 0; i < dwNumBuffers; i++)
        m_apDSBuffer[i] = apDSBuffer[i];

    m_dwDSBufferSize = dwDSBufferSize;
    m_dwNumBuffers = dwNumBuffers;
    m_pWaveFile = pWaveFile;

    FillBufferWithSound(m_apDSBuffer[0], FALSE);

    for (i = 0; i < dwNumBuffers; i++)
        m_apDSBuffer[i]->SetCurrentPosition(0);

    m_bIsPlaying = FALSE;
}

HRESULT CStreamingSound::InitSoundBuffers()
{
    DWORD i;

    m_bIsPlaying = FALSE;

    for (i = 0; i < m_dwNumBuffers; i++)
    {
        if (m_apDSBuffer[i])
        {
            m_apDSBuffer[i]->Release();
            m_apDSBuffer[i] = NULL;
        }
    }

    if (m_apDSBuffer)
    {
        delete[] m_apDSBuffer;
        m_apDSBuffer = NULL;
    }

    DSBPOSITIONNOTIFY *aPosNotify = NULL;
    LPDIRECTSOUNDNOTIFY pDSNotify = NULL;

    m_apDSBuffer = new LPDIRECTSOUNDBUFFER[m_dwNumBuffers];

    for (i = 0; i < m_dwNumBuffers; i++)
    {
        if (FAILED(m_pSoundManager->m_pDS->CreateSoundBuffer(&m_dsbd, &m_apDSBuffer[i], NULL)))
            return E_FAIL;

        if (FAILED(m_apDSBuffer[i]->QueryInterface(IID_IDirectSoundNotify, (VOID **)&pDSNotify)))
            return E_FAIL;

        aPosNotify = new DSBPOSITIONNOTIFY[16];
        if (aPosNotify == NULL)
            return E_OUTOFMEMORY;

        for (DWORD j = 0; j < 16; j++)
        {
            aPosNotify[j].dwOffset = (m_dwNotifySize * j) + m_dwNotifySize - 1;
            aPosNotify[j].hEventNotify = m_hNotifyEvent;
        }

        if (FAILED(pDSNotify->SetNotificationPositions(16, aPosNotify)))
        {
            if (pDSNotify)
            {
                pDSNotify->Release();
                pDSNotify = NULL;
            }
            if (aPosNotify)
            {
                delete[] aPosNotify;
                aPosNotify = NULL;
            }
            return E_FAIL;
        }

        if (pDSNotify)
        {
            pDSNotify->Release();
            pDSNotify = NULL;
        }
        if (aPosNotify)
        {
            delete[] aPosNotify;
            aPosNotify = NULL;
        }
    }

    return S_OK;
}

CSound::~CSound()
{
    for (DWORD i = 0; i < m_dwNumBuffers; i++)
    {
        if (m_apDSBuffer[i])
        {
            m_apDSBuffer[i]->Release();
            m_apDSBuffer[i] = NULL;
        }
    }

    if (m_apDSBuffer)
    {
        delete[] m_apDSBuffer;
        m_apDSBuffer = NULL;
    }

    if (m_pWaveFile)
    {
        delete m_pWaveFile;
        m_pWaveFile = NULL;
    }
}

HRESULT CSound::FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB, BOOL bRepeatWavIfBufferLarger)
{
    HRESULT hr;
    VOID *pDSLockedBuffer = NULL;
    DWORD dwDSLockedBufferSize = 0;
    DWORD dwWavDataRead = 0;

    if (pDSB == NULL)
        return CO_E_NOTINITIALIZED;

    if (FAILED(hr = RestoreBuffer(pDSB, NULL)))
        return hr;

    if (FAILED(hr = pDSB->Lock(0, m_dwDSBufferSize, &pDSLockedBuffer, &dwDSLockedBufferSize, NULL, NULL, 0L)))
        return hr;

    m_pWaveFile->ResetFile(false);

    if (FAILED(hr = m_pWaveFile->Read((BYTE *)pDSLockedBuffer, dwDSLockedBufferSize, &dwWavDataRead)))
        return hr;

    if (dwWavDataRead == 0)
    {
        FillMemory((BYTE *)pDSLockedBuffer, dwDSLockedBufferSize,
                   (BYTE)(m_pWaveFile->m_pzwf->format.wBitsPerSample == 8 ? 128 : 0));
    }
    else if (dwWavDataRead < dwDSLockedBufferSize)
    {
        if (bRepeatWavIfBufferLarger)
        {
            DWORD dwReadSoFar = dwWavDataRead;
            while (dwReadSoFar < dwDSLockedBufferSize)
            {
                if (FAILED(hr = m_pWaveFile->ResetFile(false)))
                    return hr;

                hr = m_pWaveFile->Read((BYTE *)pDSLockedBuffer + dwReadSoFar,
                                       dwDSLockedBufferSize - dwReadSoFar, &dwWavDataRead);
                if (FAILED(hr))
                    return hr;

                dwReadSoFar += dwWavDataRead;
            }
        }
        else
        {
            FillMemory((BYTE *)pDSLockedBuffer + dwWavDataRead, dwDSLockedBufferSize - dwWavDataRead,
                       (BYTE)(m_pWaveFile->m_pzwf->format.wBitsPerSample == 8 ? 128 : 0));
        }
    }

    pDSB->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0);

    return S_OK;
}

HRESULT CSound::RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL *pbWasRestored)
{
    HRESULT hr;

    if (pDSB == NULL)
        return CO_E_NOTINITIALIZED;
    if (pbWasRestored)
        *pbWasRestored = FALSE;

    DWORD dwStatus;
    if (FAILED(hr = pDSB->GetStatus(&dwStatus)))
        return hr;

    if (dwStatus & DSBSTATUS_BUFFERLOST)
    {
        do
        {
            hr = pDSB->Restore();
            if (hr == DSERR_BUFFERLOST)
                Sleep(10);
        } while (hr = pDSB->Restore());

        if (pbWasRestored != NULL)
            *pbWasRestored = TRUE;

        return S_OK;
    }

    return S_FALSE;
}

#pragma var_order(bIsPlaying, i)
LPDIRECTSOUNDBUFFER CSound::GetFreeBuffer()
{
    BOOL bIsPlaying = FALSE;

    if (m_apDSBuffer == NULL)
        return FALSE;

    DWORD i;
    for (i = 0; i < m_dwNumBuffers; i++)
    {
        if (m_apDSBuffer[i])
        {
            DWORD dwStatus = 0;
            m_apDSBuffer[i]->GetStatus(&dwStatus);
            if ((dwStatus & DSBSTATUS_PLAYING) == 0)
                break;
        }
    }

    if (i != m_dwNumBuffers)
        return m_apDSBuffer[i];

    return m_apDSBuffer[rand() % m_dwNumBuffers];
}

LPDIRECTSOUNDBUFFER CSound::GetBuffer(DWORD dwIndex)
{
    if (m_apDSBuffer == NULL)
        return NULL;
    if (dwIndex >= m_dwNumBuffers)
        return NULL;

    return m_apDSBuffer[dwIndex];
}

HRESULT CSound::Reset()
{
    if (m_apDSBuffer == NULL)
        return CO_E_NOTINITIALIZED;

    HRESULT hr = 0;

    for (DWORD i = 0; i < m_dwNumBuffers; i++)
        hr |= m_apDSBuffer[i]->SetCurrentPosition(0);

    return hr;
}

HRESULT CSound::Stop()
{
    if (m_apDSBuffer == NULL)
        return CO_E_NOTINITIALIZED;

    HRESULT hr = 0;

    m_bIsPlaying = FALSE;

    for (DWORD i = 0; i < m_dwNumBuffers; i++)
    {
        hr |= m_apDSBuffer[i]->Stop();
        hr |= m_apDSBuffer[i]->SetCurrentPosition(0);
    }

    m_iFadeType = 0;

    return hr;
}

HRESULT CSound::Pause()
{
    if (m_apDSBuffer == NULL)
        return CO_E_NOTINITIALIZED;

    HRESULT hr = 0;

    m_bIsPlaying = FALSE;
    hr |= m_apDSBuffer[0]->Stop();

    return hr;
}

HRESULT CSound::Unpause()
{
    if (m_apDSBuffer == NULL)
        return CO_E_NOTINITIALIZED;

    LPDIRECTSOUNDBUFFER pDSB = m_apDSBuffer[0];

    m_bIsPlaying = TRUE;

    return pDSB->Play(0, m_dwPriority, m_dwFlags);
}

CStreamingSound::CStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer, DWORD dwDSBufferSize, CWaveFile *pWaveFile,
                                 DWORD dwNotifySize)
    : CSound(&pDSBuffer, dwDSBufferSize, 1, pWaveFile)
{
    m_dwLastPlayPos = 0;
    m_dwPlayProgress = 0;
    m_dwNotifySize = dwNotifySize;
    m_dwNextWriteOffset = 0;
    m_bFillNextNotificationWithSilence = FALSE;
}

CStreamingSound::~CStreamingSound()
{
}

HRESULT CStreamingSound::UpdateFadeOut()
{
    if (m_iFadeType != 0)
    {
        m_iCurFadeProgress = m_iCurFadeProgress - 1;
        if (m_iCurFadeProgress <= 0)
        {
            m_iFadeType = 0;
            m_apDSBuffer[0]->Stop();
            return S_FALSE;
        }
        DWORD volume = (m_iCurFadeProgress * 5000) / m_iTotalFade - 5000;
        HRESULT result = m_apDSBuffer[0]->SetVolume(volume);
    }
    return S_OK;
}

HRESULT CStreamingSound::Reset()
{
    HRESULT hr;

    if (m_apDSBuffer[0] == NULL || m_pWaveFile == NULL)
        return CO_E_NOTINITIALIZED;

    m_dwLastPlayPos = 0;
    m_dwPlayProgress = 0;
    m_dwNextWriteOffset = 0;
    m_bFillNextNotificationWithSilence = FALSE;

    BOOL bRestored;
    if (FAILED(hr = RestoreBuffer(m_apDSBuffer[0], &bRestored)))
        return hr;

    if (bRestored)
    {
        if (FAILED(hr = FillBufferWithSound(m_apDSBuffer[0], FALSE)))
            return hr;
    }

    m_pWaveFile->ResetFile(false);

    return m_apDSBuffer[0]->SetCurrentPosition(0L);
}
} // namespace th07
