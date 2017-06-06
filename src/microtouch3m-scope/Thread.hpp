#ifndef MICROTOUCH3M_SCOPE_THREAD_HPP
#define MICROTOUCH3M_SCOPE_THREAD_HPP

#include <string>

#include <pthread.h>

#include "Mutex.hpp"

class Thread
{
public:
    explicit Thread(const std::string &name = "");
    virtual ~Thread();

    void start();
    void join();
    void exit();
    bool done();

protected:
    virtual bool run() = 0;

    void set_exit(bool exit);
    bool get_exit();
    void set_done(bool done);

private:
    pthread_attr_t m_attr;
    pthread_t m_thr;
    bool m_exit;
    bool m_done;
    Mutex m_mut_exit, m_mut_done;
    bool m_started, m_joined;
    std::string m_name;

    static void *thread_run(void *arg);
};

#endif // MICROTOUCH3M_SCOPE_THREAD_HPP
