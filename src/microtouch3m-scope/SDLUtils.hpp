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

#ifndef MICROTOUCH_3M_SCOPE_SDLUTILS_HPP
#define MICROTOUCH_3M_SCOPE_SDLUTILS_HPP

#include <string>

#include "Color.hpp"

#include "SDL_video.h"

namespace sdl_utils
{
    void set_pixel_no_clip(SDL_Surface *surface, uint32_t x, uint32_t y, Uint32 color);
    void set_pixel(SDL_Surface *surface, uint32_t x, uint32_t y, Uint32 color);
    void draw_line(SDL_Surface *surface, int32_t x0, int32_t y0, int32_t x1, int32_t y1, Uint32 color);
    void set_clip_area(int x, int y, int width, int height);
    void save_buffer(SDL_Surface *surface, const std::string &file_name);

    static int32_t m_clip_x0, m_clip_y0, m_clip_x1, m_clip_y1;
}


#endif // MICROTOUCH_3M_SCOPE_SDLUTILS_HPP
