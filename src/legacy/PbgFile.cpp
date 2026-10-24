#include "PbgFile.hpp"

namespace th07
{
// Target 0x0045E8F0 loads this target-pinned zero origin before both restore
// seeks; its storage is in the read-only PBG constants immediately after the
// archive error strings.
extern u32 g_TargetPbgSeekBegin;

// Target 0x0045E550 first establishes the IPbgFile construction vptr, then
// the CPbgFile vptr, INVALID_HANDLE_VALUE, and the unopened mode value.
CPbgFile::CPbgFile() : m_handle(INVALID_HANDLE_VALUE), m_mode(0)
{
}

CPbgFile::~CPbgFile()
{
    Close();
}

#pragma var_order(curMode, goToEnd, filePathBuffer, creationDisposition)
bool CPbgFile::Open(const char *filename, const char *mode)
{
    DWORD creationDisposition;
    BOOL goToEnd = FALSE;
    char filePathBuffer[MAX_PATH];

    Close();

    const char *curMode;
    for (curMode = mode; *curMode != '\0'; curMode++)
    {
        if (*curMode == 'r')
        {
            m_mode = GENERIC_READ;
            creationDisposition = OPEN_EXISTING;
            break;
        }
        if (*curMode == 'w')
        {
            DeleteFileA(filename);
            m_mode = GENERIC_WRITE;
            creationDisposition = CREATE_ALWAYS;
            break;
        }
        if (*curMode == 'a')
        {
            goToEnd = TRUE;
            m_mode = GENERIC_WRITE;
            creationDisposition = OPEN_ALWAYS;
            break;
        }
    }

    if (*curMode == '\0')
    {
        return false;
    }

    GetFullFilePath(filePathBuffer, filename);
    m_handle = CreateFileA(filePathBuffer, m_mode, FILE_SHARE_READ, NULL, creationDisposition,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (m_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    if (goToEnd)
    {
        SetFilePointer(m_handle, 0, NULL, FILE_END);
    }
    return true;
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

u32 CPbgFile::Read(void *buffer, u32 size)
{
    DWORD bytesRead = 0;
    if (m_mode != GENERIC_READ)
    {
        return 0;
    }
    ReadFile(m_handle, buffer, size, &bytesRead, NULL);
    return bytesRead;
}

bool CPbgFile::Write(const void *buffer, u32 size)
{
    DWORD bytesWritten = 0;
    if (m_mode != GENERIC_WRITE)
    {
        return false;
    }
    WriteFile(m_handle, buffer, size, &bytesWritten, NULL);
    return size == bytesWritten ? true : false;
}

#pragma var_order(memory, fileSize, previousOffset)
void *CPbgFile::ReadWholeFile(u32 maximumSize)
{
    u32 fileSize;
    i32 previousOffset;
    HGLOBAL memory;

    if (m_mode != GENERIC_READ)
    {
        return NULL;
    }
    fileSize = GetSize();
    if (fileSize > maximumSize)
    {
        return NULL;
    }
    memory = GlobalAlloc(GMEM_ZEROINIT, fileSize);
    if (memory == NULL)
    {
        return NULL;
    }
    previousOffset = Tell();
    if (!Seek(previousOffset, g_TargetPbgSeekBegin))
    {
        return NULL;
    }
    if (!Read(memory, fileSize))
    {
        if (memory != NULL)
        {
            GlobalFree(memory);
            memory = NULL;
        }
        return NULL;
    }
    Seek(previousOffset, g_TargetPbgSeekBegin);
    return memory;
}

void __fastcall CPbgFile::GetFullFilePath(char *buffer, const char *filename)
{
    if (strchr(filename, ':') != NULL)
    {
        strcpy(buffer, filename);
    }
    else
    {
        GetModuleFileNameA(NULL, buffer, MAX_PATH);

        char *endOfModulePath = strrchr(buffer, '\\');
        if (endOfModulePath == NULL)
        {
            strcpy(buffer, "");
        }

        endOfModulePath[1] = '\0';
        strcat(buffer, filename);
    }
}
} // namespace th07
