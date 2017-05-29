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
    std::cout << "microtouch3m_log: (0x" << std::hex << thread_id << std::dec << ") "
              << std::string(message) << std::endl;
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
