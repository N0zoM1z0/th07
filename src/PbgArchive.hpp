#pragma once

#include "PbgFile.hpp"

namespace th07
{
// Target 0x0045F6B0 clears exactly these four archive-owner fields.  Their
// roles are corroborated by the immediately following TH07 archive methods.
struct PbgArchiveEntry;

class PbgArchive
{
  public:
    PbgArchive();

  private:
    PbgArchiveEntry *m_entries;
    i32 m_entryCount;
    char *m_filename;
    CPbgFile *m_file;
};

typedef char PbgArchiveSizeMustBe0x10[(sizeof(PbgArchive) == 0x10) ? 1 : -1];
} // namespace th07
