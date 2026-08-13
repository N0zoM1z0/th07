#pragma once

#include "inttypes.hpp"

#include <windows.h>

namespace th07
{
// The target IPbgFile table at 0x0049526C has seven file operations before its
// virtual destructor.  CPbgFile's target implementations establish this ABI
// order and the return widths below.
class IPbgFile
{
  public:
    virtual bool Open(const char *path, const char *mode) = 0;
    virtual void Close() = 0;
    virtual u32 Read(void *buffer, u32 size) = 0;
    virtual bool Write(const void *buffer, u32 size) = 0;
    virtual u32 Tell() = 0;
    virtual u32 GetSize() = 0;
    virtual bool Seek(i32 offset, u32 origin) = 0;
    virtual ~IPbgFile()
    {
    }
};

class CPbgFile : public IPbgFile
{
  public:
    CPbgFile();

    virtual bool Open(const char *path, const char *mode);
    virtual void Close();
    virtual u32 Read(void *buffer, u32 size);
    virtual bool Write(const void *buffer, u32 size);
    virtual u32 Tell();
    virtual u32 GetSize();
    virtual bool Seek(i32 offset, u32 origin);
    virtual ~CPbgFile();
    virtual void *ReadWholeFile(u32 maximumSize);

    // Target 0x0045E9D0 receives its two arguments in ECX/EDX and is called
    // by Open after its mode parser accepts a file mode.  It turns a relative
    // path into one rooted at the executable's directory.
    static void __fastcall GetFullFilePath(char *buffer, const char *filename);

    HANDLE m_handle;
    u32 m_mode;
};

typedef char CPbgFileSizeMustBe0xC[(sizeof(CPbgFile) == 0xC) ? 1 : -1];
} // namespace th07
