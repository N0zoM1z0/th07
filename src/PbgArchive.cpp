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

#pragma var_order(entryBuffer, decompressedSize, magic, size, fileTableOffset, fileTableBuffer)
bool PbgArchive::ParseHeader(const char *filename)
{
    HGLOBAL entryBuffer;
    u32 decompressedSize;
    i32 magic;
    u32 size;
    u32 fileTableOffset;
    HGLOBAL fileTableBuffer;

    fileTableBuffer = NULL;
    entryBuffer = NULL;

    if (m_file == NULL)
    {
        return false;
    }
    if (!m_file->Open(filename, g_TargetPbgOpenReadMode))
    {
        goto parseError;
    }
    if (m_file->ReadInt(&magic) == 0)
    {
        goto parseError;
    }
    if (magic != 0x34474250)
    {
        goto parseError;
    }
    if (m_file->ReadInt(&m_entryCount) == 0)
    {
        goto parseError;
    }
    if (m_entryCount <= 0)
    {
        goto parseError;
    }

    size = m_file->GetSize();
    if (m_file->ReadInt((i32 *)&fileTableOffset) == 0)
    {
        goto parseError;
    }
    if (fileTableOffset >= size)
    {
        goto parseError;
    }
    size -= fileTableOffset;
    if (m_file->ReadInt((i32 *)&decompressedSize) == 0)
    {
        goto parseError;
    }

    m_file->Seek(fileTableOffset, g_TargetPbgSeekBegin);
    fileTableBuffer = GlobalAlloc(0, size);
    if (fileTableBuffer == NULL)
    {
        goto parseError;
    }
    if (m_file->Read(fileTableBuffer, size) == 0)
    {
        goto parseError;
    }

    entryBuffer = LzssDecode(fileTableBuffer, size, NULL, decompressedSize);
    if (entryBuffer == NULL)
    {
        goto parseError;
    }
    m_entries = AllocEntries(entryBuffer, m_entryCount, fileTableOffset);
    if (m_entries == NULL)
    {
        goto parseError;
    }

    if (fileTableBuffer != NULL)
    {
        GlobalFree(fileTableBuffer);
        fileTableBuffer = NULL;
    }
    if (entryBuffer != NULL)
    {
        GlobalFree(entryBuffer);
        entryBuffer = NULL;
    }
    return true;

parseError:
    if (fileTableBuffer != NULL)
    {
        GlobalFree(fileTableBuffer);
        fileTableBuffer = NULL;
    }
    if (entryBuffer != NULL)
    {
        GlobalFree(entryBuffer);
        entryBuffer = NULL;
    }
    if (m_file != NULL)
    {
        delete m_file;
        m_file = NULL;
    }
    DebugPrint("\203\164\203\100\203\103\203\213 %s \202\314\203\111\201\133\203\166\203\223\222\206\202\311\203\107\203\211\201\133\202\252\224\255\220\266\202\265\202\334\202\265\202\275\015\012", filename);
    while (false)
    {
    }
    return false;
}

#pragma var_order(entryData, index, entries)
PbgArchiveEntry *PbgArchive::AllocEntries(void *entryBuffer, i32 count, u32 dataOffset)
{
    void *entryData;
    i32 index;
    PbgArchiveEntry *entries = NULL;

    entries = new PbgArchiveEntry[count + 1]();
    if (entries == NULL)
    {
        goto allocationError;
    }

    entryData = entryBuffer;
    for (index = 0; index < count; ++index)
    {
        entries[index].filename = CopyFileName((const char *)entryData);
        SeekPastString(&entryData);
        entries[index].dataOffset = *(u32 *)entryData;
        SeekPastInt(&entryData);
        entries[index].decompressedSize = *(u32 *)entryData;
        SeekPastInt(&entryData);
        entries[index].unknown = *(u32 *)entryData;
        SeekPastInt(&entryData);
    }

    entries[count].dataOffset = dataOffset;
    entries[count].decompressedSize = 0;
    return entries;

allocationError:
    if (entries != NULL)
    {
        delete[] entries;
        entries = NULL;
    }
    return NULL;
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
