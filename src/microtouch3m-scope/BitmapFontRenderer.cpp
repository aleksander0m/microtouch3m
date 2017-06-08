#include <algorithm>
#include <sstream>
#include "BitmapFontRenderer.hpp"

/* Ignore ISO C++ warnings when including font xpm files */
#pragma GCC diagnostic ignored "-Wwrite-strings"

#if 0

#include "font_25x32.xpm"

const int BitmapFontRenderer::s_font_w = 25;
const int BitmapFontRenderer::s_font_h = 32;

#define FONT_NAME font_25x32_xpm

#else

#include "font_13x16.xpm"

const int BitmapFontRenderer::s_font_w = 13;
const int BitmapFontRenderer::s_font_h = 16;

#define FONT_NAME font_13x16_xpm

#endif

/* Recover ISO C++ warnings */
#pragma GCC diagnostic warning "-Wwrite-strings"

BitmapFontRenderer::BitmapFontRenderer()
{
    m_font_surface = IMG_ReadXPMFromArray(FONT_NAME);
}

BitmapFontRenderer::~BitmapFontRenderer()
{
    SDL_FreeSurface(m_font_surface);
}

void BitmapFontRenderer::draw(SDL_Surface *surface, int32_t x, int32_t y, const std::string &text,
                              bool align_right, bool align_bottom)
{
    if (text.empty()) return;

    SDL_Rect src;
    src.w = (Uint16) s_font_w; src.h = (Uint16) s_font_h;

    SDL_Rect dst;
    dst.w = (Uint16) s_font_w; dst.h = (Uint16) s_font_h;

    int32_t origin_x = align_right ? x - s_font_w : x;
    dst.x = origin_x;
    dst.y = align_bottom ? y - s_font_h : y;

    if (align_bottom)
    {
        dst.y -= s_font_h * std::count(text.begin(), text.end(), '\n');
    }

    // start/end of line
    size_t sol = 0;
    size_t eol = text.find_first_of('\n', sol);

    if (eol == std::string::npos)
    {
        eol = text.size();
    }

    while (sol < text.size())
    {
        for (size_t i = sol; i < eol; ++i)
        {
            const char letter = text[align_right ? eol - i + sol - 1 : i];

            map_rect_for_letter(&src, letter);

            SDL_BlitSurface(m_font_surface, &src, surface, &dst);

            dst.x += align_right ? -s_font_w : s_font_w;
        }

        sol = eol + 1;

        if ((eol = text.find_first_of('\n', sol)) == std::string::npos)
        {
            eol = text.size();
        }

        dst.x = origin_x;
        dst.y += s_font_h;
    }
}

uint32_t BitmapFontRenderer::text_width(const std::string &text) const
{
    std::istringstream iss(text);
    std::string line;
    size_t ret = 0;

    while (std::getline(iss, line))
    {
        ret = std::max(ret, line.length());
    }

    return (uint32_t) (ret * s_font_w);
}

uint32_t BitmapFontRenderer::text_height(const std::string &text) const
{
    if (text.empty()) return 0;

    return (uint32_t) ((std::count(text.begin(), text.end(), '\n') + (text[text.length() - 1] != '\n')) * s_font_h);
}

void BitmapFontRenderer::map_rect_for_letter(SDL_Rect *rect, char letter)
{
    if (!rect) return;

    letter = (char) std::toupper(letter);

    if (letter >= 65 && letter <= 90)
    {
        rect->x = s_font_w * (letter - 65);
        rect->y = s_font_h * 0;
    }
    else if (letter >= 48 && letter <= 57)
    {
        rect->x = s_font_w * (letter - 48);
        rect->y = s_font_h * 1;
    }
    else
    {
        switch (letter)
        {
            case ':':
                rect->x = s_font_w * 10;
                rect->y = s_font_h * 1;
                break;

            case '-':
                rect->x = s_font_w * 11;
                rect->y = s_font_h * 1;
                break;

            case '.':
                rect->x = s_font_w * 12;
                rect->y = s_font_h * 1;
                break;

            case '+':
                rect->x = s_font_w * 13;
                rect->y = s_font_h * 1;
                break;

            // whitespace
            default:
                rect->x = s_font_w * 27;
                rect->y = 0;
                break;
        }
    }
}
