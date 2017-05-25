#include "Microtouch3MScopeApp.hpp"

#include <sstream>
#include <cmath>
#include <cstring>

#if defined(IMX51)
#include <fcntl.h>
#endif

#include "SDL_net.h"

Microtouch3MScopeApp::Microtouch3MScopeApp(uint32_t width, uint32_t height, uint8_t bits_per_pixel, uint32_t flags,
                                           uint32_t fps_limit) :
    SDLApp(width, height, bits_per_pixel, flags, fps_limit),
    m_sample_count(100),
    m_current_pos(0),
    m_title_update_time(0),
    m_chart_mode(CHART_MODE_ONE),
    m_net_text(""),
    m_scale_target(10000000)
{
    create_charts();

    if (SDLNet_Init() == 0)
    {
        IPaddress adr[10];
        int num = SDLNet_GetLocalAddresses(adr, 10);

        for (int i = 0; i < num; ++i)
        {
            const std::string ipaddr(SDLNet_ResolveIP(&adr[i]));

            if (ipaddr.compare("localhost") == 0) continue;

            std::cout << ipaddr << std::endl;

            m_net_text += "\n" + ipaddr;
        }

        SDLNet_Quit();
    }

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

//    {
//        M3MDevice m3m_dev;
//        m3m_dev.print_info();
//        std::cout << std::endl;
//    }

    m_m3m_dev_mon_thread.start();
}

Microtouch3MScopeApp::~Microtouch3MScopeApp()
{
}

void Microtouch3MScopeApp::update(uint32_t delta_time)
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

    // update charts

    M3MDeviceMonitorThread::signal_t sig;

    while (m_m3m_dev_mon_thread.pop_signal(sig))
    {
        const double scale = (double) (screen_surface()->h / 2 - 10) / m_scale_target;

        int val0 = (int) (sig.ul_corrected_signal * scale);
        int val1 = (int) (sig.ur_corrected_signal * scale);
        int val2 = (int) (sig.ll_corrected_signal * scale);
        int val3 = (int) (sig.lr_corrected_signal * scale);

//        int val0 = (int) (sin(SDL_GetTicks() * 0.01) * 100 - 190);
//        int val1 = (int) (cos(SDL_GetTicks() * 0.05) * 50 + 50);
//        int val2 = (int) ((sin(SDL_GetTicks() * 0.01) + cos(SDL_GetTicks() * 0.02)) * 100 + 50);
//        int val3 = (int) (m_current_pos % 30 - 100);

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

    for (std::vector<LineChart<int> >::iterator it = m_charts.begin(); it != m_charts.end(); ++it)
    {
        it->set_progress((float) (m_current_pos % m_sample_count) / m_sample_count);
    }
}

void Microtouch3MScopeApp::draw()
{
    SDL_FillRect(screen_surface(), 0, 0x00000000);

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

    const int text_margin = 20;

    std::ostringstream oss;
    oss << m_scale_target;

    const std::string str_scale_target(oss.str());

    switch (m_chart_mode)
    {
        case CHART_MODE_ONE:
        {
            const LineChart<int> &chart = m_charts.at(0);

            draw_text(chart.left() + text_margin, chart.top() + text_margin, "Combined");
            draw_text(chart.left() + chart.width() - text_margin, chart.top() + text_margin, "+" + str_scale_target, true);
            draw_text(chart.left() + chart.width() - text_margin,
                      chart.top() + chart.height() - text_margin, "-" + str_scale_target, true, true);
        }
            break;

        case CHART_MODE_FOUR:
        {
            if (m_charts.size() == 4)
            {
                static const char *names[] = { "UL", "UR", "LL", "LR" };

                for (int i = 0; i < m_charts.size(); ++i)
                {
                    const LineChart<int> &chart = m_charts.at(i);

                    draw_text(chart.left() + text_margin, chart.top() + text_margin, names[i]);
                    draw_text(chart.left() + chart.width() - text_margin, chart.top() + text_margin, "+" + str_scale_target, true);
                    draw_text(chart.left() + chart.width() - text_margin,
                              chart.top() + chart.height() - text_margin, "-" + str_scale_target, true, true);
                }
            }
        }

            break;
    }

    draw_text(text_margin, screen_surface()->h - text_margin, m_net_text, false, true);

    SDL_Flip(screen_surface());
}

void Microtouch3MScopeApp::on_event(const SDL_Event &event)
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

void Microtouch3MScopeApp::create_charts()
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

void Microtouch3MScopeApp::draw_text(int32_t x, int32_t y, const std::string &text, bool align_right, bool align_bottom)
{
    m_bmp_font_renderer.draw(screen_surface(), x, y, text, align_right, align_bottom);
}
