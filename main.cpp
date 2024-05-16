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
    while ((c = getopt(argc, argv, "h:w:H:c:d:r:f:o:l:L:s:S:p:")) != -1) {
        switch (c) {
            case 'w': { // Width
                int pixelsWide = (int) strtol(optarg, nullptr, 0);
                if (pixelsWide <= 0) {
                    std::cout << "Invalid width argument " << optarg << std::endl;
                    return 1;
                }
                std::cout << "Setting renderer width to " << pixelsWide << std::endl;
                rainbow_renderer->setPixelsWide(pixelsWide);
                break;
            }
            case 'h': { // Height
                int pixelsHigh = (int) strtol(optarg, nullptr, 0);
                if (pixelsHigh <= 0) {
                    std::cout << "Invalid height argument " << optarg << std::endl;
                    return 1;
                }
                std::cout << "Setting renderer height to " << pixelsHigh << std::endl;
                rainbow_renderer->setPixelsHigh(pixelsHigh);
                break;
            }
            case 'H': { // Starting hues
                int hue = (int) strtol(optarg, nullptr, 0);
                if (hue < 0 || hue > 360) {
                    std::cout << "Starting hue must be between 0 and 360 " << std::endl;
                    return 1;
                }
                std::cout << "Adding a starting hue of " << hue << std::endl;
                rainbow_renderer->addStartingHue(hue);
                break;
            }
            case 'l': { // Minimum luminosity
                float minLuminosity = std::stof(optarg);
                if (minLuminosity <= 0 || minLuminosity >= 1.0) {
                    std::cerr << "Minimum luminosity must be between 0 and 1 " << std::endl;
                    return 1;
                }
                std::cout << "Setting the minimum luminosity to " << minLuminosity << std::endl;
                rainbow_renderer->setMinimumLuminosity(minLuminosity);
                break;
            }
            case 'L': { // Maximum luminosity
                float maxLuminosity = strtof(optarg, nullptr);
                if (maxLuminosity <= 0 || maxLuminosity >= 1.0) {
                    std::cerr << "Minimum luminosity must be between 0 and 1" << std::endl;
                    return 1;
                }
                std::cout << "Setting the maximum luminosity to " << maxLuminosity << std::endl;
                rainbow_renderer->setMaximumLuminosity(maxLuminosity);
                break;
            }
            case 's': { // Minimum saturation
                float minSaturation = std::stof(optarg);
                if (minSaturation <= 0 || minSaturation >= 1.0) {
                    std::cerr << "Minimum saturation must be between 0 and 1 " << std::endl;
                    return 1;
                }
                std::cout << "Setting the minimum saturation to " << minSaturation << std::endl;
                rainbow_renderer->setMinimumSaturation(minSaturation);
                break;
            }
            case 'S': { // Maximum saturation
                float maxSaturation = strtof(optarg, nullptr);
                if (maxSaturation <= 0 || maxSaturation >= 1.0) {
                    std::cerr << "Maximum saturation must be between 0 and 1" << std::endl;
                    return 1;
                }
                std::cout << "Setting the maximum saturation to " << maxSaturation << std::endl;
                rainbow_renderer->setMaximumSaturation(maxSaturation);
                break;
            }
            case 'c': { // Colour depth
                int colourDepth = (int) strtol(optarg, nullptr, 0);
                if (colourDepth <= 0) {
                    std::cout << "Invalid colour depth argument " << optarg << std::endl;
                    return 1;
                }
                std::cout << "Setting the colour depth to " << colourDepth << std::endl;
                rainbow_renderer->setColourDepth(colourDepth);
                break;
            }
            case 'd': { // Colour difference function
                std::string diff_type = optarg;
                float (*difference_func)(const Colour &, const Colour &);
                if (diff_type == "lum") {
                    difference_func = getColourLuminosityDiff;
                } else if (diff_type == "colour" || diff_type == "color") {
                    difference_func = getColourAbsoluteDiff;
                } else if (diff_type == "natural") {
                    difference_func = getNaturalColourDiff;
                } else if (diff_type == "hue") {
                    difference_func = getColourHueDiff;
                } else {
                    std::cout << "Unknown difference type " << diff_type << std::endl;
                    return 1;
                }
                rainbow_renderer->setDifferenceFunction(difference_func);
                break;
            }
            case 'p': { // Starting positions
                std::string start_type_str = optarg;
                if (start_type_str == "center" || start_type_str == "centre") {
                    std::cout << "Setting start position to centre" << std::endl;
                    start_type = RainbowRenderer::START_TYPE_CENTRE;
                } else if (start_type_str == "corner") {
                    std::cout << "Setting start position too corner(s)" << std::endl;
                    start_type = RainbowRenderer::START_TYPE_CORNER;
                } else if (start_type_str == "horizontal") {
                    start_type = RainbowRenderer::START_TYPE_HORIZONTAL_LINE;
                } else if (start_type_str == "vertical") {
                    start_type = RainbowRenderer::START_TYPE_VERTICAL_LINE;
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
            case 'f': { // Fill mode
                std::string fill_mode_str = optarg;
                if (fill_mode_str == "edge") { // Fastest
                    std::cout << "Setting fill mode to \"edge\"" << std::endl;
                    fill_mode = RainbowRenderer::FILL_MODE_EDGE;
                } else if (fill_mode_str == "neighbour" || fill_mode_str == "neighbor") { // Slower, smoother
                    std::cout << "Setting fill mode to \"closest neighbour\"" << std::endl;
                    fill_mode = RainbowRenderer::FILL_MODE_NEIGHBOUR;
                } else if (fill_mode_str == "average") { // Really slow, "coral" effect
                    std::cout << "Setting fill mode to \"average neighbour\"" << std::endl;
                    fill_mode = RainbowRenderer::FILL_MODE_NEIGHBOUR_AVERAGE;
                } else {
                    std::cerr << "Unknown fill mode " << fill_mode_str << std::endl;
                    return 1;
                }
                rainbow_renderer->setFillMode(fill_mode);
                break;
            }
            case 'o': { // Initial colour ordering
                std::string colour_order_string = optarg;
                for (char &it: colour_order_string) {
                    switch (it) {
                        case 'r':
                        case 'R':
                            rainbow_renderer->addColourOrder({COLOUR_ORDER_RANDOM, false});
                            std::cout << "Randomising colours" << std::endl;
                            break;
                        case 'h':
                        case 'H':
                            rainbow_renderer->addColourOrder({COLOUR_ORDER_HUE, it == 'H'});
                            std::cout << "Sorting colours by hue " << (it == 'h' ? "ascending" : "descending")
                                      << std::endl;
                            break;
                        case 's':
                        case 'S':
                            rainbow_renderer->addColourOrder({COLOUR_ORDER_SAT, it == 'S'});
                            std::cout << "Sorting colours by saturation " << (it == 's' ? "ascending" : "descending")
                                      << std::endl;
                            break;
                        case 'l':
                        case 'L':
                            rainbow_renderer->addColourOrder({COLOUR_ORDER_LUM, it == 'L'});
                            std::cout << "Sorting colours by luminosity " << (it == 'l' ? "ascending" : "descending")
                                      << std::endl;
                            break;
                        default:
                            std::cerr << "Unknown colour ordering " << it << std::endl;
                            exit(1);
                    }
                }
                break;
            }
            case 'r': {
                int seed = (int) strtol(optarg, nullptr, 0);
                if (seed <= 0) {
                    std::cout << "Invalid seed argument " << optarg << std::endl;
                    return 1;
                }
                rainbow_renderer->setSeed(seed);
                break;
            }
            case '?': {
                if (optopt == 'h' || optopt == 'w' || optopt == 'c' || optopt == 'd' || optopt == 'f' ||
                    optopt == 'p') {
                    std::cerr << "Option -" << char(optopt) << " requires an argument" << std::endl;
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