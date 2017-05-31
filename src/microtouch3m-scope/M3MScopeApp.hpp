#ifndef MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP
#define MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP

#include "SDLApp.hpp"
#include "LineChart.hpp"
#include "BitmapFontRenderer.hpp"
#include "M3MDevice.hpp"
#include "M3MLogger.hpp"

class M3MScopeApp : public SDLApp
{
public:

    enum ChartMode
    {
        CHART_MODE_ONE,
        CHART_MODE_FOUR
    };

    M3MScopeApp(uint32_t width, uint32_t height, uint8_t bits_per_pixel, uint32_t flags,
                uint32_t fps_limit, bool verbose, bool vsync, bool m3m_log, uint32_t samples,
                ChartMode chart_mode);

    virtual ~M3MScopeApp();

    void set_print_fps(bool enable);
    void set_scale(uint32_t scale);

protected:
    virtual void update(uint32_t delta_time);
    virtual void draw();
    virtual void on_event(const SDL_Event &event);

private:
    void create_charts();
    void draw_text(int32_t x, int32_t y, const std::string &text, bool align_right = false, bool align_bottom = false);

    uint32_t m_sample_count;
    uint64_t m_current_pos;
    std::vector<LineChart<int> > m_charts;
    uint32_t m_title_update_time;
    ChartMode m_chart_mode;
    BitmapFontRenderer m_bmp_font_renderer;
    std::string m_net_text;
    uint32_t m_scale_target;
    M3MLogger m_m3m_logger;
    M3MDeviceMonitorThread m_m3m_dev_mon_thread;
    std::string m_static_text_string;
    std::string m_scale_target_string;
    bool m_print_fps;
    uint32_t m_upd_start;
    uint32_t m_upd_end;
    float m_chart_prog;
    float m_old_chart_prog;
    Uint32 m_clear_color;
    M3MDeviceMonitorThread::signal_t m_strays;
    M3MDeviceMonitorThread::signal_t m_prev_strays;
    SDL_Rect m_strays_text_rect;
    std::string m_strays_text_string;
};

#endif // MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP
