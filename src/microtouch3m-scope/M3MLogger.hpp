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
