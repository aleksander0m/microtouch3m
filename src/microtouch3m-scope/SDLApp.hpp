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

#ifndef MICROTOUCH_3M_SCOPE_SDLAPP_HPP
#define MICROTOUCH_3M_SCOPE_SDLAPP_HPP

#include <string>

#include "SDL.h"

class SDLApp
{
public:
    SDLApp(uint32_t width, uint32_t height, uint8_t bits_per_pixel, uint32_t flags, uint32_t fps_limit,
           bool verbose, bool vsync);
    virtual ~SDLApp();

    int exec();
    void enable_cursor(bool enable);
    uint32_t fps() const;
    void screenshot(const std::string &file_path);

protected:
    SDL_Surface *screen_surface() const;

    virtual void on_start() = 0;
    virtual void update(uint32_t delta_time) = 0;
    virtual void draw() = 0;
    virtual void on_event(const SDL_Event &event) = 0;

    int m_exit_code;
    bool m_exit;

private:
    const SDL_VideoInfo *m_video_info;
    SDL_Surface *m_screen_surface;

    uint32_t m_fps_limit;
    uint32_t m_fps;
    int m_fbdev;
    bool m_vsync;
    bool m_make_screenshot;
    std::string m_screenshot_path;
};

#endif // MICROTOUCH_3M_SCOPE_SDLAPP_HPP
