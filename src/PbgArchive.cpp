#include "PbgArchive.hpp"

namespace th07
{
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
} // namespace th07
