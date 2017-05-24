#include "Mutex.hpp"

Mutex::Mutex()
{
    pthread_mutex_init(&m_mut, 0);
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&m_mut);
}

void Mutex::lock()
{
    pthread_mutex_lock(&m_mut);
}

void Mutex::unlock()
{
    pthread_mutex_unlock(&m_mut);
}

MutexLock::MutexLock(Mutex *m) : m_mutex(m)
{
    m_mutex->lock();
}

MutexLock::~MutexLock()
{
    m_mutex->unlock();
}
