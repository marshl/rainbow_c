#include <iostream>
#include <cstdlib>
#include <list>
#include <algorithm>
#include <random>
#include <ctime>
#include <unistd.h>

#include "bmp.h"

auto rng = std::default_random_engine(std::random_device{}());

int PIXELS_WIDE = 256;
int PIXELS_HIGH = 256;
int COLOUR_DEPTH = 256;

int getHue(int red, int green, int blue) {
    int fmin = std::min(std::min(red, green), blue);
    int fmax = std::max(std::max(red, green), blue);

    if (fmin == fmax) {
        return 0;
    }

    float hue = 0;
    if (fmax == red) {
        hue = float(green - blue) / float(fmax - fmin);
    } else if (fmax == green) {
        hue = 2 + float(blue - red) / float(fmax - fmin);
    } else {
        hue = 4 + float(red - green) / float(fmax - fmin);
    }

    hue = hue * 60.0f;
    if (hue < 0.0f) {
        hue = hue + 360.0f;
    }
    return int(hue);
}

int getLum(int r, int g, int b) {
    return (std::max(r, std::max(g, b)) + std::min(r, std::min(g, b))) / 2;
}

struct Colour {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    int hue;
    int lum;

    Colour() { this->r = this->g = this->b = this->hue = this->lum = 0; }

    Colour(uint8_t _r, uint8_t _g, uint8_t _b) {
        this->r = _r;
        this->g = _g;
        this->b = _b;
        this->hue = getHue(_r, _g, _b);
        this->lum = getLum(_r, _g, _b);
    }
};

std::ostream &operator<<(std::ostream &os, const Colour &colour) {
    return os << (int) colour.r << "," << (int) colour.g << "," << (int) colour.b;
}

struct Point {
    int x;
    int y;

    Point() {
        x = -1;
        y = -1;
    }

    Point(int _x, int _y) {
        this->x = _x;
        this->y = _y;
    }

    bool operator==(const Point &other) {
        return other.x == this->x && other.y == this->y;
    }
};

struct Pixel {
    Colour colour = Colour(0,0,0);
    bool is_filled = false;
};

std::ostream &operator<<(std::ostream &os, const Point &point) {
    return os << point.x << "," << point.y;
}

std::vector<Colour> colours;
std::vector<Pixel> pixels;
std::list<Point> available_edges;
std::vector<Point> neighbours;
long colour_index = 0;

int ColourDiff(Colour *colour_1, Colour *colour_2) {
    int r = (int) colour_1->r - (int) colour_2->r;
    int g = (int) colour_1->g - (int) colour_2->g;
    int b = (int) colour_1->b - (int) colour_2->b;
    return r * r + g * g + b * b;
}

int HueDiff(Colour *colour_1, Colour *colour_2) {
    return abs(colour_1->hue - colour_2->hue);
}

int LuminosityDiff(Colour *colour_1, Colour *colour_2) {
    return abs(colour_1->lum - colour_2->lum);
}

Pixel *GetPixel(int x, int y) {
    return &pixels[y * PIXELS_WIDE + x];
}

Pixel *GetPixelAtPoint(Point &point) {
    return &pixels[point.y * PIXELS_WIDE + point.x];
}

int FillPointNeighbours(Point point) {
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

int FillColours(int colour_depth) {
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

enum StartType {
    START_TYPE_CENTRE,
    START_TYPE_CORNER,
    START_TYPE_RANDOM,
    START_TYPE_EDGE
};

void PlaceAtPoint(Point &point) {
    available_edges.push_back(point);
    Pixel *centre_pixel = GetPixelAtPoint(point);
    centre_pixel->colour = colours[colour_index];
    centre_pixel->is_filled = true;
}

int main(int argc, char *argv[]) {
    int c;
    auto difference_func = ColourDiff;
    opterr = 0;
    StartType start_type = START_TYPE_CENTRE;
    int num_start_points = 1;

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
                    difference_func = LuminosityDiff;
                } else if (diff_type == "colour" || diff_type == "color") {
                    difference_func = ColourDiff;
                } else if (diff_type == "hue") {
                    difference_func = HueDiff;
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
    long COLOUR_COUNT = FillColours(COLOUR_DEPTH);
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
        PlaceAtPoint(possible_start_points[i]);
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
            int diff = difference_func(current_colour, &GetPixelAtPoint(available_edge)->colour);
            if (diff < best_difference) {
                best_point = available_edge;
                best_difference = diff;
            }
        }

        int neighbour_count = FillPointNeighbours(best_point);
        std::shuffle(std::begin(neighbours), std::end(neighbours), rng);
        int neighbour_index = 0;
        for (; neighbour_index < neighbour_count; ++neighbour_index) {
            Point &neighbour = neighbours[neighbour_index];
            Pixel *neighbour_pixel = GetPixelAtPoint(neighbour);
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
            Pixel *pixel = GetPixel(x, y);
            bmp.set_pixel(x, y, pixel->colour.b, pixel->colour.g, pixel->colour.r);
        }
    }
    bmp.write("output.bmp");

    return 0;
}