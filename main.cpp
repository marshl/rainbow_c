#include <iostream>
#include <cstdlib>
#include <list>
#include <iostream>
#include <algorithm>
#include <random>
#include <ctime>
#include <unistd.h>

#include "bmp.h"

auto rng = std::default_random_engine(std::random_device{}());


int PIXELS_WIDE = 256;
int PIXELS_HIGH = 256;
int COLOUR_DEPTH = 128;
const long COLOUR_COUNT = COLOUR_DEPTH * COLOUR_DEPTH * COLOUR_DEPTH;


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

struct Colour {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    int hue;

    Colour() { this->r = this->g = this->b = this->hue = 0; }

    Colour(uint8_t _r, uint8_t _g, uint8_t _b) {
        this->r = _r;
        this->g = _g;
        this->b = _b;
        this->hue = getHue(_r, _g, _b);
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
    Colour colour;
    bool is_filled;
};

std::ostream &operator<<(std::ostream &os, const Point &point) {
    return os << point.x << "," << point.y;
}

int ColourDiff(Colour *colour_1, Colour *colour_2) {
//    int d = colour_1->hue - colour_2->hue;
//    std::cout << d << std::endl;
//    return  abs(d);
//    return abs((colour_1->r + colour_1->g + colour_1->b) - (colour_2->r + colour_2->g + colour_2->b));

    int r = (int) colour_1->r - (int) colour_2->r;
    int g = (int) colour_1->g - (int) colour_2->g;
    int b = (int) colour_1->b - (int) colour_2->b;
    return r * r + g * g + b * b;
}

Pixel *GetPixel(std::vector<Pixel> &pixels, int x, int y) {
    return &pixels[y * PIXELS_WIDE + x];
}

Pixel *GetPixelAtPoint(std::vector<Pixel> &pixels, Point &point) {
    return &pixels[point.y * PIXELS_WIDE + point.x];
}

int FillPointNeighbours(std::vector<Point> &neighbours, Point point) {
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
    return neighbours.size();// neighbour_count;
}

void FillColours(std::vector<Colour> &colours) {
    long colour_index = 0;
    for (int r = 0; r < COLOUR_DEPTH; ++r) {
        for (int g = 0; g < COLOUR_DEPTH; ++g) {
            for (int b = 0; b < COLOUR_DEPTH; ++b) {
                Colour *colour = &colours[colour_index];
                colour->r = r * 255 / (COLOUR_DEPTH - 1);
                colour->g = g * 255 / (COLOUR_DEPTH - 1);
                colour->b = b * 255 / (COLOUR_DEPTH - 1);
                colour->hue = getHue(colour->r, colour->g, colour->b);
                ++colour_index;
            }
        }
    }
    std::shuffle(std::begin(colours), std::end(colours), rng);
}

int main(int argc, char *argv[]) {

    time_t start_time = time(nullptr);
    std::vector<Colour> colours(COLOUR_COUNT);
    std::vector<Pixel> pixels(PIXELS_WIDE * PIXELS_HIGH);
    std::list<Point> available_edges;
    std::vector<Point> neighbours(8);
    long colour_index = 0;

    FillColours(colours);

    Point centre_point = Point(PIXELS_WIDE / 2, PIXELS_HIGH / 2);
    available_edges.push_back(centre_point);
    Pixel *centre_pixel = GetPixelAtPoint(pixels, centre_point);
    centre_pixel->colour = colours[colour_index];
    centre_pixel->is_filled = true;

    /*
    for (int i = 0; i < 100; ++i) {
        int rand_x = rng() % PIXELS_WIDE;
        int rand_y = rng() % PIXELS_HIGH;

        Point random_point = Point(rand_x, rand_y);
        available_edges.push_back(random_point);
        Colour* centre_pixel = GetPixelAtPoint(pixels, random_point);
        centre_pixel->r = colours[colour_index].r;
        centre_pixel->g = colours[colour_index].g;
        centre_pixel->b = colours[colour_index].b;
        centre_pixel->hue = colours[colour_index].hue;
        centre_pixel->filled = true;
        colour_index += 1;
    }
    int i = 0;*/
    /*for (long y = 0; y < PIXELS_HIGH; ++y) {
        for (long x = 0; x < PIXELS_WIDE; ++x) {
            if (x == 0 || y == 0 || x == PIXELS_WIDE - 1 || y == PIXELS_HIGH - 1)
            //if ((x == 0 || x == PIXELS_WIDE - 1  ) && (y == 0 || y == PIXELS_HIGH - 1))
            //if (((x == 0 || x == PIXELS_WIDE - 1) && y == PIXELS_HIGH / 2) || ((y == 0 || y == PIXELS_HIGH - 1) && x == PIXELS_WIDE / 2))
            {
                //++i;
                //if (i % 64 == 0)
                {
                    Colour* colour = GetPixel(pixels, x, y);
                    colour->r = colours[colour_index].r;
                    colour->g = colours[colour_index].g;
                    colour->b = colours[colour_index].b;
                    ++colour_index;
                    colour->filled = true;
                    available_edges.push_back(Point(x, y));
                }
            }

        }
    }*/

    while (true) {
        if (available_edges.empty() || colour_index == COLOUR_COUNT) {
            std::cout << "Out of edges or colours" << std::endl;
            break;
        }
        Colour *current_colour = &colours[colour_index];
        Point best_point;
        int best_difference = INT32_MAX;
        for (auto &available_edge : available_edges) {
            int diff = ColourDiff(current_colour, &GetPixelAtPoint(pixels, available_edge)->colour);
            if (diff < best_difference) {
                best_point = available_edge;
                best_difference = diff;
            }
        }

        int neighbour_count = FillPointNeighbours(neighbours, best_point);
        std::shuffle(std::begin(neighbours), std::end(neighbours), rng);
        bool placed_point = false;
        for (int i = 0; i < neighbour_count; ++i) {
            Point &neighbour = neighbours[i];
            Pixel *neighbour_pixel = GetPixelAtPoint(pixels, neighbour);
            if (neighbour_pixel->is_filled) {
                continue;
            }
            neighbour_pixel->colour = *current_colour;
            neighbour_pixel->is_filled = true;
            available_edges.push_back(neighbour);
            colour_index += 1;
            placed_point = true;

            if (colour_index % 1000 == 0) {
                std::cout << colour_index << ": " << available_edges.size() << " ("
                          << ((float) colour_index / float(PIXELS_HIGH * PIXELS_WIDE) * 100) << "%)" << std::endl;
            }
            break;
        }

        if (!placed_point) {
            available_edges.remove(best_point);
        }
    }
    time_t end_time = time(nullptr);
    std::cout << "Completed in " << (end_time - start_time) << "s" << std::endl;

    BMP bmp = BMP(PIXELS_WIDE, PIXELS_HIGH, false);
    for (int y = 0; y < PIXELS_HIGH; ++y) {
        for (int x = 0; x < PIXELS_WIDE; ++x) {
            Pixel *pixel = GetPixel(pixels, x, y);
            bmp.set_pixel(x, y, pixel->colour.b, pixel->colour.g, pixel->colour.r);
        }
    }
    bmp.write("output.bmp");

    return 0;
}