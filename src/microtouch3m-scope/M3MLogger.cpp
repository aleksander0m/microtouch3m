#include "M3MLogger.hpp"

#include <iostream>

M3MLogger::M3MLogger()
{
}

M3MLogger::~M3MLogger()
{
}

void M3MLogger::log_handler(pthread_t thread_id, const char *message)
{
    std::ios::fmtflags f(std::cout.flags());
    std::cout << "microtouch3m_log: (" << std::showbase << std::hex << thread_id << std::dec << std::noshowbase << ") "
              << std::string(message) << std::endl;
    std::cout.flags(f);
}

void M3MLogger::enable(bool e)
{
    if (e)
    {
        microtouch3m_log_set_handler(log_handler);
    }
    else
    {
        microtouch3m_log_set_handler(0);
    }
}
