#ifndef MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP
#define MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP

#include "SDLApp.hpp"
#include "LineChart.hpp"
#include "BitmapFontRenderer.hpp"

class Microtouch3MScopeApp : public SDLApp
{
public:
    Microtouch3MScopeApp(uint32_t width, uint32_t height, uint32_t flags, uint32_t fps_limit);
    virtual ~Microtouch3MScopeApp();

protected:
    virtual void update(uint32_t delta_time);
    virtual void draw();
    virtual void on_event(const SDL_Event &event);

private:

    enum ChartMode
    {
        CHART_MODE_ONE,
        CHART_MODE_FOUR
    };

    void create_charts();
    void draw_text(int32_t x, int32_t y, const std::string &text, bool align_right = false, bool align_bottom = false);

    uint32_t m_sample_count;
    uint64_t m_current_pos;
    std::vector<LineChart<int> > m_charts;
    uint32_t m_title_update_time;
    ChartMode m_chart_mode;
    BitmapFontRenderer m_bmp_font_renderer;
    std::string m_net_text;
};

#endif // MICROTOUCH_3M_SCOPE_MICROTOUCH3MSCOPEAPP_HPP