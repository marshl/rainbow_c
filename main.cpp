#include <iostream>
#include <cstdlib>
#include <list>
#include <random>
#include <ctime>
#include <unistd.h>

#include "colour.h"
#include "rainbow_renderer.h"

/// Program entrypoint
/// \param argc
/// \param argv
/// \return
int main(int argc, char *argv[]) {

    opterr = 0;
    RainbowRenderer::StartType start_type;
    RainbowRenderer::FillMode fill_mode;

    auto rainbow_renderer = new RainbowRenderer();

    int c;
    while ((c = getopt(argc, argv, "h:w:c:d:s:f:")) != -1) {
        switch (c) {
            case 'w': {
                int pixelsWide = (int) strtol(optarg, nullptr, 0);
                if (pixelsWide <= 0) {
                    std::cout << "Invalid width argument " << optarg << std::endl;
                    return 1;
                }
                rainbow_renderer->setPixelsWide(pixelsWide);
                break;
            }
            case 'h': {
                int pixelsHigh = (int) strtol(optarg, nullptr, 0);
                if (pixelsHigh <= 0) {
                    std::cout << "Invalid height argument " << optarg << std::endl;
                    return 1;
                }
                rainbow_renderer->setPixelsHigh(pixelsHigh);
                break;
            }
            case 'c': {
                int colour_depth = (int) strtol(optarg, nullptr, 0);
                if (colour_depth <= 0) {
                    std::cout << "Invalid colour depth argument " << optarg << std::endl;
                    return 1;
                }
                rainbow_renderer->setColourDepth(colour_depth);
                break;
            }
            case 'd': {
                std::string diff_type = optarg;
                float (*difference_func)(const Colour *const, const Colour *const);
                if (diff_type == "lum") {
                    difference_func = getColourLuminosityDiff;
                } else if (diff_type == "colour" || diff_type == "color") {
                    difference_func = getColourAbsoluteDiff;
                } else if (diff_type == "hue") {
                    difference_func = getColourHueDiff;
                } else {
                    std::cout << "Unknown difference type " << diff_type << std::endl;
                    return 1;
                }
                rainbow_renderer->setDifferenceFunction(difference_func);
                break;
            }
            case 's': {
                std::string start_type_str = optarg;
                if (start_type_str == "center" || start_type_str == "centre") {
                    start_type = RainbowRenderer::START_TYPE_CENTRE;
                } else if (start_type_str == "corner") {
                    start_type = RainbowRenderer::START_TYPE_CORNER;
                } else if (start_type_str == "edge") {
                    start_type = RainbowRenderer::START_TYPE_EDGE;
                } else if (start_type_str == "random") {
                    start_type = RainbowRenderer::START_TYPE_RANDOM;
                } else if (start_type_str == "circle") {
                    start_type = RainbowRenderer::START_TYPE_CIRCLE;
                } else {
                    std::cout << "Unknown start type " << start_type_str << std::endl;
                    return 1;
                }
                rainbow_renderer->setStartType(start_type);
                break;
            }
            case 'f': {
                std::string fill_mode_str = optarg;
                if (fill_mode_str == "edge") {
                    fill_mode = RainbowRenderer::FILL_MODE_EDGE;
                } else if (fill_mode_str == "neighbour") {
                    fill_mode = RainbowRenderer::FILL_MODE_NEIGHBOUR;
                } else if (fill_mode_str == "average") {
                    fill_mode = RainbowRenderer::FILL_MODE_NEIGHBOUR_AVERAGE;
                } else {
                    std::cerr << "Unknown fill mode " << fill_mode_str << std::endl;
                    return 1;
                }
                rainbow_renderer->setFillMode(fill_mode);
                break;
            }
            case '?': {
                if (optopt == 'h' || optopt == 'w' || optopt == 'c' || optopt == 'd' || optopt == 'f') {
                    std::cerr << "Option -" << optopt << " requires an argument" << std::endl;
                } else if (isprint(optopt)) {
                    std::cerr << "Unknown option -" << char(optopt) << std::endl;
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    std::cerr << "Unknown option character" << optopt << std::endl;
                }
                return 1;
            }
            default:
                abort();
        }
    }

    for (; optind < argc; optind++) {
        int num_start_points = (int) strtol(argv[optind], nullptr, 0);
        if (num_start_points > 0) {
            rainbow_renderer->setNumStartPoints(num_start_points);
        }
    }

    time_t start_time = time(nullptr);
    rainbow_renderer->init();
    rainbow_renderer->fill();
    time_t end_time = time(nullptr);
    std::cout << "Completed in " << (end_time - start_time) << "s" << std::endl;

    rainbow_renderer->writeToFile("output.bmp");

    delete rainbow_renderer;
    return 0;
}