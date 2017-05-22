#include "SDLUtils.hpp"

#include <iostream>

#include "SDL_endian.h"

void ::sdl_utils::set_clip_area(int x = 0, int y = 0, int width = 999999, int height = 999999)
{
    m_clip_x0 = x;
    m_clip_y0 = y;
    m_clip_x1 = m_clip_x0 + width;
    m_clip_y1 = m_clip_y0 + height;
}

void ::sdl_utils::set_pixel(SDL_Surface *surface, uint32_t x, uint32_t y, const Color &color)
{
    if (x >= surface->w || y >= surface->h) return;

    if (x < m_clip_x0 || x > m_clip_x1 || y < m_clip_y0 || y > m_clip_y1) return;

    const Uint32 col = SDL_MapRGB(surface->format, color.r, color.g, color.b);

    const int bpp = surface->format->BytesPerPixel;

    Uint8 *p = (Uint8 *) surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp)
    {
        case 1:
            *p = (Uint8) col;
            break;

        case 2:
            *(Uint16 *) p = (Uint16) col;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
                // bgr
                p[0] = (col >> 16) & 0xff;
                p[1] = (col >> 8) & 0xff;
                p[2] = col & 0xff;
            }
            else
            {
                // rgb
                p[0] = col & 0xff;
                p[1] = (col >> 8) & 0xff;
                p[2] = (col >> 16) & 0xff;
            }
            break;

        case 4:
            *(Uint32 *) p = col;
            break;

        default:break;
    }
}

void ::sdl_utils::draw_line(SDL_Surface *surface, int32_t x0, int32_t y0, int32_t x1, int32_t y1, const Color &color)
{
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
