#include "M3MDevice.hpp"

#include <stdexcept>
#include <iostream>
#include <cmath>

#include <unistd.h>

#include "Utils.hpp"

#define PROCESS_IQ(i,q) (uint64_t) sqrt ((((double)i) * ((double)i)) + (((double)q) * ((double)q)))

M3MContext::M3MContext()
{
    if (!(m_ctx = microtouch3m_context_new()))
    {
        throw std::runtime_error("microtouch3m_context_new failed");
    }
}

M3MContext::~M3MContext()
{
    microtouch3m_context_unref(m_ctx);
}

microtouch3m_context_t *M3MContext::context() const
{
    return m_ctx;
}

M3MDevice::M3MDevice() :
    m_ul_stray_signal(0),
    m_ur_stray_signal(0),
    m_ll_stray_signal(0),
    m_lr_stray_signal(0),
    m_fw_major(-1),
    m_fw_minor(-1)
{
    if (!(m_dev = microtouch3m_device_new_first(m_ctx.context())))
    {
        throw std::runtime_error("M3M: Getting device failed");
    }
}

M3MDevice::~M3MDevice()
{
    microtouch3m_device_close(m_dev);
    microtouch3m_device_unref(m_dev);
}

void M3MDevice::open()
{
    microtouch3m_status_t st;

    if ((st = microtouch3m_device_open(m_dev)) != MICROTOUCH3M_STATUS_OK)
    {
        throw std::runtime_error("M3M: Couldn't open device - " + std::string(microtouch3m_status_to_string(st)));
    }
}

void M3MDevice::print_info()
{
    std::cout << "M3M device: Bus " << (int) microtouch3m_device_get_usb_bus_number(m_dev)
              << " Device " << (int) microtouch3m_device_get_usb_device_address(m_dev) << std::endl;

    {
        int fw_maj, fw_min;

        get_fw_version(&fw_maj, &fw_min);

        std::cout << "M3M: firmware version - " << fw_maj << "." << fw_min << std::endl;
    }

    std::cout << "M3M: frequency - " << get_frequency_string() << std::endl;
}

void M3MDevice::read_strays()
{
    microtouch3m_status_t st;

    int32_t ul_stray_i;
    int32_t ul_stray_q;
    int32_t ur_stray_i;
    int32_t ur_stray_q;
    int32_t ll_stray_i;
    int32_t ll_stray_q;
    int32_t lr_stray_i;
    int32_t lr_stray_q;

    if ((st = microtouch3m_read_strays(m_dev, &ul_stray_i, &ul_stray_q,
                                       &ur_stray_i, &ur_stray_q,
                                       &ll_stray_i, &ll_stray_q,
                                       &lr_stray_i, &lr_stray_q))
        != MICROTOUCH3M_STATUS_OK)
    {
        throw std::runtime_error("M3M: Couldn't read strays - " + std::string(microtouch3m_status_to_string(st)));
    }

    m_ul_stray_signal = PROCESS_IQ(ul_stray_i, ul_stray_q);
    m_ur_stray_signal = PROCESS_IQ(ur_stray_i, ur_stray_q);
    m_ll_stray_signal = PROCESS_IQ(ll_stray_i, ll_stray_q);
    m_lr_stray_signal = PROCESS_IQ(lr_stray_i, lr_stray_q);
}

void M3MDevice::get_fw_version(int *major, int *minor)
{
    if (m_fw_major == -1 || m_fw_minor == -1)
    {
        microtouch3m_status_t st;
        uint8_t fw_maj, fw_min;

        if ((st = microtouch3m_device_query_controller_id(m_dev, 0, &fw_maj, &fw_min, 0, 0, 0, 0, 0))
            != MICROTOUCH3M_STATUS_OK)
        {
            throw std::runtime_error("M3M: Couldn't query controller - "
                                     + std::string(microtouch3m_status_to_string(st)));
        }

        m_fw_major = fw_maj;
        m_fw_minor = fw_min;
    }

    if (major) *major = m_fw_major;
    if (minor) *minor = m_fw_minor;
}

std::string M3MDevice::get_frequency_string()
{
    if (m_frequency_str.empty())
    {
        microtouch3m_status_t st;

        microtouch3m_device_frequency_t dev_freq;

        if ((st = microtouch3m_device_get_frequency(m_dev, &dev_freq)) != MICROTOUCH3M_STATUS_OK)
        {
            throw std::runtime_error("M3M: Couldn't get frequency - "
                                     + std::string(microtouch3m_status_to_string(st)));
        }

        m_frequency_str = microtouch3m_device_frequency_to_string(dev_freq);
    }

    return m_frequency_str;
}

void M3MDevice::monitor_async_reports(microtouch3m_device_async_report_scope_f *callback, void *user_data)
{
    microtouch3m_status_t st;

    if ((st = microtouch3m_device_monitor_async_reports(m_dev, callback, user_data)) != MICROTOUCH3M_STATUS_OK)
    {
        throw std::runtime_error("M3M: Couldn't monitor async reports - " + std::string(microtouch3m_status_to_string(st)));
    }
}

M3MDeviceMonitorThread::M3MDeviceMonitorThread() :
    Thread("m3m-dev-mon"),
    m_signals_r(&m_signals0),
    m_signals_w(&m_signals1),
    m_callback_status(MICROTOUCH3M_STATUS_OK)
{ }

M3MDeviceMonitorThread::~M3MDeviceMonitorThread()
{
    exit();
    join();
}

std::queue<M3MDeviceMonitorThread::signal_t> *M3MDeviceMonitorThread::get_signals_r()
{
    MutexLock lock(&m_mut_signals);
    std::swap(m_signals_r, m_signals_w);
    return m_signals_r;
}

M3MDeviceMonitorThread::signal_t M3MDeviceMonitorThread::get_strays()
{
    signal_t strays;
    {
        MutexLock lock(&m_mut_strays);
        strays = m_strays;
    }
    return strays;
}

void M3MDeviceMonitorThread::push_signal(const M3MDeviceMonitorThread::signal_t &sig)
{
    MutexLock lock(&m_mut_signals);
    m_signals_w->push(sig);
}

void M3MDeviceMonitorThread::set_strays(const M3MDeviceMonitorThread::signal_t &sig)
{
    MutexLock lock(&m_mut_strays);
    m_strays = sig;
}

bool M3MDeviceMonitorThread::run()
{
    try
    {
        clock_gettime(CLOCK_REALTIME, &m_strays_update_time);

#ifdef TEST_VALUES
        while (!get_exit())
        {
            static uint64_t count = 0;

            const int scale = 30000;

            double test_time = count * 20.0;

            int val0 = (int) (scale * (sin(test_time * 0.01) * 100 - 190));
            int val1 = (int) (scale * (cos(test_time * 0.05) * 50 + 50));
            int val2 = (int) (scale * ((sin(test_time * 0.01) + cos(test_time * 0.02)) * 100 + 50));
            int val3 = (int) (scale * (count % 30 - 100));

            push_signal(signal_t(val0, val1, val2, val3));

            timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            const timespec time_diff = Utils::timespec_diff(m_strays_update_time, now);

            if (time_diff.tv_sec * 1000000000 + time_diff.tv_nsec >= 500000000)
            {
                m_strays_update_time = now;
                set_strays(signal_t(val0, val1, val2, val3));
            }

            ++count;

            usleep(1000000 / 70);
        }
#else
        m_m3m_dev = new M3MDevice();

        m_m3m_dev->open();
        m_m3m_dev->read_strays();

        m_m3m_dev->monitor_async_reports(M3MDeviceMonitorThread::monitor_async_reports_callback, this);

        delete m_m3m_dev;
        m_m3m_dev = 0;
#endif
    }
    catch (std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
    }

    if (m_callback_status != MICROTOUCH3M_STATUS_OK)
    {
        std::cerr << "M3M: callback failed with status - " << microtouch3m_status_to_string(m_callback_status) << std::endl;
    }

    return false;
}

bool M3MDeviceMonitorThread::monitor_async_reports_callback(microtouch3m_device_t *dev, microtouch3m_status_t status, int32_t ul_i,
                                              int32_t ul_q, int32_t ur_i, int32_t ur_q, int32_t ll_i, int32_t ll_q,
                                              int32_t lr_i, int32_t lr_q, void *user_data)
{
    M3MDeviceMonitorThread * const thread = static_cast<M3MDeviceMonitorThread * const>(user_data);

    if (status != MICROTOUCH3M_STATUS_OK)
    {
        thread->m_callback_status = status;

        return false;
    }

    if (!thread) return false;

    const uint64_t ul_signal = PROCESS_IQ(ul_i, ul_q);
    const uint64_t ur_signal = PROCESS_IQ(ur_i, ur_q);
    const uint64_t ll_signal = PROCESS_IQ(ll_i, ll_q);
    const uint64_t lr_signal = PROCESS_IQ(lr_i, lr_q);

    thread->push_signal(signal_t(
        ((int64_t) ul_signal) - ((int64_t) thread->m_m3m_dev->m_ul_stray_signal),
        ((int64_t) ur_signal) - ((int64_t) thread->m_m3m_dev->m_ur_stray_signal),
        ((int64_t) ll_signal) - ((int64_t) thread->m_m3m_dev->m_ll_stray_signal),
        ((int64_t) lr_signal) - ((int64_t) thread->m_m3m_dev->m_lr_stray_signal)
    ));

    timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    const timespec time_diff = Utils::timespec_diff(thread->m_strays_update_time, now);

    if (time_diff.tv_sec * 1000000000 + time_diff.tv_nsec >= 500000000)
    {
        thread->m_m3m_dev->read_strays();
        thread->m_strays_update_time = now;

        thread->set_strays(signal_t(
            thread->m_m3m_dev->m_ul_stray_signal,
            thread->m_m3m_dev->m_ur_stray_signal,
            thread->m_m3m_dev->m_ll_stray_signal,
            thread->m_m3m_dev->m_lr_stray_signal
        ));
    }

    return !thread->get_exit();
}
