#include "zwave.hpp"

namespace th07
{
// Target 0x0045E4B0 owns the file handle only when the target-observed mode
// word is one. CloseHandle's BOOL is intentionally ignored; this method's
// HRESULT contract is the target's constant S_OK return.
HRESULT CWaveFile::Close()
{
    if (m_dwFlags == 1)
    {
        CloseHandle(m_hWaveFile);
        m_hWaveFile = INVALID_HANDLE_VALUE;
    }

    return S_OK;
}
} // namespace th07
