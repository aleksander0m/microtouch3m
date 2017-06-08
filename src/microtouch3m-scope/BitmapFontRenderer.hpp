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
