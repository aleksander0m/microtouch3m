#ifndef MICROTOUCH_3M_SCOPE_SDLUTILS_HPP
#define MICROTOUCH_3M_SCOPE_SDLUTILS_HPP

#include "Color.hpp"

#include "SDL_video.h"

namespace sdl_utils
{
    void set_pixel(SDL_Surface *surface, uint32_t x, uint32_t y, const Color &color);
    void draw_line(SDL_Surface *surface, int32_t x0, int32_t y0, int32_t x1, int32_t y1, const Color &color);
    void set_clip_area(int x, int y, int width, int height);

    static int32_t m_clip_x0, m_clip_y0, m_clip_x1, m_clip_y1;
}


#endif // MICROTOUCH_3M_SCOPE_SDLUTILS_HPP
