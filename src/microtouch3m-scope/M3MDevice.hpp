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
};

class M3MDeviceMonitorThread : public Thread
{
public:
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

    bool pop_signal(signal_t &sig);
    signal_t strays();

private:

    void push_signal(const signal_t &sig);
    void set_strays(const signal_t &sig);
    virtual bool run();

    static bool monitor_async_reports_callback(microtouch3m_device_t *dev, microtouch3m_status_t status, int32_t ul_i,
                                               int32_t ul_q, int32_t ur_i, int32_t ur_q, int32_t ll_i, int32_t ll_q,
                                               int32_t lr_i, int32_t lr_q, void *user_data);

    M3MDevice *m_m3m_dev;
    std::queue<signal_t> m_signals;
    Mutex m_mut_signals;
    timespec m_strays_update_time;
    Mutex m_mut_strays;
    signal_t m_strays;
};

#endif // MICROTOUCH3M_SCOPE_M3MDEVICE_HPP
