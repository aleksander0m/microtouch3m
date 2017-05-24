#ifndef MICROTOUCH3M_SCOPE_M3MDEVICE_HPP
#define MICROTOUCH3M_SCOPE_M3MDEVICE_HPP

#include <iostream>
#include <queue>
#include "SDL_stdinc.h"

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
    void monitor_async_reports(microtouch3m_device_async_report_scope_f *callback, void *user_data);
private:
    M3MContext m_ctx;
    microtouch3m_device_t *m_dev;

    uint64_t m_ul_stray_signal;
    uint64_t m_ur_stray_signal;
    uint64_t m_ll_stray_signal;
    uint64_t m_lr_stray_signal;
};

class M3MDeviceMonitorThread : public Thread
{
public:
    struct signal_t
    {
        signal_t() :
        ul_corrected_signal(0),
        ur_corrected_signal(0),
        ll_corrected_signal(0),
        lr_corrected_signal(0)
        {}

        signal_t(int64_t ul, int64_t ur, int64_t ll, int64_t lr) :
        ul_corrected_signal(ul),
        ur_corrected_signal(ur),
        ll_corrected_signal(ll),
        lr_corrected_signal(lr)
        {}

        int64_t ul_corrected_signal;
        int64_t ur_corrected_signal;
        int64_t ll_corrected_signal;
        int64_t lr_corrected_signal;
    };

    bool pop_signal(signal_t &sig);

private:

    void push_signal(const signal_t &sig);
    virtual bool run();

    static bool monitor_async_reports_callback(microtouch3m_device_t *dev, microtouch3m_status_t status, int32_t ul_i,
                                               int32_t ul_q, int32_t ur_i, int32_t ur_q, int32_t ll_i, int32_t ll_q,
                                               int32_t lr_i, int32_t lr_q, void *user_data);

    M3MDevice *m_m3m_dev;
    std::queue<signal_t> m_signals;
    Mutex m_mut_signals;
};

#endif // MICROTOUCH3M_SCOPE_M3MDEVICE_HPP
