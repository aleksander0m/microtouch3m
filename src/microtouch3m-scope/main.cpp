#include <iostream>
#include <cmath>
#include <sstream>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedImportStatement"

#include <stdexcept>

#pragma clang diagnostic pop

#include <getopt.h>

#include "M3MScopeApp.hpp"

static void print_help();

enum Options
{
    OPT_FPS_LIMIT = 1000
};

int main(int argc, char *argv[])
{
    bool verbose = false;
    int print_fps = 0;
    int m3m_log = 0;
    uint32_t scale = 10000000;
    uint16_t bits_per_pixel = 16;
    uint32_t samples = 100;
    uint32_t fps_limit = 1000;
    int four_charts = 0;
    int no_vsync = 0;

    const option long_options[] = {
    { "help",        no_argument,       0,            'h' },
    { "verbose",     no_argument,       0,            'v' },
    { "print-fps",   no_argument,       &print_fps,   1 },
    { "m3m-log",     no_argument,       &m3m_log,     1 },
    { "scale",       required_argument, 0,            's' },
    { "bpp",         required_argument, 0,            'b' },
    { "samples",     required_argument, 0,            'k' },
    { "fps-limit",   required_argument, 0,            OPT_FPS_LIMIT },
    { "four-charts", no_argument,       &four_charts, 1 },
    { "no-vsync",    no_argument,       &no_vsync,    1 },
    { 0, 0,                             0,            0 }
    };

    int idx;
    int iarg = 0;

    while (iarg != -1)
    {
        iarg = getopt_long(argc, argv, "hvs:b:k:", long_options, &idx);

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
                std::istringstream(optarg) >> bits_per_pixel;

                if (bits_per_pixel != 0 && (bits_per_pixel < 8 || bits_per_pixel > 32))
                {
                    std::cerr << "Invalid bpp argument: " << optarg << std::endl;
                    return 1;
                }
                break;

            case 'k':
                std::istringstream(optarg) >> samples;

                if (samples < 2 || samples > 10000)
                {
                    std::cerr << "Invalid samples argument: " << optarg << std::endl;
                    return 1;
                }
                break;

            case OPT_FPS_LIMIT:
                std::istringstream(optarg) >> fps_limit;

                if (fps_limit < 1 || fps_limit > 1000)
                {
                    std::cerr << "Invalid fps-limit argument: " << optarg << std::endl;
                    return 1;
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

        M3MScopeApp sdlApp(width, height, (uint8_t) bits_per_pixel, flags, fps_limit, verbose, !no_vsync,
                           (bool) m3m_log, samples,
                           four_charts ? M3MScopeApp::CHART_MODE_FOUR : M3MScopeApp::CHART_MODE_ONE);

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
              << "  -s, --scale          Min/max value of chart in [10K; 999999999] range." << std::endl
              << "                       Examples of acceptable values: 100, 5K, 6M, etc." << std::endl
              << "  --bpp                Bits per pixel. Default: 16." << std::endl
              << "  -k, --samples        Number of samples in charts. Values in range [2; 10000] are accepted." << std::endl
              << "  --fps-limit          FPS limit." << std::endl
              << "  --four-charts        Draw four charts." << std::endl
              << "  --no-vsync           Disable VSYNC." << std::endl
              << std::endl;
}
