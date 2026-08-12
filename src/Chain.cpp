#include "Chain.hpp"

#include <new>

namespace th07
{

Chain::~Chain()
{
}

ChainElem::ChainElem()
{
    prev = NULL;
    next = NULL;
    callback = NULL;
    unknown = this;
    addedCallback = NULL;
    deletedCallback = NULL;
    priority = 0;
    isHeapAllocated = false;
}

ChainElem::~ChainElem()
{
    if (deletedCallback != NULL)
    {
        deletedCallback(argument);
    }
    prev = NULL;
    next = NULL;
    callback = NULL;
    addedCallback = NULL;
    deletedCallback = NULL;
}

Chain::Chain()
{
}

i32 Chain::AddToCalcChain(ChainElem *element, i32 priority)
{
    ChainElem *current;

    current = &calcChain;
    element->priority = (i16)priority;
    while (current->next != NULL)
    {
        if (current->priority > priority)
        {
            break;
        }
        current = current->next;
    }
    if (current->priority > priority)
    {
        element->next = current;
        element->prev = current->prev;
        if (element->prev != NULL)
        {
            element->prev->next = element;
        }
        current->prev = element;
    }
    else
    {
        element->next = NULL;
        element->prev = current;
        current->next = element;
    }
    if (element->addedCallback != NULL)
    {
        i32 result = element->addedCallback(element->argument);
        element->addedCallback = NULL;
        return result;
    }
    return 0;
}

i32 Chain::AddToDrawChain(ChainElem *element, i32 priority)
{
    ChainElem *current;

    current = &drawChain;
    element->priority = (i16)priority;
    while (current->next != NULL)
    {
        if (current->priority > priority)
        {
            break;
        }
        current = current->next;
    }
    if (current->priority > priority)
    {
        element->next = current;
        element->prev = current->prev;
        if (element->prev != NULL)
        {
            element->prev->next = element;
        }
        current->prev = element;
    }
    else
    {
        element->next = NULL;
        element->prev = current;
        current->next = element;
    }
    if (element->addedCallback != NULL)
    {
        return element->addedCallback(element->argument);
    }
    return 0;
}

i32 Chain::RunCalcChain()
{
    ChainElem *removed;
    ChainElem *current;
    i32 updatedCount;

restart:
    updatedCount = 0;
    current = &calcChain;
    while (current != NULL)
    {
        if (current->callback != NULL)
        {
        execute_again:
            switch (current->callback(current->argument))
            {
            case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
                removed = current;
                current = current->next;
                Cut(removed);
                ++updatedCount;
                continue;
            case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
                goto execute_again;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
                return 0;
            case CHAIN_CALLBACK_RESULT_BREAK:
                return 1;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
                return -1;
            case CHAIN_CALLBACK_RESULT_RESTART_FROM_FIRST_JOB:
                goto restart;
            default:
                break;
            }
            ++updatedCount;
        }
        current = current->next;
    }
    return updatedCount;
}

i32 Chain::RunDrawChain()
{
    ChainElem *removed;
    ChainElem *current;
    i32 updatedCount;

    updatedCount = 0;
    current = &drawChain;
    while (current != NULL)
    {
        if (current->callback != NULL)
        {
        execute_again:
            switch (current->callback(current->argument))
            {
            case CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB:
                removed = current;
                current = current->next;
                Cut(removed);
                ++updatedCount;
                continue;
            case CHAIN_CALLBACK_RESULT_EXECUTE_AGAIN:
                goto execute_again;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_SUCCESS:
                return 0;
            case CHAIN_CALLBACK_RESULT_BREAK:
                return 1;
            case CHAIN_CALLBACK_RESULT_EXIT_GAME_ERROR:
                return -1;
            default:
                break;
            }
            ++updatedCount;
        }
        current = current->next;
    }
    return updatedCount;
}

void Chain::ReleaseSingleChain(ChainElem *root)
{
    ChainElem temporaryRoot;
    ChainElem *current;
    ChainElem *temporary;
    ChainElem *next;

    temporary = new ChainElem();
    temporaryRoot.next = temporary;
    current = root;
    while (current != NULL)
    {
        temporary->unknown = current;
        temporary->next = new ChainElem();
        temporary = temporary->next;
        current = current->next;
    }
    current = &temporaryRoot;
    while (current != NULL)
    {
        Cut(current->unknown);
        current = current->next;
    }
    temporary = temporaryRoot.next;
    while (temporary != NULL)
    {
        next = temporary->next;
        delete temporary;
        temporary = NULL;
        temporary = next;
    }
}

void Chain::Release()
{
    ReleaseSingleChain(&calcChain);
    ReleaseSingleChain(&drawChain);
}

ChainElem *Chain::CreateElem(ChainCallback callback)
{
    ChainElem *element;

    element = new ChainElem();
    element->callback = callback;
    element->addedCallback = NULL;
    element->deletedCallback = NULL;
    element->isHeapAllocated = true;
    return element;
}

void Chain::Cut(ChainElem *element)
{
    i32 isDrawChain;
    ChainElem *current;

    isDrawChain = 0;
    if (element == NULL)
    {
        return;
    }
    current = &calcChain;
    while (current != NULL)
    {
        if (current == element)
        {
            goto destroy_element;
        }
        current = current->next;
    }
    isDrawChain = 1;
    current = &drawChain;
    while (current != NULL)
    {
        if (current == element)
        {
            goto destroy_element;
        }
        current = current->next;
    }
    return;

destroy_element:
    if (element->prev != NULL)
    {
        element->callback = NULL;
        element->prev->next = element->next;
        if (element->next != NULL)
        {
            element->next->prev = element->prev;
        }
        element->prev = NULL;
        element->next = NULL;
        if (element->isHeapAllocated)
        {
            delete element;
            element = NULL;
        }
        else if (element->deletedCallback != NULL)
        {
            ChainLifetimeCallback callback = element->deletedCallback;
            element->deletedCallback = NULL;
            callback(element->argument);
        }
    }
    (void)isDrawChain;
}

} // namespace th07
