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

#ifndef MICROTOUCH_3M_SCOPE_BITMAPFONTRENDERER_HPP
#define MICROTOUCH_3M_SCOPE_BITMAPFONTRENDERER_HPP

#include <string>

#include "SDL_image.h"

class BitmapFontRenderer
{
public:
    BitmapFontRenderer();
    virtual ~BitmapFontRenderer();

    void draw(SDL_Surface *surface, int32_t x, int32_t y, const std::string &text,
              bool align_right = false, bool align_bottom = false);

    uint32_t text_width(const std::string &text) const;
    uint32_t text_height(const std::string &text) const;

private:

    void map_rect_for_letter(SDL_Rect *rect, char letter);

    static const int s_font_w;
    static const int s_font_h;

    SDL_Surface *m_font_surface;
};

#endif // MICROTOUCH_3M_SCOPE_BITMAPFONTRENDERER_HPP
