#include <iostream>
#include <stdexcept>

#include <getopt.h>

#include "M3MScopeApp.hpp"

const static uint32_t FPS_LIMIT = 60;
const static uint8_t BITS_PER_PIXEL = 16;

static void print_help();

int main(int argc, char *argv[])
{
    const option long_options[] = {
    { "help",    no_argument, 0, 'h' },
    { "verbose", no_argument, 0, 'v' },
    { 0, 0,                   0, 0 }
    };

    int idx;
    int iarg = 0;

    bool verbose = false;

    while (iarg != -1)
    {
        iarg = getopt_long(argc, argv, "hv", long_options, &idx);

        switch (iarg)
        {
            case 'h':
                print_help();
                return 0;

            case 'v':
                verbose = true;
                break;
        }
    }

    try
    {
#if defined(IMX51)
        int width = 0; int height = 0;
#else
        int width = 1280; int height = 800;
#endif

        Uint32 flags = SDL_SWSURFACE
#if defined(IMX51)
            | SDL_FULLSCREEN
#endif
        ;

        M3MScopeApp sdlApp(width, height, BITS_PER_PIXEL, flags, FPS_LIMIT, verbose);

#if defined(IMX51)
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

void print_help()
{
    std::cout << std::endl
              << "  -v, --verbose   Be verbose. Will enable microtouch3m log." << std::endl
              << "  -h, --help      Show help." << std::endl
              << std::endl;
}
