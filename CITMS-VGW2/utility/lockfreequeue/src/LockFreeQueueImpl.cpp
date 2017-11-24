#include "LockFreeQueueImpl.h"

CLockFreeQueueImpl::CLockFreeQueueImpl(void)
{
    fifoinit(&_f);
}

CLockFreeQueueImpl::~CLockFreeQueueImpl(void)
{
}

unsigned long CLockFreeQueueImpl::size(void)
{
    return fifosize(&_f);
}

void CLockFreeQueueImpl::enqueue(fifocell* cl)
{
    fifoput(&_f, cl);
}

fifocell* CLockFreeQueueImpl::dequeue(void)
{
    return fifoget(&_f);
}
