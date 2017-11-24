#ifndef __lfqueueimpl__
#define __lfqueueimpl__

#include "lffifo.h"

class CLockFreeQueueImpl
{
public:
    CLockFreeQueueImpl(void);
    virtual ~CLockFreeQueueImpl(void);

    unsigned long size(void);	
    void enqueue(fifocell* cl);
    fifocell* dequeue(void);

private:
    fifo _f;
};


#endif