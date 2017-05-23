#include <iostream>
#include <stdexcept>

#include "Microtouch3MScopeApp.hpp"

const static uint32_t FPS_LIMIT = 60;

int main()
{
    try
    {
#if defined(IMX6) || defined(IMX51)
        int width = 0; int height = 0;
#else
        int width = 1280; int height = 800;
#endif

        Uint32 flags = SDL_SWSURFACE
#if defined(IMX6) || defined(IMX51)
            | SDL_FULLSCREEN
#endif
        ;

        Microtouch3MScopeApp sdlApp(width, height, flags, FPS_LIMIT);

#if defined(IMX6) || defined(IMX51)
        sdlApp.enable_cursor(false);
#endif

        return sdlApp.exec();
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;

        return 1;
    }
}
