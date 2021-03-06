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

#ifndef MICROTOUCH3M_SCOPE_M3MDEVICE_HPP
#define MICROTOUCH3M_SCOPE_M3MDEVICE_HPP

#include <iostream>
#include <queue>

#include <time.h>

#include "microtouch3m.h"

#include "Mutex.hpp"
#include "Thread.hpp"

class M3MContext
{
public:
    M3MContext();
    virtual ~M3MContext();

    microtouch3m_context_t *context() const;

private:
    microtouch3m_context_t *m_ctx;
};

class M3MDevice
{
    friend class M3MDeviceMonitorThread;

public:
    M3MDevice();
    virtual ~M3MDevice();

    void open();
    void print_info();
    void read_strays();
    void get_fw_version(int *major, int *minor);
    std::string get_frequency_string();
    void get_sensitivity_info();
    uint8_t sensitivity_level() const;
    uint8_t touchdown() const;
    uint8_t liftoff() const;
    uint8_t palm() const;
    uint8_t stray() const;
    uint8_t stray_alpha() const;
    void monitor_async_reports(microtouch3m_device_async_report_scope_f *callback, void *user_data);

private:
    M3MContext m_ctx;
    microtouch3m_device_t *m_dev;

    uint64_t m_ul_stray_signal;
    uint64_t m_ur_stray_signal;
    uint64_t m_ll_stray_signal;
    uint64_t m_lr_stray_signal;
    int m_fw_major;
    int m_fw_minor;
    std::string m_frequency_str;
    uint8_t m_sensitivity_level;
    uint8_t m_touchdown, m_liftoff, m_palm, m_stray, m_stray_alpha;
};

class M3MDeviceMonitorThread : public Thread
{
public:
    M3MDeviceMonitorThread();
    virtual ~M3MDeviceMonitorThread();

    struct signal_t
    {
        signal_t() : ul(0), ur(0), ll(0), lr(0)
        {}

        signal_t(int64_t ul, int64_t ur, int64_t ll, int64_t lr) : ul(ul), ur(ur), ll(ll), lr(lr)
        {}

        bool operator==(const signal_t &sig)
        {
            return this->ul == sig.ul
                   && this->ur == sig.ur
                   && this->ll == sig.ll
                   && this->lr == sig.lr;
        }

        bool operator!=(const signal_t &sig)
        {
            return !(*this == sig);
        }

        int64_t ul;
        int64_t ur;
        int64_t ll;
        int64_t lr;
    };

    std::queue<signal_t> *get_signals_r();
    signal_t get_strays();

private:

    void push_signal(const signal_t &sig);
    void set_strays(const signal_t &sig);
    virtual bool run();

    static bool monitor_async_reports_callback(microtouch3m_device_t *dev, microtouch3m_status_t status, int32_t ul_i,
                                               int32_t ul_q, int32_t ur_i, int32_t ur_q, int32_t ll_i, int32_t ll_q,
                                               int32_t lr_i, int32_t lr_q, void *user_data);

    M3MDevice *m_m3m_dev;
    std::queue<signal_t> m_signals0, m_signals1, *m_signals_r, *m_signals_w;
    Mutex m_mut_signals;
    timespec m_strays_update_time;
    Mutex m_mut_strays;
    signal_t m_strays;
    int m_callback_failures;
};

#endif // MICROTOUCH3M_SCOPE_M3MDEVICE_HPP
