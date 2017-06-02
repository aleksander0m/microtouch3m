#include "SDLUtils.hpp"

#include <iostream>

#include "SDL_endian.h"

void ::sdl_utils::set_clip_area(int x = 0, int y = 0, int width = 999999, int height = 999999)
{
    m_clip_x0 = x;
    m_clip_y0 = y;
    m_clip_x1 = m_clip_x0 + width - 1;
    m_clip_y1 = m_clip_y0 + height - 1;
}

void ::sdl_utils::set_pixel(SDL_Surface *surface, uint32_t x, uint32_t y, Uint32 color)
{
    if (x < m_clip_x0 || x > m_clip_x1 || y < m_clip_y0 || y > m_clip_y1) return;

    const int bpp = surface->format->BytesPerPixel;

    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
        case 1:
            *p = (Uint8) color;
            break;

        case 2:
            *(Uint16 *) p = (Uint16) color;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
                // bgr
                p[0] = (color >> 16) & 0xff;
                p[1] = (color >> 8) & 0xff;
                p[2] = color & 0xff;
            }
            else
            {
                // rgb
                p[0] = color & 0xff;
                p[1] = (color >> 8) & 0xff;
                p[2] = (color >> 16) & 0xff;
            }
            break;

        case 4:
            *(Uint32 *) p = color;
            break;

        default:break;
    }
}

void ::sdl_utils::draw_line(SDL_Surface *surface, int32_t x0, int32_t y0, int32_t x1, int32_t y1, Uint32 color)
{
    if ((y0 < m_clip_y0 && y1 < m_clip_y0) || (y0 > m_clip_y1 && y1 > m_clip_y1)
        || (x0 < m_clip_x0 && x1 < m_clip_x0) || (x0 > m_clip_x1 && x1 > m_clip_x1))
    {
        return;
    }

    if (x0 == x1)
    {
        if (y0 > y1) std::swap(y0, y1);

        for (int y = y0; y < y1; ++y)
            set_pixel(surface, x0, y, color);

        return;
    }
    else if (y0 == y1)
    {
        if (x0 > x1) std::swap(x0, x1);

        for (int x = x0; x < x1; ++x)
            set_pixel(surface, x, y0, color);

        return;
    }

    const bool steep = (abs(y1 - y0) > abs(x1 - x0));

    if (steep)
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    const float dx = x1 - x0;
    const float dy = (const float) abs(y1 - y0);

    float error = dx / 2.0f;
    const int ystep = (y0 < y1) ? 1 : -1;
    int y = (int) y0;

    const int maxX = (int) x1;

    for (int x = (int) x0; x < maxX; x++)
    {
        if (steep)
        {
            set_pixel(surface, y, x, color);
        }
        else
        {
            set_pixel(surface, x, y, color);
        }

        error -= dy;
        if (error < 0)
        {
            y += ystep;
            error += dx;
        }
    }
}
