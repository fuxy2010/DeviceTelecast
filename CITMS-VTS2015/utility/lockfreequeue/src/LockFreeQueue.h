#ifndef __lfqueue__
#define __lfqueue__

#include "lffifocell.h"

class CLockFreeQueueImpl;
class CLockFreeQueue
{
public:
    CLockFreeQueue(void);
    virtual ~CLockFreeQueue(void);

    unsigned long size(void);	
    void enqueue(fifocell* cl);
    fifocell* dequeue(void);

private:
    CLockFreeQueueImpl* _impl_ptr;
};


#endif