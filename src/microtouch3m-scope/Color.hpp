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

#ifndef MICROTOUCH_3M_SCOPE_COLOR_HPP
#define MICROTOUCH_3M_SCOPE_COLOR_HPP

#include "SDL_video.h"

struct Color
{
    Color() : r(0), g(0), b(0)
    { }

    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 0xff) :
    r(red),
    g(green),
    b(blue)
    { }

    Uint32 map_rgb(const SDL_PixelFormat * const format) const
    {
        return SDL_MapRGB(format, r, g, b);
    }

    uint8_t r, g, b;
};

#endif // MICROTOUCH_3M_SCOPE_COLOR_HPP
