#ifndef MICROTOUCH_3M_SCOPE_SDLAPP_HPP
#define MICROTOUCH_3M_SCOPE_SDLAPP_HPP

#include "SDL.h"

class SDLApp
{
public:
    SDLApp(uint32_t width, uint32_t height, uint8_t bits_per_pixel, uint32_t flags, uint32_t fps_limit, bool verbose);
    virtual ~SDLApp();

    int exec();
    void enable_cursor(bool enable);
    uint32_t fps() const;

protected:
    SDL_Surface *screen_surface() const;

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

};

#endif // MICROTOUCH_3M_SCOPE_SDLAPP_HPP
