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

#ifndef MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP
#define MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP

#include <csignal>

#include "SDLApp.hpp"
#include "LineChart.hpp"
#include "BitmapFontRenderer.hpp"
#include "M3MDevice.hpp"
#include "M3MLogger.hpp"

extern volatile sig_atomic_t g_make_screenshot;

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
    virtual void on_start();
    virtual void update(uint32_t delta_time);
    virtual void draw();
    virtual void on_event(const SDL_Event &event);

private:
    void create_charts();
    void draw_text(int32_t x, int32_t y, const std::string &text, bool align_right = false, bool align_bottom = false);
    void make_screenshot();

    static uint32_t s_text_margin;

    uint32_t m_sample_count;
    uint64_t m_current_pos;
    std::vector<LineChart<int> > m_charts;
    uint32_t m_title_update_time;
    ChartMode m_chart_mode;
    BitmapFontRenderer m_bmp_font_renderer;
    uint32_t m_scale_target;
    M3MLogger m_m3m_logger;
    M3MDeviceMonitorThread m_m3m_dev_mon_thread;
    std::string m_static_text_string;
    std::string m_static_version_text_string;
    std::string m_scale_target_string;
    bool m_print_fps;
    uint32_t m_upd_start;
    uint32_t m_upd_end;
    float m_chart_prog;
    float m_old_chart_prog;
    Uint32 m_clear_color;
    M3MDeviceMonitorThread::signal_t m_strays;
    M3MDeviceMonitorThread::signal_t m_prev_strays;
    M3MDeviceMonitorThread::signal_t m_signal;
    SDL_Rect m_strays_text_rect;
    std::string m_strays_text_string;
    std::string m_sensitivity_info_string;
    std::string m_mac_suffix;
};

#endif // MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP
