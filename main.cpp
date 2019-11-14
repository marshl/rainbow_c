#include <iostream>
#include <cstdlib>
#include <list>
#include <algorithm>
#include <random>
#include <ctime>
#include <unistd.h>

#include "bmp.h"

#include "colour.h"
#include "pixel.h"
#include "point.h"

auto rng = std::default_random_engine(std::random_device{}());

enum StartType {
    START_TYPE_CENTRE,
    START_TYPE_CORNER,
    START_TYPE_RANDOM,
    START_TYPE_EDGE
};

int PIXELS_WIDE = 256;
int PIXELS_HIGH = 256;
int COLOUR_DEPTH = 256;

std::vector<Colour> colours;
std::vector<Pixel> pixels;
std::list<Point> available_edges;
std::vector<Point> neighbours;
long colour_index = 0;

/// Get the pixel at the given x and y coordinates
/// \param x The x coordinate
/// \param y The y coordinate
/// \return The pixel at that position
Pixel *getPixel(int x, int y) {
    return &pixels[y * PIXELS_WIDE + x];
}

/// Gets the pixel at the given point
/// \param point The point
/// \return The pixel at that point
Pixel *getPixelAtPoint(Point &point) {
    return &pixels[point.y * PIXELS_WIDE + point.x];
}

/// Gets the neighbour positions of the given point (points over the edge of the board aren ot returned)
/// \param point The point to find the neighbours for
/// \return The number of neighbours
int getNeighboursOfPoint(Point point) {
    neighbours.clear();
    for (int y = -1; y <= 1; ++y) {
        if (y + point.y < 0 || y + point.y >= PIXELS_HIGH) {
            continue;
        }

        for (int x = -1; x <= 1; ++x) {
            if ((x == 0 && y == 0) || x + point.x < 0 || x + point.x >= PIXELS_WIDE) {
                continue;
            }

            neighbours.emplace_back(Point(x + point.x, y + point.y));
        }
    }
    return neighbours.size();
}

/// Fills the list of random colours
/// \param colour_depth The number of each unique colours in each channel
/// \return The total number of colours
int fillColours(int colour_depth) {
    for (int r = 0; r < colour_depth; ++r) {
        for (int g = 0; g < colour_depth; ++g) {
            for (int b = 0; b < colour_depth; ++b) {
                colours.emplace_back(Colour(r * 255 / (colour_depth - 1),
                                            g * 255 / (colour_depth - 1),
                                            b * 255 / (colour_depth - 1)));
            }
        }
    }
    std::shuffle(std::begin(colours), std::end(colours), rng);
    return colours.size();
}



/// Fills the pixel at the given point
/// \param point The pointto place the pixel at
void fillPoint(Point &point) {
    available_edges.push_back(point);
    Pixel *centre_pixel = getPixelAtPoint(point);
    centre_pixel->colour = colours[colour_index];
    centre_pixel->is_filled = true;
    ++colour_index;
}

/// Program entrypoint
/// \param argc
/// \param argv
/// \return
int main(int argc, char *argv[]) {

    auto difference_func = getColourAbsoluteDiff;
    opterr = 0;
    StartType start_type = START_TYPE_CENTRE;
    int num_start_points = 1;

    int c;
    while ((c = getopt(argc, argv, "h:w:c:d:s:")) != -1)
        switch (c) {
            case 'w':
                PIXELS_WIDE = (int) strtol(optarg, nullptr, 0);
                if (PIXELS_WIDE <= 0) {
                    std::cout << "Invalid width argument " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'h':
                PIXELS_HIGH = (int) strtol(optarg, nullptr, 0);
                if (PIXELS_HIGH <= 0) {
                    std::cout << "Invalid height argument " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'c':
                COLOUR_DEPTH = (int) strtol(optarg, nullptr, 0);
                if (COLOUR_DEPTH <= 0) {
                    std::cout << "Invalid colour depth argument " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'd': {
                std::string diff_type = optarg;
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
                break;
            }
            case 's': {
                std::string start_type_str = optarg;
                if (start_type_str == "center" || start_type_str == "centre") {
                    start_type = START_TYPE_CENTRE;
                } else if (start_type_str == "corner") {
                    start_type = START_TYPE_CORNER;
                } else if (start_type_str == "edge") {
                    start_type = START_TYPE_EDGE;
                } else if (start_type_str == "random") {
                    start_type = START_TYPE_RANDOM;
                } else {
                    std::cout << "Unknown start type " << start_type_str << std::endl;
                    return 1;
                }
                break;
            }
            case '?':
                if (optopt == 'h' || optopt == 'w' || optopt == 'c' || optopt == 'd') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                }
                return 1;

            default:
                abort();
        }
    for (; optind < argc; optind++) {
        num_start_points = (int) strtol(argv[optind], nullptr, 0);
    }

    time_t start_time = time(nullptr);
    pixels.resize(PIXELS_WIDE * PIXELS_HIGH);
    long COLOUR_COUNT = fillColours(COLOUR_DEPTH);
    std::vector<Point> possible_start_points;
    if (num_start_points <= 0) {
        num_start_points = INT32_MAX;
    }

    switch (start_type) {
        case START_TYPE_CENTRE: {
            possible_start_points.emplace_back(Point(PIXELS_WIDE / 2, PIXELS_HIGH / 2));
            break;
        }
        case START_TYPE_CORNER: {
            possible_start_points.emplace_back(Point(0, 0));
            possible_start_points.emplace_back(Point(PIXELS_WIDE - 1, 0));
            possible_start_points.emplace_back(Point(0, PIXELS_HIGH - 1));
            possible_start_points.emplace_back(Point(PIXELS_WIDE - 1, PIXELS_HIGH - 1));
            break;
        }
        case START_TYPE_EDGE:
        case START_TYPE_RANDOM:
            for (int y = 0; y < PIXELS_HIGH; ++y) {
                for (int x = 0; x < PIXELS_WIDE; ++x) {
                    if (start_type == START_TYPE_RANDOM ||
                        (x == 0 || x == PIXELS_WIDE - 1 || y == 0 || y == PIXELS_HIGH - 1)) {
                        possible_start_points.emplace_back(Point(x, y));
                    }
                }
            }
            break;
    }
    std::shuffle(possible_start_points.begin(), possible_start_points.end(), rng);
    for (int i = 0; i < possible_start_points.size() && i < num_start_points; ++i) {
        fillPoint(possible_start_points[i]);
    }

    while (true) {
        if (available_edges.empty() || colour_index == COLOUR_COUNT) {
            std::cout << "Out of edges or colours" << std::endl;
            break;
        }
        Colour *current_colour = &colours[colour_index];
        Point best_point;
        int best_difference = INT32_MAX;
        for (auto &available_edge : available_edges) {
            int diff = difference_func(current_colour, &getPixelAtPoint(available_edge)->colour);
            if (diff < best_difference) {
                best_point = available_edge;
                best_difference = diff;
            }
        }

        int neighbour_count = getNeighboursOfPoint(best_point);
        std::shuffle(std::begin(neighbours), std::end(neighbours), rng);
        int neighbour_index = 0;
        for (; neighbour_index < neighbour_count; ++neighbour_index) {
            Point &neighbour = neighbours[neighbour_index];
            Pixel *neighbour_pixel = getPixelAtPoint(neighbour);
            if (neighbour_pixel->is_filled) {
                continue;
            }
            neighbour_pixel->colour = *current_colour;
            neighbour_pixel->is_filled = true;
            available_edges.push_back(neighbour);
            ++colour_index;

            if (colour_index % 1000 == 0) {
                std::cout << colour_index << ": " << available_edges.size() << " ("
                          << ((float) colour_index / float(PIXELS_HIGH * PIXELS_WIDE) * 100) << "%)" << std::endl;
            }
            break;
        }

        if (neighbour_index == neighbour_count) {
            available_edges.remove(best_point);
        }
    }
    time_t end_time = time(nullptr);
    std::cout << "Completed in " << (end_time - start_time) << "s" << std::endl;

    BMP bmp = BMP(PIXELS_WIDE, PIXELS_HIGH, false);
    for (int y = 0; y < PIXELS_HIGH; ++y) {
        for (int x = 0; x < PIXELS_WIDE; ++x) {
            Pixel *pixel = getPixel(x, y);
            bmp.set_pixel(x, y, pixel->colour.b, pixel->colour.g, pixel->colour.r);
        }
    }
    bmp.write("output.bmp");

    return 0;
}