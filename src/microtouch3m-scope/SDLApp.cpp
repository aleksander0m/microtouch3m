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

#include "SDLApp.hpp"

#include <iostream>
#include <stdexcept>
#include <cmath>

#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "SDLUtils.hpp"

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)
#endif

SDLApp::SDLApp(uint32_t width, uint32_t height, uint8_t bits_per_pixel, uint32_t flags, uint32_t fps_limit,
               bool verbose, bool vsync) :
    m_video_info(0),
    m_screen_surface(0),
    m_fps_limit(fps_limit),
    m_fps(0),
    m_fbdev(-1),
    m_vsync(vsync),
    m_make_screenshot(false)
{
    if (verbose)
    {
        std::cout << "SDL version: " << SDL_MAJOR_VERSION
                  << "." << SDL_MINOR_VERSION
                  << "." << SDL_PATCHLEVEL
                  << std::endl << std::endl;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error(std::string("Video initialization failed: ") + SDL_GetError());
    }

    m_video_info = SDL_GetVideoInfo();

    if (!m_video_info)
    {
        throw std::runtime_error(std::string("Video query failed: ") + SDL_GetError());
    }

    if (verbose)
    {
        std::cout << "Video mode" << std::endl
                  << "\tresolution: \t" << m_video_info->current_w << "x" << m_video_info->current_h << std::endl
                  << "\thw_available: \t" << m_video_info->hw_available << std::endl
                  << "\tblit_hw: \t" << m_video_info->blit_hw << std::endl
                  << "\tblit_fill: \t" << m_video_info->blit_fill << std::endl
                  << "\tvideo_mem: \t" << m_video_info->video_mem << "K" << std::endl
                  << "\tBPP: \t" << (uint32_t) m_video_info->vfmt->BitsPerPixel << std::endl
                  << std::endl;
    }

    if (width == 0 || height == 0)
    {
        if (verbose)
        {
            std::cout << "Using video mode resolution as window size" << std::endl;
        }

        width = (uint32_t) m_video_info->current_w;
        height = (uint32_t) m_video_info->current_h;
    }

    m_screen_surface = SDL_SetVideoMode(width, height, bits_per_pixel, flags);

    if (verbose)
    {
        std::cout << "Screen surface" << std::endl
                  << "\tResolution: \t" << m_screen_surface->w << "x" << m_screen_surface->h << std::endl
                  << "\tBPP: \t" << (uint32_t) m_screen_surface->format->BitsPerPixel << std::endl
                  << std::endl;
    }

    if (m_screen_surface == 0)
    {
        throw std::runtime_error(std::string("Video mode set failed: ") + SDL_GetError());
    }

    if ((m_fbdev = open("/dev/fb0", O_RDWR)) < 0)
    {
        std::cerr << "Couldn't read /dev/fb0. VSYNC disabled." << std::endl;
        m_vsync = false;
    }
}

SDLApp::~SDLApp()
{
}

int SDLApp::exec()
{
    m_fps = 0;
    uint32_t frame_count = 0;
    Uint32 last_fps_ticks = SDL_GetTicks();
    Uint32 last_iteration_start = SDL_GetTicks();

    m_exit_code = 0;
    m_exit = false;

    on_start();

    while (!m_exit)
    {
        Uint32 iteration_start = SDL_GetTicks();

        // poll events

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            on_event(event);

            switch (event.type)
            {
                case SDL_QUIT:
                    m_exit = true;
                    break;

                default:
                    break;
            }
        }

        // update

        update(iteration_start - last_iteration_start);

        // draw

        draw();

        ++frame_count;

        // calculate FPS

        if (SDL_GetTicks() - last_fps_ticks >= 1000)
        {
            m_fps = frame_count;
            frame_count = 0;
            last_fps_ticks = SDL_GetTicks();
        }

        // flip after VSYNC

        if (m_vsync)
        {
            ioctl(m_fbdev, FBIO_WAITFORVSYNC, 0);
        }

        SDL_Flip(screen_surface());

        // screenshot

        if (m_make_screenshot)
        {
            std::cout << std::endl << "Writing screenshot to \"" << m_screenshot_path << "\" ..." << std::endl;
            sdl_utils::save_buffer(m_screen_surface, m_screenshot_path);
            std::cout << "Done." << std::endl;

            m_make_screenshot = false;
        }

        // sleep until next frame to keep CPU load sane

        int32_t frame_time_remaining = (int32_t) round(1000.0 / m_fps_limit - (SDL_GetTicks() - iteration_start));

        if (frame_time_remaining > 0)
        {
            SDL_Delay((Uint32) frame_time_remaining);
        }

        last_iteration_start = iteration_start;
    }

    SDL_Quit();

    return m_exit_code;
}

void SDLApp::enable_cursor(bool enable)
{
    SDL_ShowCursor(enable ? SDL_ENABLE : SDL_DISABLE);
}

uint32_t SDLApp::fps() const
{
    return m_fps;
}

void SDLApp::screenshot(const std::string &file_path)
{
    m_make_screenshot = true;
    m_screenshot_path = file_path;
}

SDL_Surface *SDLApp::screen_surface() const
{
    return m_screen_surface;
}
