#ifndef MICROTOUCH3M_SCOPE_M3MLOGGER_HPP
#define MICROTOUCH3M_SCOPE_M3MLOGGER_HPP

#include <pthread.h>

#include "microtouch3m.h"

class M3MLogger
{
public:
    M3MLogger();
    virtual ~M3MLogger();

    void enable(bool e);

private:
    static void log_handler(pthread_t thread_id, const char *message);
};

#endif // MICROTOUCH3M_SCOPE_M3MLOGGER_HPP
