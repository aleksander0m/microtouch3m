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

    uint8_t r, g, b;
};

#endif // MICROTOUCH_3M_SCOPE_COLOR_HPP
