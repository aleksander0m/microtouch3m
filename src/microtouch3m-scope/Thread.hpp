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
