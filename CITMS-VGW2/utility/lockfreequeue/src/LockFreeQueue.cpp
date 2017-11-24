#include "LockFreeQueue.h"
#include "LockFreeQueueImpl.h"

CLockFreeQueue::CLockFreeQueue(void)
{
    _impl_ptr = new CLockFreeQueueImpl();
}

CLockFreeQueue::~CLockFreeQueue(void)
{
    delete _impl_ptr;
}

unsigned long CLockFreeQueue::size(void)
{
    if (_impl_ptr)
    {
        return _impl_ptr->size();
    }

    return 0;
}

void CLockFreeQueue::enqueue(fifocell* cl)
{
    if (_impl_ptr)
    {
        _impl_ptr->enqueue(cl);
    }
}

fifocell* CLockFreeQueue::dequeue(void)
{
    if (_impl_ptr)
    {
        return _impl_ptr->dequeue();
    }

    return 0;
}
