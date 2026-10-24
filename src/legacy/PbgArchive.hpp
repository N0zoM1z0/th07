#pragma once

#include "PbgFile.hpp"

namespace th07
{
// Target 0x0045F6B0 clears exactly these four archive-owner fields.  Their
// roles are corroborated by the immediately following TH07 archive methods.
struct PbgArchiveEntry
{
    // Target destructor 0x0045F680 only owns the filename allocation; the
    // following fields agree with target-side archive entry consumers.
    HGLOBAL filename;
    u32 dataOffset;
    u32 decompressedSize;
    u32 unknown;

    PbgArchiveEntry()
    {
        filename = NULL;
    }

    ~PbgArchiveEntry()
    {
        if (filename != NULL)
        {
            GlobalFree(filename);
            filename = NULL;
        }
    }
};

typedef char PbgArchiveEntrySizeMustBe0x10[(sizeof(PbgArchiveEntry) == 0x10) ? 1 : -1];

class PbgArchive
{
  public:
    PbgArchive();
    ~PbgArchive();
    bool Load(const char *filename);
    void Release();
    u32 GetEntryDecompressedSize(const char *filename);
    PbgArchiveEntry *FindEntry(const char *filename);
    char *CopyFileName(const char *filename);
    bool ParseHeader(const char *filename);
    PbgArchiveEntry *AllocEntries(void *entryBuffer, i32 count, u32 dataOffset);
    void *ReadDecompressEntry(const char *filename, void *outBuffer);

    static inline i32 SeekPastInt(void **cursor)
    {
        *cursor = (i32 *)*cursor + 1;
        return *(i32 *)*cursor;
    }

    static inline void *SeekPastString(void **cursor)
    {
        *cursor = (char *)*cursor + strlen((char *)*cursor) + 1;
        return *cursor;
    }

  private:
    PbgArchiveEntry *m_entries;
    i32 m_entryCount;
    char *m_filename;
    IPbgFile *m_file;
};

typedef char PbgArchiveSizeMustBe0x10[(sizeof(PbgArchive) == 0x10) ? 1 : -1];
} // namespace th07
