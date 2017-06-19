/*
 * microtouch3m-scope - Graphical tool for monitoring MicroTouch 3M touchscreen scope
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA.
 *
 * Copyright (C) 2017 Zodiac Inflight Innovations
 * Copyright (C) 2017 Sergey Zhuravlevich
 */

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
