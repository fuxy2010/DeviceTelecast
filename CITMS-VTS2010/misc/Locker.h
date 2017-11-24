#ifndef _LOCKER_H_
#define _LOCKER_H_

#ifdef _WIN32 // Windows version
#include <windows.h>
#else // Linux version
#include <pthread.h>
#endif

class CSSMutex
{
public:
    CSSMutex()
    {
#ifdef _WIN32 // Windows version
        InitializeCriticalSection(&_mutex);
#else // Linux version
        pthread_mutex_init(&_mutex, NULL); 
#endif
    };

    ~CSSMutex()
    {
#ifdef _WIN32 // Windows version
        DeleteCriticalSection(&_mutex);
#else // Linux version
        pthread_mutex_destroy(&_mutex); 
#endif
    };

    void lock() const
    {
#ifdef _WIN32 // Windows version
        EnterCriticalSection(&_mutex);
#else // Linux version
        pthread_mutex_lock(&_mutex); 
#endif
    };

    void unlock() const
    {
#ifdef _WIN32 // Windows version
        LeaveCriticalSection(&_mutex);
#else // Linux version
        pthread_mutex_unlock(&_mutex); 
#endif
    };

private:
#ifdef _WIN32 // Windows version
    mutable CRITICAL_SECTION _mutex;
#else // Linux version
    pthread_mutex_t _mutex;
#endif
};

class CSSLocker
{
public:
    CSSLocker(CSSMutex * mutex_ptr)
    {
        _mutex_ptr = mutex_ptr;
        if (_mutex_ptr)
        {
            _mutex_ptr->lock();
        }
    };

    ~CSSLocker()
    {
        if (_mutex_ptr)
        {
            _mutex_ptr->unlock();
        }
    };

private:
    CSSLocker(const CSSLocker&);
    CSSLocker& operator=(const CSSLocker&);

private:
    CSSMutex * _mutex_ptr;
};

#endif // _LOCKER_H_