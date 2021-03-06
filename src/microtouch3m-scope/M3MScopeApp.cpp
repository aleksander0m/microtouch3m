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

#include "config.h"

#include "M3MScopeApp.hpp"

#include <sstream>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <algorithm>

#include <fcntl.h>
#include <unistd.h>

#include "Utils.hpp"

volatile sig_atomic_t g_make_screenshot = 0;

uint32_t M3MScopeApp::s_text_margin = 20;

M3MScopeApp::M3MScopeApp(uint32_t width, uint32_t height, uint8_t bits_per_pixel, uint32_t flags,
                         uint32_t fps_limit, bool verbose, bool vsync, bool m3m_log, uint32_t samples,
                         ChartMode chart_mode) :
    SDLApp(width, height, bits_per_pixel, flags, fps_limit, verbose, vsync),
    m_sample_count(samples),
    m_current_pos(0),
    m_title_update_time(0),
    m_chart_mode(chart_mode),
    m_print_fps(false),
    m_upd_start(0),
    m_upd_end(0),
    m_chart_prog(0.0f),
    m_old_chart_prog(0.0f),
    m_clear_color(Color(0, 0, 0).map_rgb(screen_surface()->format)),
    m_static_version_text_string("SW Version: " + std::string(PACKAGE_VERSION)),
    m_mac_suffix(Utils::mac().substr(9, 8).erase(2, 1).erase(4, 1))
{
    m_m3m_logger.enable(m3m_log);

    // print m3m info
    // creating device will fail with exception when the device can't be found
#ifndef TEST_VALUES
    {
        M3MDevice m3m_dev;
        m3m_dev.open();

        if (verbose)
        {
            m3m_dev.print_info();

            std::cout << std::endl;
        }

        int fw_maj, fw_min;
        m3m_dev.get_fw_version(&fw_maj, &fw_min);

        {
            std::vector<size_t> len_value, len_label;

            const std::string ip = Utils::ipv4_string();
            const std::string label_ip = "IP: ";
            len_value.push_back(ip.length());
            len_label.push_back(label_ip.length());

            std::ostringstream oss0;
            oss0 << std::hex << fw_maj << "." << fw_min;
            const std::string fw = oss0.str();
            const std::string label_fw = "FW Version: ";
            len_value.push_back(fw.length());
            len_label.push_back(label_fw.length());

            const std::string freq = m3m_dev.get_frequency_string();
            const std::string label_freq = "Frequency: ";
            len_value.push_back(freq.length());
            len_label.push_back(label_freq.length());

            const int w1 = (const int) *std::max_element(len_label.begin(), len_label.end());
            const int w2 = (const int) *std::max_element(len_value.begin(), len_value.end());

            std::ostringstream oss;
            oss << std::left << std::setw(w1) << label_ip << std::right << std::setw(w2) << ip << std::endl
                << std::left << std::setw(w1) << label_fw << std::right << std::setw(w2) << fw << std::endl
                << std::left << std::setw(w1) << label_freq << std::right << std::setw(w2) << freq << std::endl;

            m_static_text_string = oss.str();
        }

        m3m_dev.get_sensitivity_info();

        {
            std::vector<size_t> len_value, len_label;

            std::string sensitivity_level;
            {
                int sl;

                sl = m3m_dev.sensitivity_level();
                if (sl >= MICROTOUCH3M_DEVICE_SENSITIVITY_LEVEL_MIN && sl <= MICROTOUCH3M_DEVICE_SENSITIVITY_LEVEL_MAX) {
                    std::ostringstream oss;
                    oss << sl;
                    sensitivity_level = oss.str();
                } else
                    sensitivity_level = "n/a";
            }
            const std::string label_sensitivity_level = "Sensitivity Level: ";
            len_value.push_back(sensitivity_level.length());
            len_label.push_back(label_sensitivity_level.length());

            std::string touchdown;
            {
                std::ostringstream oss;
                oss << (int) m3m_dev.touchdown();
                touchdown = oss.str();
            }
            const std::string label_touchdown = "Touchdown: ";
            len_value.push_back(touchdown.length());
            len_label.push_back(label_touchdown.length());

            std::string liftoff;
            {
                std::ostringstream oss;
                oss << (int) m3m_dev.liftoff();
                liftoff = oss.str();
            }
            const std::string label_liftoff = "Liftoff: ";
            len_value.push_back(liftoff.length());
            len_label.push_back(label_liftoff.length());

            std::string palm;
            {
                std::ostringstream oss;
                oss << (int) m3m_dev.palm();
                palm = oss.str();
            }
            const std::string label_palm = "Palm: ";
            len_value.push_back(palm.length());
            len_label.push_back(label_palm.length());

            std::string stray;
            {
                std::ostringstream oss;
                oss << (int) m3m_dev.stray();
                stray = oss.str();
            }
            const std::string label_stray = "Stray: ";
            len_value.push_back(stray.length());
            len_label.push_back(label_stray.length());

            std::string stray_alpha;
            {
                std::ostringstream oss;
                oss << (int) m3m_dev.stray_alpha();
                stray_alpha = oss.str();
            }
            const std::string label_stray_alpha = "Stray Alpha: ";
            len_value.push_back(stray_alpha.length());
            len_label.push_back(label_stray_alpha.length());

            const int w1 = (const int) *std::max_element(len_label.begin(), len_label.end());
            const int w2 = (const int) *std::max_element(len_value.begin(), len_value.end());

            std::ostringstream oss;
            oss << std::left << std::setw(w1) << label_sensitivity_level << std::right << std::setw(w2) << sensitivity_level << std::endl
                << std::left << std::setw(w1) << label_touchdown << std::right << std::setw(w2) << touchdown << std::endl
                << std::left << std::setw(w1) << label_liftoff << std::right << std::setw(w2) << liftoff << std::endl
                << std::left << std::setw(w1) << label_palm << std::right << std::setw(w2) << palm << std::endl
                << std::left << std::setw(w1) << label_stray << std::right << std::setw(w2) << stray << std::endl
                << std::left << std::setw(w1) << label_stray_alpha << std::right << std::setw(w2) << stray_alpha << std::endl;

            m_sensitivity_info_string = oss.str();
        }
    }
#endif

    set_scale(10000000);

    create_charts();

#if defined(IMX51)
    {
        const char * const file_path = "/sys/devices/platform/mxc_sdc_fb.0/graphics/fb0/pan";

        int fd = open(file_path, O_RDWR | O_TRUNC);

        if (fd < 0)
        {
            std::cerr << "Error: can't open " << file_path << std::endl;
        }
        else
        {
            write(fd, "0,0", 3);
            close(fd);
        }
    }
#endif
}

M3MScopeApp::~M3MScopeApp()
{
}

void M3MScopeApp::set_print_fps(bool enable)
{
    m_print_fps = enable;
}

void M3MScopeApp::set_scale(uint32_t scale)
{
    m_scale_target = scale;

    std::ostringstream oss;
    oss << m_scale_target;

    m_scale_target_string = oss.str();
}

void M3MScopeApp::on_start()
{
    m_m3m_dev_mon_thread.start();
}

void M3MScopeApp::update(uint32_t delta_time)
{
    if (g_make_screenshot)
    {
        g_make_screenshot = 0;

        make_screenshot();
    }

    if (m_print_fps)
    {
        if ((m_title_update_time += delta_time) > 1000)
        {
#if !defined(IMX51)
            std::ostringstream oss;

            oss << "microtouch-3m-scope - " << screen_surface()->w << "x" << screen_surface()->h
                << " FPS: " << fps();

            SDL_WM_SetCaption(oss.str().c_str(), 0);
#endif

            std::cout << "FPS " << fps() << std::endl;

            m_title_update_time = 0;
        }
    }

    if (m_m3m_dev_mon_thread.done())
    {
        m_exit = true;
        return;
    }

    // update charts

    m_upd_start = m_upd_end;

    std::queue<M3MDeviceMonitorThread::signal_t> * const signals = m_m3m_dev_mon_thread.get_signals_r();

    while (!signals->empty())
    {
        const double scale = (double) (screen_surface()->h / 2 - 10) / m_scale_target;

        m_signal = signals->front();
        signals->pop();

        int val0 = (int) (m_signal.ul * scale);
        int val1 = (int) (m_signal.ur * scale);
        int val2 = (int) (m_signal.ll * scale);
        int val3 = (int) (m_signal.lr * scale);

        switch (m_chart_mode)
        {
            case CHART_MODE_ONE:
            {
                if (m_charts.size() == 1)
                {
                    uint32_t pos = (uint32_t) (m_current_pos % m_sample_count);

                    LineChart<int> &chart = m_charts.at(0);

                    chart.curve(0).set(pos, val0);
                    chart.curve(1).set(pos, val1);
                    chart.curve(2).set(pos, val2);
                    chart.curve(3).set(pos, val3);
                }
            }
                break;

            case CHART_MODE_FOUR:
            {
                if (m_charts.size() == 4)
                {
                    uint32_t pos = (uint32_t) (m_current_pos % m_sample_count);

                    m_charts.at(0).curve(0).set(pos, val0);
                    m_charts.at(1).curve(0).set(pos, val1);
                    m_charts.at(2).curve(0).set(pos, val2);
                    m_charts.at(3).curve(0).set(pos, val3);
                }
            }
                break;
        }

        ++m_current_pos;
    }

    m_upd_end = (uint32_t) ((m_current_pos - 1) % m_sample_count);

    m_strays = m_m3m_dev_mon_thread.get_strays();

    m_old_chart_prog = m_chart_prog;
    m_chart_prog = (float) (m_current_pos % m_sample_count) / m_sample_count;

    for (std::vector<LineChart<int> >::iterator it = m_charts.begin(); it != m_charts.end(); ++it)
    {
        it->set_progress(m_chart_prog);
    }
}

void M3MScopeApp::draw()
{
    sdl_utils::set_clip_area(0, 0, screen_surface()->w, screen_surface()->h);

    // screen clearing optimization - only for one chart mode
    switch (m_chart_mode)
    {
        case CHART_MODE_ONE:
        {
//            m_clear_color = Color((uint8_t) (rand() % 256), (uint8_t) (rand() % 256), (uint8_t) (rand() % 256))
//                            .map_rgb(screen_surface()->format);

            SDL_Rect bounds;

            if ((m_upd_start == m_upd_end && m_upd_start == 0) || m_upd_end < m_upd_start)
            {
                bounds.x = 0;
                bounds.y = 0;
                bounds.w = (Uint16) screen_surface()->w;
                bounds.h = (Uint16) screen_surface()->h;
            }
            else
            {
                LineChart<int> &chart = m_charts.front();

                const float w_step = (float) chart.width() / (m_sample_count - 1);
                const int sx = (const int) (w_step * m_upd_start) + chart.left();
                const int ex = (const int) (w_step * m_upd_end) + chart.left();

                bounds.x = sx;
                bounds.y = chart.top();
                bounds.w = (Uint16) (ex - sx + 1);
                bounds.h = (Uint16) screen_surface()->h;

                // clear old progress line
                sdl_utils::draw_line(screen_surface(),
                                     (int32_t) (chart.left() + chart.width() * m_old_chart_prog), chart.top(),
                                     (int32_t) (chart.left() + chart.width() * m_old_chart_prog), chart.top() + chart.height(),
                                     0);
            }

            SDL_FillRect(screen_surface(), &bounds, m_clear_color);

            // clear strays text area
//            if (m_strays != m_prev_strays)
            {
                std::ostringstream oss;

                oss << std::left << "STRAYS UL: " << std::setw(11) << std::right << m_strays.ul << std::endl
                    << std::left << "STRAYS UR: " << std::setw(11) << std::right << m_strays.ur << std::endl
                    << std::left << "STRAYS LL: " << std::setw(11) << std::right << m_strays.ll << std::endl
                    << std::left << "STRAYS LR: " << std::setw(11) << std::right << m_strays.lr;

                const int64_t delta_strays_ul = m_signal.ul;
                const int64_t delta_strays_ur = m_signal.ur;
                const int64_t delta_strays_ll = m_signal.ll;
                const int64_t delta_strays_lr = m_signal.lr;
                const int64_t delta_strays_sum = delta_strays_ul + delta_strays_ur + delta_strays_ll + delta_strays_lr;

                oss << std::endl << std::endl
                    << std::left << "DELTA UL: " << std::setw(11) << std::right << delta_strays_ul << std::endl
                    << std::left << "DELTA UR: " << std::setw(11) << std::right << delta_strays_ur << std::endl
                    << std::left << "DELTA LL: " << std::setw(11) << std::right << delta_strays_ll << std::endl
                    << std::left << "DELTA LR: " << std::setw(11) << std::right << delta_strays_lr;

                oss << std::endl << std::endl
                    << std::left << "DELTA SUM: " << std::setw(11) << std::right << delta_strays_sum << std::endl;

                m_strays_text_string = oss.str();

                const uint32_t text_width = m_bmp_font_renderer.text_width(m_strays_text_string);

                m_strays_text_rect.h = (Uint16) m_bmp_font_renderer.text_height(m_strays_text_string);
                m_strays_text_rect.w = (Uint16) text_width;
                m_strays_text_rect.x = screen_surface()->w - s_text_margin - m_strays_text_rect.w;
                m_strays_text_rect.y = s_text_margin + m_bmp_font_renderer.text_height(m_static_text_string)
                                       + s_text_margin + m_bmp_font_renderer.text_height(m_sensitivity_info_string)
                                       + s_text_margin;

                SDL_FillRect(screen_surface(), &m_strays_text_rect, m_clear_color);

                m_prev_strays = m_strays;
            }
        }
            break;

        case CHART_MODE_FOUR:
            SDL_FillRect(screen_surface(), 0, m_clear_color);
            break;
    }

    if (SDL_MUSTLOCK(screen_surface()))
    {
        if (SDL_LockSurface(screen_surface()) != 0)
        {
            std::cerr << "Can't lock screen: " << SDL_GetError() << std::endl;
            m_exit_code = 1;
            m_exit = true;
            return;
        }
    }

    // draw charts

    for (std::vector<LineChart<int> >::iterator it = m_charts.begin(); it != m_charts.end(); ++it)
    {
        it->draw(screen_surface());
    }

    if (SDL_MUSTLOCK(screen_surface()))
    {
        SDL_UnlockSurface(screen_surface());
    }

    // draw text

    const int text_margin = s_text_margin;

    switch (m_chart_mode)
    {
        case CHART_MODE_ONE:
        {
            const LineChart<int> &chart = m_charts.at(0);

            draw_text(chart.left() + chart.width() / 2 - text_margin, chart.top() + text_margin, "Combined");
            draw_text(chart.left() + text_margin, chart.top() + text_margin, "+" + m_scale_target_string);
            draw_text(chart.left() + text_margin,
                      chart.top() + chart.height() - text_margin, "-" + m_scale_target_string, false, true);
        }
            break;

        case CHART_MODE_FOUR:
        {
            if (m_charts.size() == 4)
            {
                static const char *names[] = { "UL", "UR", "LL", "LR" };

                for (size_t i = 0; i < m_charts.size(); ++i)
                {
                    const LineChart<int> &chart = m_charts.at(i);

                    draw_text(chart.left() + text_margin, chart.top() + text_margin, names[i]);

                    if (i != 1)
                    {
                        draw_text(chart.left() + chart.width() - text_margin, chart.top() + text_margin,
                                  "+" + m_scale_target_string, true);
                    }

                    draw_text(chart.left() + chart.width() - text_margin,
                              chart.top() + chart.height() - text_margin, "-" + m_scale_target_string, true, true);
                }
            }
        }

            break;
    }

    draw_text(screen_surface()->w - text_margin, text_margin, m_static_text_string, true);

    draw_text(screen_surface()->w - text_margin, text_margin * 2 + m_bmp_font_renderer.text_height(m_static_text_string),
              m_sensitivity_info_string, true);

    draw_text(m_strays_text_rect.x + m_strays_text_rect.w, m_strays_text_rect.y, m_strays_text_string, true);

    draw_text(screen_surface()->w - text_margin, screen_surface()->h - text_margin,
              m_static_version_text_string, true, true);
}

void M3MScopeApp::on_event(const SDL_Event &event)
{
    switch (event.type)
    {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE)
                m_exit = true;

            if (event.key.keysym.sym == SDLK_SPACE)
            {
                m_chart_mode = m_chart_mode == CHART_MODE_ONE ? CHART_MODE_FOUR : CHART_MODE_ONE;

                create_charts();
            }

            break;

        default:
            break;
    }
}

void M3MScopeApp::create_charts()
{
    const uint32_t margin = 10;

    const uint32_t width = (const uint32_t) screen_surface()->w;
    const uint32_t height = (const uint32_t) screen_surface()->h;

    m_charts.clear();

    m_current_pos = 0;

    switch (m_chart_mode)
    {
        case CHART_MODE_ONE:
        {
            LineChart<int> chart;

            chart.set_geometry(margin, margin, width - margin * 2, height - margin * 2);

            chart.add_curve(Color(0xff, 0, 0), m_sample_count, 0);
            chart.add_curve(Color(0, 0xff, 0), m_sample_count, 0);
            chart.add_curve(Color(0, 0, 0xff), m_sample_count, 0);
            chart.add_curve(Color(0xff, 0xff, 0xff), m_sample_count, 0);

            m_charts.push_back(chart);
        }
            break;

        case CHART_MODE_FOUR:
        {
            LineChart<int> lc_ul;
            LineChart<int> lc_ur;
            LineChart<int> lc_ll;
            LineChart<int> lc_lr;

            lc_ul.set_geometry(margin, margin, width / 2 - margin * 2, height / 2 - margin * 2);
            lc_ur.set_geometry(margin + width / 2, margin, width / 2 - margin * 2, height / 2 - margin * 2);
            lc_ll.set_geometry(margin, margin + height / 2, width / 2 - margin * 2, height / 2 - margin * 2);
            lc_lr.set_geometry(margin + width / 2, margin + height / 2, width / 2 - margin * 2, height / 2 - margin * 2);

            lc_ul.add_curve(Color(0xff, 0, 0), m_sample_count, 0);
            lc_ur.add_curve(Color(0, 0xff, 0), m_sample_count, 0);
            lc_ll.add_curve(Color(0, 0, 0xff), m_sample_count, 0);
            lc_lr.add_curve(Color(0xff, 0xff, 0xff), m_sample_count, 0);

            m_charts.push_back(lc_ul);
            m_charts.push_back(lc_ur);
            m_charts.push_back(lc_ll);
            m_charts.push_back(lc_lr);
        }
            break;
    }
}

void M3MScopeApp::draw_text(int32_t x, int32_t y, const std::string &text, bool align_right, bool align_bottom)
{
    m_bmp_font_renderer.draw(screen_surface(), x, y, text, align_right, align_bottom);
}

void M3MScopeApp::make_screenshot()
{
    time_t t = time(0);
    struct tm *tmp = localtime(&t);

    if (!tmp)
    {
        std::cerr << "localtime error" << std::endl;
        return;
    }

    char timestr[200];

    if (strftime(timestr, sizeof(timestr), "%Y-%m-%d-%H-%M-%S%z", tmp) == 0)
    {
        std::cerr << "strftime error" << std::endl;
        return;
    }

    screenshot("/tmp/microtouch3m-scope-" + m_mac_suffix + "-" + std::string(timestr) + ".ppm");
}
