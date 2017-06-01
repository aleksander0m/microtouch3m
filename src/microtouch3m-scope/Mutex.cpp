#include "Mutex.hpp"

#include <stdexcept>
#include <cstring>

Mutex::Mutex()
{
    pthread_mutex_init(&m_mut, 0);
}

Mutex::~Mutex()
{
    lock();
    unlock();

    int error;
    if ((error = pthread_mutex_destroy(&m_mut)))
    {
        throw std::runtime_error("Couldn't destroy mutex: " + std::string(strerror(error)));
    }
}

void Mutex::lock()
{
    int error;
    if ((error = pthread_mutex_lock(&m_mut)))
    {
        throw std::runtime_error("Couldn't lock mutex: " + std::string(strerror(error)));
    }
}

void Mutex::unlock()
{
    int error;
    if ((error = pthread_mutex_unlock(&m_mut)))
    {
        throw std::runtime_error("Couldn't unlock mutex: " + std::string(strerror(error)));
    }
}

MutexLock::MutexLock(Mutex *m) : m_mutex(m)
{
    m_mutex->lock();
}

MutexLock::~MutexLock()
{
    m_mutex->unlock();
}
