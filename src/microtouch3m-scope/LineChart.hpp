#ifndef MICROTOUCH_3M_SCOPE_GRAPH_HPP
#define MICROTOUCH_3M_SCOPE_GRAPH_HPP

#include <vector>

#include "SDL.h"

#include "SDLUtils.hpp"

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

    LineChart() : m_width(0), m_height(0), m_left(0), m_top(0), m_progress(0.0f)
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

        // border

        const Uint32 col_border(Color(0xff, 0xff, 0x0).map_rgb(surface->format));

        sdl_utils::draw_line(surface,
                            m_left, m_top,
                            m_left + m_width, m_top,
                            col_border);

        sdl_utils::draw_line(surface,
                            m_left + m_width, m_top,
                            m_left + m_width, m_top + m_height,
                            col_border);

        sdl_utils::draw_line(surface,
                            m_left + m_width, m_top + m_height,
                            m_left, m_top + m_height,
                            col_border);

        sdl_utils::draw_line(surface,
                            m_left, m_top + m_height,
                            m_left, m_top,
                            col_border);

        // horizontal

        const Uint32 col_axis_x(Color(0xff, 0xff, 0x0).map_rgb(surface->format));

        sdl_utils::draw_line(surface,
                            m_left, m_top + m_height / 2,
                            m_left + m_width, m_top + m_height / 2,
                            col_axis_x);

        for (int i = 1; i < 10; ++i)
        {
            const int x = m_left + i * m_width / 10;

            sdl_utils::draw_line(surface,
                                x, m_top + m_height / 2 - 5,
                                x, m_top + m_height / 2 + 5,
                                col_axis_x);
        }

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
    int m_left, m_top;
    float m_progress;
    std::vector<Curve> m_curves;
};

#endif // MICROTOUCH_3M_SCOPE_GRAPH_HPP
