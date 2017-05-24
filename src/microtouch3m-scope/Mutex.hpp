#ifndef MICROTOUCH3M_SCOPE_MUTEX_HPP
#define MICROTOUCH3M_SCOPE_MUTEX_HPP

#include <pthread.h>

class Mutex
{
public:
    Mutex();
    virtual ~Mutex();

    void lock();
    void unlock();

private:
    pthread_mutex_t m_mut;
};

class MutexLock
{
public:
    explicit MutexLock(Mutex *m);
    virtual ~MutexLock();

private:
    Mutex *m_mutex;
};

#endif // MICROTOUCH3M_SCOPE_MUTEX_HPP
