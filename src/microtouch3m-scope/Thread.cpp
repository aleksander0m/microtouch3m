#include "Thread.hpp"

Thread::Thread() : m_exit(false)
{
}

Thread::~Thread()
{
    exit();
    join();
}

void Thread::start()
{
    pthread_create(&m_thr, 0, Thread::thread_run, this);
}

void Thread::join()
{
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
