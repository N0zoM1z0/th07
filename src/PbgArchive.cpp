#include "PbgArchive.hpp"

namespace th07
{
PbgArchiveEntry::~PbgArchiveEntry()
{
    if (filename != NULL)
    {
        GlobalFree(filename);
        filename = NULL;
    }
}

PbgArchive::PbgArchive()
    : m_entries(NULL), m_entryCount(0), m_filename(NULL), m_file(NULL)
{
}
} // namespace th07
