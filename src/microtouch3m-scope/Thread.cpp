#include "Thread.hpp"

#include <cstring>
#include <stdexcept>

Thread::Thread() : m_exit(false), m_started(false), m_joined(false)
{
    memset(&m_attr, -1, sizeof(m_attr));
}

Thread::~Thread()
{
    exit();
    join();
}

void Thread::start()
{
    pthread_attr_init(&m_attr);

    int error;
    if ((error = pthread_create(&m_thr, &m_attr, Thread::thread_run, this)))
    {
        throw std::runtime_error("Couldn't create thread: " + std::string(strerror(error)));
    }

    m_started = true;
}

void Thread::join()
{
    if (!m_started || m_joined) return;

    int detstate;

    pthread_attr_getdetachstate(&m_attr, &detstate);

    if (detstate != PTHREAD_CREATE_JOINABLE) return;

    int error;
    if ((error = pthread_join(m_thr, 0)))
    {
        throw std::runtime_error("Couldn't join thread: " + std::string(strerror(error)));
    }

    m_joined = true;
}

void Thread::exit()
{
    set_exit(true);
}

void Thread::set_exit(bool exit)
{
    MutexLock lock(&m_mut_exit);
    m_exit = exit;
}

bool Thread::get_exit()
{
    MutexLock lock(&m_mut_exit);
    return m_exit;
}

void *Thread::thread_run(void *arg)
{
    Thread *thr = static_cast<Thread*>(arg);

    while (!thr->get_exit() && thr->run())
    { }

    return 0;
}
