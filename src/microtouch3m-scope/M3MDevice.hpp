#ifndef MICROTOUCH3M_SCOPE_M3MDEVICE_HPP
#define MICROTOUCH3M_SCOPE_M3MDEVICE_HPP

#include "SDL_stdinc.h"

#include "microtouch3m.h"

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
public:
    M3MDevice();
    virtual ~M3MDevice();

    int64_t ul_corrected_signal() const;
    int64_t ur_corrected_signal() const;
    int64_t ll_corrected_signal() const;
    int64_t lr_corrected_signal() const;

    void open();
    void print_info();
    void read_strays();
    void test_async();
private:
    static bool monitor_async_reports_callback(microtouch3m_device_t *dev,
                                               microtouch3m_status_t status,
                                               int32_t ul_i,
                                               int32_t ul_q,
                                               int32_t ur_i,
                                               int32_t ur_q,
                                               int32_t ll_i,
                                               int32_t ll_q,
                                               int32_t lr_i,
                                               int32_t lr_q,
                                               void *user_data);

    M3MContext m_ctx;
    microtouch3m_device_t *m_dev;

    uint64_t m_ul_stray_signal;
    uint64_t m_ur_stray_signal;
    uint64_t m_ll_stray_signal;
    uint64_t m_lr_stray_signal;

    int64_t m_ul_corrected_signal;
    int64_t m_ur_corrected_signal;
    int64_t m_ll_corrected_signal;
    int64_t m_lr_corrected_signal;
};

#endif // MICROTOUCH3M_SCOPE_M3MDEVICE_HPP
