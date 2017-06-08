#ifndef MICROTOUCH_3M_SCOPE_GRAPH_HPP
#define MICROTOUCH_3M_SCOPE_GRAPH_HPP

#include <vector>

#include "SDL.h"

#include "SDLUtils.hpp"
#include "Utils.hpp"

template<class T>
class LineChart
{
public:

    class Curve
    {
        friend class LineChart;

    public:
        Curve(const Color &color, typename std::vector<T>::size_type fill_count, T fill_value) :
            color(color)
        {
            data.insert(data.begin(), fill_count, fill_value);
        }

        size_t data_count() const
        {
            return data.size();
        }

        T get(typename std::vector<T>::size_type pos) const
        {
            return data.at(pos);
        }

        void set(typename std::vector<T>::size_type pos, T value)
        {
            data.at(pos) = value;
        }

        Color color;

    private:
        std::vector<T> data;
    };

    LineChart() : m_width(0), m_height(0), m_left(0), m_top(0), m_right(0), m_bottom(0), m_progress(0.0f)
    {}

    Curve &add_curve(const Color &color, typename std::vector<T>::size_type fill_count, T fill_value)
    {
        m_curves.push_back(Curve(color, fill_count, fill_value));

        return m_curves.back();
    }

    size_t curves_count() const
    {
        return m_curves.size();
    }

    Curve &curve(typename std::vector<Curve>::size_type i)
    {
        return m_curves.at(i);
    }

    void set_progress(float progress)
    {
        m_progress = progress;
    }

    void set_geometry(uint32_t left, uint32_t top, uint32_t width, uint32_t height)
    {
        m_left = left;
        m_top = top;
        m_width = width;
        m_height = height;

        m_right = m_left + m_width - 1;
        m_bottom = m_top + m_height - 1;

        m_middle_y = m_top + m_height / 2;

        m_grid_cell_w = m_width / Utils::closest_factor(m_width, 14);
        m_grid_cell_h = m_height / Utils::closest_factor(m_height, 10);
        m_grid_dash_step = Utils::gcd(m_grid_cell_w, m_grid_cell_h);
        m_grid_stub_h = m_grid_dash_step * 2;
    }

    void draw(SDL_Surface *surface)
    {
        if (!surface) return;

        sdl_utils::set_clip_area(m_left, m_top, m_width, m_height);

        for (typename std::vector<Curve>::iterator it = m_curves.begin(); it != m_curves.end(); ++it)
        {
            Curve &curve = *it;

            if (curve.data.size() < 2)
                continue;

            float w_step = (float) m_width / (curve.data.size() - 1);

            bool first = true;
            int32_t px = 0, py = 0;
            const Uint32 col = curve.color.map_rgb(surface->format);

            for (int i = 0; i < curve.data.size(); ++i)
            {
                const int y = curve.data.at(i);
                const int x = (const int) (w_step * i);

                if (i == 0)
                {
                    px = x; py = y;
                    continue;
                }

                sdl_utils::draw_line(surface,
                                    px + m_left, m_top + m_height / 2 - py,
                                    x + m_left, m_top + m_height / 2 - y,
                                    col);

                px = x; py = y;
            }
        }

        // grid

        const Uint32 col_border(Color(0xff, 0xff, 0x0).map_rgb(surface->format));

        // horizontal lines
        for (int y = m_top + m_grid_cell_h; y < m_bottom; y += m_grid_cell_h)
        {
            for (int x = m_left; x < m_right; x += m_grid_dash_step)
            {
                sdl_utils::set_pixel_no_clip(surface, (uint32_t) x, (uint32_t) y, col_border);
            }
        }

        // vertical lines
        for (int x = m_left + m_grid_cell_w; x < m_right; x += m_grid_cell_w)
        {
            for (int y = m_top; y < m_bottom; y += m_grid_dash_step)
            {
                sdl_utils::set_pixel_no_clip(surface, (uint32_t) x, (uint32_t) y, col_border);
            }

            // stubs
            sdl_utils::draw_line(surface, x, m_top, x, m_top + m_grid_stub_h, col_border);
            sdl_utils::draw_line(surface, x, m_bottom, x, m_bottom - m_grid_stub_h, col_border);
            sdl_utils::draw_line(surface, x, m_middle_y - m_grid_stub_h, x, m_middle_y + m_grid_stub_h, col_border);
        }

        // border

        sdl_utils::draw_line(surface,
                            m_left, m_top,
                            m_right, m_top,
                            col_border);

        sdl_utils::draw_line(surface,
                            m_right, m_top,
                            m_right, m_bottom,
                            col_border);

        sdl_utils::draw_line(surface,
                            m_right, m_bottom,
                            m_left, m_bottom,
                            col_border);

        sdl_utils::draw_line(surface,
                            m_left, m_bottom,
                            m_left, m_top,
                            col_border);

        // horizontal

        const Uint32 col_axis_x(Color(0xff, 0xff, 0x0).map_rgb(surface->format));

        sdl_utils::draw_line(surface,
                            m_left, m_top + m_height / 2,
                            m_right, m_top + m_height / 2,
                            col_axis_x);

        // vertical

        const Uint32 col_axis_y(Color(0xaa, 0xff, 0xaa).map_rgb(surface->format));

        sdl_utils::draw_line(surface,
                            (int32_t) (m_left + m_width * m_progress), m_top,
                            (int32_t) (m_left + m_width * m_progress), m_top + m_height,
                            col_axis_y);
    }

    int left() const
    {
        return m_left;
    }

    int top() const
    {
        return m_top;
    }

    uint32_t width() const
    {
        return m_width;
    }

    uint32_t height() const
    {
        return m_height;
    }

protected:
    uint32_t m_width, m_height;
    int32_t m_left, m_top;
    int32_t m_right, m_bottom;
    float m_progress;
    std::vector<Curve> m_curves;
    uint32_t m_grid_dash_step;
    uint32_t m_grid_cell_w;
    uint32_t m_grid_cell_h;
    uint32_t m_middle_y;
    uint32_t m_grid_stub_h;
};

#endif // MICROTOUCH_3M_SCOPE_GRAPH_HPP
