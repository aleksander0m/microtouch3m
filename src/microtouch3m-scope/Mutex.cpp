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

#include "Mutex.hpp"

#include <stdexcept>
#include <cstring>

Mutex::Mutex()
{
    pthread_mutex_init(&m_mut, 0);
}

Mutex::~Mutex()
{
    lock();
    unlock();

    int error;
    if ((error = pthread_mutex_destroy(&m_mut)))
    {
        throw std::runtime_error("Couldn't destroy mutex: " + std::string(strerror(error)));
    }
}

void Mutex::lock()
{
    int error;
    if ((error = pthread_mutex_lock(&m_mut)))
    {
        throw std::runtime_error("Couldn't lock mutex: " + std::string(strerror(error)));
    }
}

void Mutex::unlock()
{
    int error;
    if ((error = pthread_mutex_unlock(&m_mut)))
    {
        throw std::runtime_error("Couldn't unlock mutex: " + std::string(strerror(error)));
    }
}

MutexLock::MutexLock(Mutex *m) : m_mutex(m)
{
    m_mutex->lock();
}

MutexLock::~MutexLock()
{
    m_mutex->unlock();
}
