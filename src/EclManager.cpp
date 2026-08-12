#include "EclManager.hpp"

#include <stdlib.h>

namespace th07
{

void EclManager::Unload()
{
    EclRawHeader *file;

    if (eclFile != NULL)
    {
        file = eclFile;
        free(file);
    }
    eclFile = NULL;
}

ZunResult EclManager::CallEclSub(EnemyEclContext *context, i16 subId)
{
    context->currentInstr = subTable[subId];
    context->time.InitializeForPopup();
    context->unknown80.InitializeForPopup();
    context->subId = subId;
    return ZUN_SUCCESS;
}

} // namespace th07
