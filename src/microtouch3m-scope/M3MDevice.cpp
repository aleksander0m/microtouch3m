#include "M3MDevice.hpp"

#include <stdexcept>
#include <iostream>
#include <cmath>

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
    m_lr_stray_signal(0)
{
    if (!(m_dev = microtouch3m_device_new_first(m_ctx.context())))
    {
        throw std::runtime_error("M3M: Getting device failed");
    }

    std::cout << "M3M device: Bus " << (int) microtouch3m_device_get_usb_bus_number(m_dev)
              << " Device " << (int) microtouch3m_device_get_usb_device_address(m_dev) << std::endl;
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
    microtouch3m_status_t st;

    {
        uint8_t fw_maj, fw_min;

        if ((st = microtouch3m_device_query_controller_id(m_dev, 0, &fw_maj, &fw_min, 0, 0, 0, 0, 0))
            != MICROTOUCH3M_STATUS_OK)
        {
            throw std::runtime_error("M3M: Couldn't query controller - "
                                     + std::string(microtouch3m_status_to_string(st)));
        }

        std::cout << "M3M: firmware version - " << (int) fw_maj << "." << (int) fw_min << std::endl;
    }

    {
        microtouch3m_device_frequency_t dev_freq;

        if ((st = microtouch3m_device_get_frequency(m_dev, &dev_freq)) != MICROTOUCH3M_STATUS_OK)
        {
            throw std::runtime_error("M3M: Couldn't get frequency - "
                                     + std::string(microtouch3m_status_to_string(st)));
        }

        std::cout << "M3M: frequency - " << microtouch3m_device_frequency_to_string(dev_freq) << std::endl;
    }
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

void M3MDevice::monitor_async_reports(microtouch3m_device_async_report_scope_f *callback, void *user_data)
{
    microtouch3m_status_t st;

    if ((st = microtouch3m_device_monitor_async_reports(m_dev, callback, user_data)) != MICROTOUCH3M_STATUS_OK)
    {
        throw std::runtime_error("M3M: Couldn't monitor async reports - " + std::string(microtouch3m_status_to_string(st)));
    }
}

bool M3MDeviceMonitorThread::pop_signal(M3MDeviceMonitorThread::signal_t &sig)
{
    MutexLock lock(&m_mut_signals);
    if (!m_signals.empty())
    {
        sig = m_signals.front();
        m_signals.pop();
        return true;
    }
    return false;
}

void M3MDeviceMonitorThread::push_signal(const M3MDeviceMonitorThread::signal_t &sig)
{
    MutexLock lock(&m_mut_signals);
    m_signals.push(sig);
}

bool M3MDeviceMonitorThread::run()
{
    try
    {
        clock_gettime(CLOCK_REALTIME, &m_strays_update_time);

        m_m3m_dev = new M3MDevice();

        m_m3m_dev->open();
        m_m3m_dev->read_strays();

        m_m3m_dev->monitor_async_reports(M3MDeviceMonitorThread::monitor_async_reports_callback, this);

        delete m_m3m_dev;
        m_m3m_dev = 0;
    }
    catch (std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return false;
}

bool M3MDeviceMonitorThread::monitor_async_reports_callback(microtouch3m_device_t *dev, microtouch3m_status_t status, int32_t ul_i,
                                              int32_t ul_q, int32_t ur_i, int32_t ur_q, int32_t ll_i, int32_t ll_q,
                                              int32_t lr_i, int32_t lr_q, void *user_data)
{
    if (status != MICROTOUCH3M_STATUS_OK)
    {
        throw std::runtime_error("M3M: callback failed - " + std::string(microtouch3m_status_to_string(status)));
    }

    M3MDeviceMonitorThread * const thread = static_cast<M3MDeviceMonitorThread * const>(user_data);

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
    }

    return !thread->get_exit();
}
