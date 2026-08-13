#include "PbgFile.hpp"

namespace th07
{
// Target 0x0045E550 first establishes the IPbgFile construction vptr, then
// the CPbgFile vptr, INVALID_HANDLE_VALUE, and the unopened mode value.
CPbgFile::CPbgFile() : m_handle(INVALID_HANDLE_VALUE), m_mode(0)
{
}

// The target close path releases only an open OS handle and restores the
// constructor's unopened state.
void CPbgFile::Close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        m_mode = 0;
    }
}

u32 CPbgFile::Tell()
{
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    return SetFilePointer(m_handle, 0, NULL, FILE_CURRENT);
}

u32 CPbgFile::GetSize()
{
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    return GetFileSize(m_handle, NULL);
}

bool CPbgFile::Seek(i32 offset, u32 origin)
{
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    SetFilePointer(m_handle, offset, NULL, origin);
    return true;
}
} // namespace th07
