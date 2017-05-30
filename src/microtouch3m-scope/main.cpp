#include <iostream>
#include <cmath>
#include <sstream>
#include <stdexcept>

#include <getopt.h>

#include "M3MScopeApp.hpp"

const static uint32_t FPS_LIMIT = 60;

static void print_help();

int main(int argc, char *argv[])
{
    bool verbose = false;
    int print_fps = 0;
    int m3m_log = 0;
    uint32_t scale = 10000000;
    uint16_t bits_per_pixel = 0;

    const option long_options[] = {
    { "help",      no_argument,       0,          'h' },
    { "verbose",   no_argument,       0,          'v' },
    { "print-fps", no_argument,       &print_fps, 1 },
    { "m3m-log",   no_argument,       &m3m_log,   1 },
    { "scale",     required_argument, 0,          's' },
    { "bpp",       required_argument, 0,          'b' },
    { 0, 0,                           0,          0 }
    };

    int idx;
    int iarg = 0;

    while (iarg != -1)
    {
        iarg = getopt_long(argc, argv, "hvs:b:", long_options, &idx);

        switch (iarg)
        {
            default:
                break;

            case '?':
            case 'h':
                print_help();
                return 0;

            case 'v':
                verbose = true;
                break;

            case 's':
            {
                bool err = false;
                uint32_t k = 0;
                scale = 0;

                const std::string arg(optarg);

                for (std::string::const_iterator cit = arg.begin(); cit != arg.end(); ++cit)
                {
                    if (*cit >= '0' && *cit <= '9')
                    {
                        scale = scale * 10 + (*cit - '0');
                        ++k;
                    }
                    else
                    {
                        switch (*cit)
                        {
                            case 'K':
                                scale *= 1000;
                                k += 3;
                                break;
                            case 'M':
                                scale *= 1000000;
                                k += 6;
                                break;
                            default:
                                err = true;
                                break;
                        }
                    }

                    if (k > 9 || err)
                    {
                        err = true;
                        break;
                    }
                }

                if (err || scale < 10000)
                {
                    std::cerr << "Invalid scale argument: " << arg << std::endl;
                    return 1;
                }
            }
                break;

            case 'b':
            {
                std::istringstream(optarg) >> bits_per_pixel;

                if (bits_per_pixel != 0 && (bits_per_pixel < 8 || bits_per_pixel > 32))
                {
                    std::cerr << "Invalid bpp argument: " << optarg << std::endl;
                    return 1;
                }
            }

                break;
        }
    }

    try
    {
#if defined(IMX51)
        uint32_t width = 0; uint32_t height = 0;
#else
        uint32_t width = 1280; uint32_t height = 800;
#endif

        Uint32 flags = SDL_SWSURFACE
#if defined(IMX51)
            | SDL_FULLSCREEN
#endif
        ;

        M3MScopeApp sdlApp(width, height, (uint8_t) bits_per_pixel, flags, FPS_LIMIT, verbose, (bool) m3m_log);

#if defined(IMX51)
        sdlApp.enable_cursor(false);
#endif

        sdlApp.set_print_fps((bool) print_fps);
        sdlApp.set_scale(scale);

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
              << "  -v, --verbose        Be verbose." << std::endl
              << "  -h, --help           Show help." << std::endl
              << "      --print-fps      Print FPS each second." << std::endl
              << "      --m3m-log        Enable microtouch3m log." << std::endl
              << "  -s, --scale          Min/max value of chart in [10K, 999999999] range." << std::endl
              << "                       Examples of acceptable values: 100, 5K, 6M, etc." << std::endl
              << "  --bpp                Bits per pixel. Default: 0 (autodetect)." << std::endl
              << std::endl;
}
