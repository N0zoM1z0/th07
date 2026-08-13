#include "PbgArchive.hpp"

namespace th07
{
// These target-owned constants are loaded through globals rather than folded
// into call sites.  Keeping that storage indirection is required by the VC7
// code shape for archive entry reads.
extern u32 g_TargetPbgSeekBegin;
extern const char *g_TargetPbgOpenReadMode;
extern void *__fastcall LzssDecode(void *compressedData, u32 compressedSize, void *outBuffer,
                                  u32 decompressedSize);

// Target 0x0045E4F0 is the build's deliberately compiled-out diagnostic
// hook.  It accepts the archive format strings but has no side effects.
void __cdecl DebugPrint(const char *format, ...)
{
}

PbgArchive::PbgArchive()
    : m_entries(NULL), m_entryCount(0), m_filename(NULL), m_file(NULL)
{
}

PbgArchive::~PbgArchive()
{
    Release();
}

bool PbgArchive::Load(const char *filename)
{
    Release();
    DebugPrint("info : %s open arcfile\r\n", filename);

    m_file = new CPbgFile();
    if (m_file == NULL)
    {
        return false;
    }

    if (ParseHeader(filename))
    {
        m_filename = CopyFileName(filename);
        if (m_filename != NULL)
        {
            return true;
        }
    }

    DebugPrint("info : %s not found\r\n", filename);
    Release();
    return false;
}

void PbgArchive::Release()
{
    DebugPrint("info : %s close arcfile\r\n", m_filename);
    if (m_filename != NULL)
    {
        GlobalFree(m_filename);
        m_filename = NULL;
    }
    if (m_entries != NULL)
    {
        delete[] m_entries;
        m_entries = NULL;
    }
    if (m_file != NULL)
    {
        delete m_file;
        m_file = NULL;
    }
    m_entryCount = 0;
}

u32 PbgArchive::GetEntryDecompressedSize(const char *filename)
{
    PbgArchiveEntry *entry = FindEntry(filename);
    if (entry != NULL)
    {
        return entry->decompressedSize;
    }
    return 0;
}

PbgArchiveEntry *PbgArchive::FindEntry(const char *filename)
{
    if (m_entries == NULL)
    {
        return NULL;
    }

    PbgArchiveEntry *entry = m_entries;
    for (i32 count = m_entryCount; count > 0; --count, ++entry)
    {
        if (_stricmp(filename, (const char *)entry->filename) == 0)
        {
            return entry;
        }
    }
    return NULL;
}

char *PbgArchive::CopyFileName(const char *filename)
{
    char *copy = (char *)GlobalAlloc(0, strlen(filename) + 1);
    if (copy != NULL)
    {
        strcpy(copy, filename);
    }
    return copy;
}

#pragma var_order(entry, decompressedSize, decompressedData, compressedData, compressedSize)
void *PbgArchive::ReadDecompressEntry(const char *filename, void *outBuffer)
{
    void *decompressedData;
    HGLOBAL compressedData = NULL;
    u32 compressedSize;
    u32 decompressedSize;

    if (m_file == NULL)
    {
        return NULL;
    }

    PbgArchiveEntry *entry = FindEntry(filename);
    if (entry == NULL)
    {
        goto entryReadError;
    }

    if (!m_file->Open(m_filename, g_TargetPbgOpenReadMode))
    {
        goto entryReadError;
    }

    compressedSize = entry[1].dataOffset - entry->dataOffset;
    decompressedSize = entry->decompressedSize;
    compressedData = GlobalAlloc(0, compressedSize);
    if (compressedData == NULL)
    {
        goto entryReadError;
    }
    if (!m_file->Seek(entry->dataOffset, g_TargetPbgSeekBegin))
    {
        goto entryReadError;
    }
    if (m_file->Read(compressedData, compressedSize) == 0)
    {
        goto entryReadError;
    }

    decompressedData = LzssDecode(compressedData, compressedSize, outBuffer, decompressedSize);
    if (compressedData != NULL)
    {
        GlobalFree(compressedData);
        compressedData = NULL;
    }
    return decompressedData;

entryReadError:
    DebugPrint("info : %s error\r\n", m_filename);
    if (compressedData != NULL)
    {
        GlobalFree(compressedData);
        compressedData = NULL;
    }
    return NULL;
}
} // namespace th07
