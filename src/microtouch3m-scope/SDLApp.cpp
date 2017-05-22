#include "SDLApp.hpp"

#include <iostream>
#include <stdexcept>
#include <cmath>

SDLApp::SDLApp(uint32_t width, uint32_t height, uint32_t flags, uint32_t fps_limit) :
    m_video_info(0),
    m_screen_surface(0),
    m_fps_limit(fps_limit),
    m_fps(0)
{
    std::cout << "SDL version: " << SDL_MAJOR_VERSION
              << "." << SDL_MINOR_VERSION
              << "." << SDL_PATCHLEVEL
              << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error(std::string("Video initialization failed: ") + SDL_GetError());
    }

    m_video_info = SDL_GetVideoInfo();

    if (!m_video_info)
    {
        throw std::runtime_error(std::string("Video query failed: ") + SDL_GetError());
    }

    std::cout << "Video mode resolution: " << m_video_info->current_w << "x" << m_video_info->current_h << std::endl
              << "hw_available: " << m_video_info->hw_available << std::endl
              << "blit_hw: " << m_video_info->blit_hw << std::endl
              << "blit_fill: " << m_video_info->blit_fill << std::endl
              << "video_mem: " << m_video_info->video_mem << "K" << std::endl
              << "BPP: " << (uint32_t) m_video_info->vfmt->BitsPerPixel << std::endl;

    if (width == 0 || height == 0)
    {
        std::cout << "Using video mode resolution as window size" << std::endl;

        width = (uint32_t) m_video_info->current_w;
        height = (uint32_t) m_video_info->current_h;
    }

    m_screen_surface = SDL_SetVideoMode(width, height, m_video_info->vfmt->BitsPerPixel, flags);

    if (m_screen_surface == 0)
    {
        throw std::runtime_error(std::string("Video mode set failed: ") + SDL_GetError());
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

        // sleep until next frame to keep CPU load sane

        {
            int32_t frame_time_remaining = (int32_t) round(1000.0 / m_fps_limit - (SDL_GetTicks() - iteration_start));

            if (frame_time_remaining > 0)
            {
                SDL_Delay((Uint32) frame_time_remaining);
            }
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

SDL_Surface *SDLApp::screen_surface() const
{
    return m_screen_surface;
}
