#include "Thread.hpp"

#include <cstring>

Thread::Thread() : m_exit(false)
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
    pthread_create(&m_thr, &m_attr, Thread::thread_run, this);
}

void Thread::join()
{
    int detstate;

    pthread_attr_getdetachstate(&m_attr, &detstate);

    if (detstate != PTHREAD_CREATE_JOINABLE) return;

    pthread_join(m_thr, 0);
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
