#ifndef RAINBOW_C_RAINBOW_RENDERER_H
#define RAINBOW_C_RAINBOW_RENDERER_H

#include <vector>
#include <list>
#include <sstream>

#include "bmp.h"
#include "colour.h"
#include "pixel.h"
#include "point.h"

class RainbowRenderer {
public:
    std::default_random_engine rng = std::default_random_engine(std::random_device{}());

    enum StartType {
        START_TYPE_CENTRE,
        START_TYPE_CORNER,
        START_TYPE_RANDOM,
        START_TYPE_EDGE,
        START_TYPE_CIRCLE,
    };

    void setPixelsWide(int _pixels_wide) {
        this->pixels_wide = _pixels_wide;
    }

    void setPixelsHigh(int _pixels_high) {
        this->pixels_high = _pixels_high;
    }

    void setNumStartPoints(int _num_start_points) {
        this->num_start_points = _num_start_points;
    }

    void setColourDepth(int _colour_depth) {
        this->colour_depth = _colour_depth;
    }

    void setDifferenceFunction(int (*_func)(Colour *, Colour *)) {
        this->difference_function = _func;
    }

    void setStartType(StartType _start_type) {
        this->start_type = _start_type;
    }

    /// Initialises starting pixels
    void init() {
        this->pixels.resize(this->pixels_wide * this->pixels_high);
        this->fillColours();
        std::vector<Point> possible_start_points;

        switch (this->start_type) {
            case START_TYPE_CENTRE: {
                possible_start_points.emplace_back(Point(this->pixels_wide / 2, this->pixels_high / 2));
                break;
            }
            case START_TYPE_CORNER: {
                possible_start_points.emplace_back(Point(0, 0));
                possible_start_points.emplace_back(Point(this->pixels_wide - 1, 0));
                possible_start_points.emplace_back(Point(0, this->pixels_high - 1));
                possible_start_points.emplace_back(Point(this->pixels_wide - 1, this->pixels_high - 1));
                break;
            }
            case START_TYPE_EDGE:
            case START_TYPE_RANDOM:
                for (int y = 0; y < this->pixels_high; ++y) {
                    for (int x = 0; x < this->pixels_wide; ++x) {
                        if (start_type == START_TYPE_RANDOM ||
                            (x == 0 || x == this->pixels_wide - 1 || y == 0 || y == this->pixels_high - 1)) {
                            possible_start_points.emplace_back(Point(x, y));
                        }
                    }
                }
                break;
            case START_TYPE_CIRCLE: {
                int radius = std::min(this->pixels_wide, this->pixels_high) / 4;
                int f = 1 - radius;
                int ddF_x = 0;
                int ddF_y = -2 * radius;
                int x_offset = 0;
                int y_offset = radius;
                int mid_x = this->pixels_wide / 2;
                int mid_y = this->pixels_high / 2;

                possible_start_points.emplace_back(Point(mid_x, mid_y + radius));
                possible_start_points.emplace_back(Point(mid_x, mid_y - radius));
                possible_start_points.emplace_back(Point(mid_x + radius, mid_y));
                possible_start_points.emplace_back(Point(mid_x - radius, mid_y));

                while (x_offset < y_offset) {
                    if (f >= 0) {
                        --y_offset;
                        ddF_y += 2;
                        f += ddF_y;
                    }
                    x_offset++;
                    ddF_x += 2;
                    f += ddF_x + 1;
                    possible_start_points.emplace_back(Point(mid_x + x_offset, mid_y + y_offset));
                    possible_start_points.emplace_back(Point(mid_x - x_offset, mid_y + y_offset));
                    possible_start_points.emplace_back(Point(mid_x + x_offset, mid_y - y_offset));
                    possible_start_points.emplace_back(Point(mid_x - x_offset, mid_y - y_offset));
                    possible_start_points.emplace_back(Point(mid_x + y_offset, mid_y + x_offset));
                    possible_start_points.emplace_back(Point(mid_x - y_offset, mid_y + x_offset));
                    possible_start_points.emplace_back(Point(mid_x + y_offset, mid_y - x_offset));
                    possible_start_points.emplace_back(Point(mid_x - y_offset, mid_y - x_offset));
                }
                break;
            }
        }

        std::shuffle(possible_start_points.begin(), possible_start_points.end(), this->rng);
        for (int i = 0; i < possible_start_points.size() && i < this->num_start_points; ++i) {
            fillPoint(possible_start_points[i]);
        }
    }

    /// Fills remaining spaces with pixels
    void fill() {
        int partition = this->pixels_high * this->pixels_wide / 100;
        while (true) {
            if (this->available_edges.empty() || this->colour_index == this->colours.size()) {
                std::cout << "Out of edges or colours" << std::endl;
                break;
            }
            Colour *current_colour = &this->colours[this->colour_index];
            Point best_point;
            int best_difference = INT32_MAX;
            for (auto &available_edge : this->available_edges) {
                int diff = this->difference_function(current_colour, &getPixelAtPoint(available_edge)->colour);
                if (diff < best_difference) {
                    best_point = available_edge;
                    best_difference = diff;
                }
            }

            int neighbour_count = getNeighboursOfPoint(best_point);
            std::shuffle(std::begin(this->neighbours), std::end(this->neighbours), this->rng);
            int neighbour_index = 0;
            for (; neighbour_index < neighbour_count; ++neighbour_index) {
                Point &neighbour = this->neighbours[neighbour_index];
                Pixel *neighbour_pixel = getPixelAtPoint(neighbour);
                if (neighbour_pixel->is_filled) {
                    continue;
                }
                neighbour_pixel->colour = *current_colour;
                neighbour_pixel->is_filled = true;
                this->available_edges.push_back(neighbour);
                ++this->colour_index;

                if (this->colour_index % partition == 0) {
                    std::cout << "Step " << this->colour_index << " with " << this->available_edges.size() << " edges ("
                              << ((float) this->colour_index / float(this->pixels_high * this->pixels_wide) * 100)
                              << "%)"
                              << std::endl;
                    std::ostringstream stream;
                    stream << "output_" << this->colour_index << ".bmp";
                    std::cout << "Saving..." << std::flush;
                    this->writeToFile(stream.str());
                    std::cout << "Done" << std::endl;

                    std::cout << "Cleaning " << std::flush;
                    auto iter = this->available_edges.begin();
                    while (iter != this->available_edges.end()) {
                        Point edgePoint = *iter;
                        neighbour_count = getNeighboursOfPoint(edgePoint);
                        bool hasOpenNeighbours = false;
                        for (int i = 0; i < neighbour_count; ++i) {
                            Point& p = neighbours[i];
                            Pixel *n = getPixelAtPoint(p);
                            if (!n->is_filled) {
                                hasOpenNeighbours = true;
                                break;
                            }
                        }
                        if (!hasOpenNeighbours) {
                            iter = this->available_edges.erase(iter);
                        } else {
                            ++iter;
                        }
                    }
                    std::cout << "Done" << std::endl;
                }
                break;
            }

            if (neighbour_index == neighbour_count) {
                this->available_edges.remove(best_point);
            }
        }
    }

    /// Writes the current content of the pixel board out to file
    /// \param _filename
    void writeToFile(const std::string &_filename) {
        BMP bmp = BMP(this->pixels_wide, this->pixels_high, false);
        for (int y = 0; y < this->pixels_high; ++y) {
            for (int x = 0; x < this->pixels_wide; ++x) {
                Pixel *pixel = getPixel(x, y);
                bmp.set_pixel(x, y, pixel->colour.b, pixel->colour.g, pixel->colour.r);
            }
        }
        bmp.write(_filename.c_str());
    }

private:

    int pixels_wide = 256;
    int pixels_high = 256;
    int colour_depth = 0;
    int num_start_points = INT32_MAX;
    StartType start_type = StartType::START_TYPE_CENTRE;

    int (*difference_function)(Colour *, Colour *) = getColourAbsoluteDiff;

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
        return &this->pixels[y * this->pixels_wide + x];
    }

    /// Gets the pixel at the given point
    /// \param point The point
    /// \return The pixel at that point
    Pixel *getPixelAtPoint(const Point &point) {
        return &this->pixels[point.y * this->pixels_wide + point.x];
    }

    /// Gets the neighbour positions of the given point (points over the edge of the board aren ot returned)
    /// \param point The point to find the neighbours for
    /// \return The number of neighbours
    int getNeighboursOfPoint(const Point &point) {
        neighbours.clear();
        for (int y = -1; y <= 1; ++y) {
            if (y + point.y < 0 || y + point.y >= this->pixels_high) {
                continue;
            }

            for (int x = -1; x <= 1; ++x) {
                if ((x == 0 && y == 0) || x + point.x < 0 || x + point.x >= this->pixels_wide) {
                    continue;
                }

                neighbours.emplace_back(Point(x + point.x, y + point.y));
            }
        }
        return neighbours.size();
    }

    /// Fills the list of random colours
    /// \param colour_depth The number of each unique colours in each channel
    void fillColours() {
        if (this->colour_depth == 0) {
            this->colour_depth = ceil(pow(this->pixels_wide * this->pixels_high, 1.0f / 3.0f));
        }

        for (int r = 0; r < this->colour_depth; ++r) {
            for (int g = 0; g < this->colour_depth; ++g) {
                for (int b = 0; b < this->colour_depth; ++b) {
                    this->colours.emplace_back(Colour(r * 255 / (this->colour_depth - 1),
                                                      g * 255 / (this->colour_depth - 1),
                                                      b * 255 / (this->colour_depth - 1)));
                }
            }
        }
//        std::shuffle(std::begin(colours), std::end(colours), rng);

        std::cout << "depth: " << this->colour_depth << " = " << this->colours.size() << " actual "
                  << (this->pixels_wide * this->pixels_high) << std::endl;
        std::sort(std::begin(colours), std::end(colours), compareLum);
        std::sort(std::begin(colours), std::end(colours), compareHue);
    }


    /// Fills the pixel at the given point
    /// \param point The pointto place the pixel at
    void fillPoint(Point &point) {
        this->available_edges.push_back(point);
        Pixel *centre_pixel = getPixelAtPoint(point);
        centre_pixel->colour = this->colours[this->colour_index];
        centre_pixel->is_filled = true;
        ++this->colour_index;
    }

};

#endif //RAINBOW_C_RAINBOW_RENDERER_H
