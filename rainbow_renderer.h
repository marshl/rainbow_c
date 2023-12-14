#ifndef RAINBOW_C_RAINBOW_RENDERER_H
#define RAINBOW_C_RAINBOW_RENDERER_H

#include <vector>
#include <list>
#include <sstream>

#include "bmp.h"
#include "colour.h"
#include "pixel.h"
#include "point.h"

enum OrderingType {
    COLOUR_ORDER_HUE,
    COLOUR_ORDER_SAT,
    COLOUR_ORDER_LUM,
    COLOUR_ORDER_RANDOM,
};

struct ColourOrdering {

    OrderingType ordering_type;
    bool reverse = false;
};

class RainbowRenderer {
public:

    enum StartType {
        START_TYPE_CENTRE,
        START_TYPE_CORNER,
        START_TYPE_RANDOM,
        START_TYPE_EDGE,
        START_TYPE_CIRCLE,
    };

    enum FillMode {
        FILL_MODE_NEIGHBOUR,
        FILL_MODE_EDGE,
        FILL_MODE_NEIGHBOUR_AVERAGE,
    };

    void setSeed(unsigned int _seed) {
        this->seed = _seed;
    }

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

    void setDifferenceFunction(float (*_func)(const Colour &, const Colour &)) {
        this->difference_function = _func;
    }

    void setStartType(StartType _start_type) {
        this->start_type = _start_type;
    }

    void setFillMode(FillMode _fill_mode) {
        this->fill_mode = _fill_mode;
    }

    void addColourOrder(ColourOrdering ordering) {
        this->colour_ordering.push_back(ordering);
    }

    /// Initialises starting pixels
    void init() {
        this->rng = std::default_random_engine(this->seed);
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
                        if (this->start_type == START_TYPE_RANDOM ||
                            (x == 0 || x == this->pixels_wide - 1 || y == 0 || y == this->pixels_high - 1)) {
                            possible_start_points.emplace_back(Point(x, y));
                        }
                    }
                }
                break;
            case START_TYPE_CIRCLE: {
                // Make a circle such that the inner area and outer area are the same (for a square board)
                int radius = int(float(std::min(this->pixels_wide, this->pixels_high)) / sqrt(2 * M_PI));
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

    void fill() {
        switch (this->fill_mode) {
            case FILL_MODE_EDGE:
                this->edge_fill();
                break;
            case FILL_MODE_NEIGHBOUR:
                this->neighbour_fill();
                break;
            case FILL_MODE_NEIGHBOUR_AVERAGE:
                this->neighbour_fill(true);
                break;
        }
    }

    /// Fills remaining spaces with pixels
    void edge_fill() {
        int partition = this->pixels_high * this->pixels_wide / 100;
        while (true) {
            if (this->available_edges.empty() || this->colour_index == this->colours.size()) {
                std::cout << "Out of edges or colours" << std::endl;
                break;
            }
            const Colour &current_colour = this->colours[this->colour_index];
            Point best_point;
            float best_difference = MAXFLOAT;
            for (auto &available_edge: this->available_edges) {
                float diff = this->difference_function(current_colour, getPixelAtPoint(available_edge)->colour);
                if (diff < best_difference) {
                    best_point = available_edge;
                    best_difference = diff;
                }
            }

            auto neighbours = getNeighboursOfPoint(best_point);
            int neighbour_count = neighbours.size();
            std::shuffle(std::begin(neighbours), std::end(neighbours), this->rng);
            int neighbour_index = 0;
            for (; neighbour_index < neighbour_count; ++neighbour_index) {
                Point &neighbour = neighbours[neighbour_index];
                Pixel *const neighbour_pixel = getPixelAtPoint(neighbour);
                if (neighbour_pixel->is_filled) {
                    continue;
                }
                neighbour_pixel->colour = current_colour;
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
                    this->cleanFilledEdges();
                }
                break;
            }

            if (neighbour_index == neighbour_count) {
                this->available_edges.remove(best_point);
            }
        }
    }

    void neighbour_fill(bool neighbour_average = false) {
        const int total_pixels = this->pixels_high * this->pixels_wide;
        const int partition = total_pixels / 100;
        // List of places where pixels can be placed (next to neighbours)
        std::list<Point> availablePoints;
        // Find all neighbours of existing points and add them to the available list
        for (int y = 0; y < this->pixels_high; ++y) {
            for (int x = 0; x < this->pixels_wide; ++x) {
                Point point = Point(x, y);
                Pixel *pixel = this->getPixelAtPoint(point);
                if (!pixel->is_filled) {
                    continue;
                }
                for (auto neighbour: this->getNeighboursOfPoint(point)) {
                    Pixel *neighbour_pixel = this->getPixelAtPoint(neighbour);
                    if (!neighbour_pixel->is_filled && !neighbour_pixel->is_available) {
                        this->getPixelAtPoint(neighbour)->is_available = true;
                        availablePoints.push_back(neighbour);
                    }
                }
            }
        }

        // While there are colours to place and avaialable spots to place them
        for (; this->colour_index < this->colours.size() && !availablePoints.empty(); ++this->colour_index) {
            Colour &colour = this->colours[colour_index];
            Point best_point;
            float best_difference = MAXFLOAT;
            // Find the point that has neighbours that are closest
            for (auto point: availablePoints) {
                float neighbourDiff = this->getNeighbourDifference(point, colour,
                                                                   neighbour_average = neighbour_average);
                if (neighbourDiff < best_difference) {
                    best_difference = neighbourDiff;
                    best_point = point;
                }
            }
            Pixel *pixel = this->getPixelAtPoint(best_point);
            pixel->is_filled = true;
            pixel->is_available = false;
            pixel->colour = colour;
            availablePoints.remove(best_point);

            for (auto neighbour: this->getNeighboursOfPoint(best_point)) {
                Pixel *neighbourPixel = this->getPixelAtPoint(neighbour);
                if (!neighbourPixel->is_filled && !neighbourPixel->is_available) {
                    neighbourPixel->is_available = true;
                    availablePoints.push_back(neighbour);
                }
            }

            if (this->colour_index % partition == 0) {
                std::cout << "Placed " << this->colour_index << "/" << total_pixels << " pixels ( "
                          << ((float) this->colour_index / float(total_pixels) * 100) << "%) with "
                          << availablePoints.size()
                          << " spaces available"
                          << std::endl;
                std::ostringstream stream;
                stream << "output_" << this->colour_index << ".bmp";
                std::cout << "Saving..." << std::flush;
                this->writeToFile(stream.str());
                std::cout << "Done" << std::endl;
            }

        }
    }

    float getNeighbourDifference(Point point, const Colour &colour, bool neighbour_average = false) {
        std::vector<float> diffs;
        for (auto neighbour: this->getNeighboursOfPoint(point)) {
            const Pixel *pixel = this->getPixelAtPoint(neighbour);
            if (!pixel->is_filled) {
                continue;
            }
            float diff = this->difference_function(colour, pixel->colour);
            diffs.push_back(diff);
        }
        if (diffs.empty()) {
            return MAXFLOAT;
        }

        if (neighbour_average) {
            return (float) (std::accumulate(diffs.begin(), diffs.end(), 0.0) / diffs.size());
        } else {
            return *std::min_element(diffs.begin(), diffs.end());
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

    void cleanFilledEdges() {
        std::cout << "Cleaning " << std::flush;
        auto iter = this->available_edges.begin();
        while (iter != this->available_edges.end()) {
            Point edgePoint = *iter;
            auto neighbours = getNeighboursOfPoint(edgePoint);
            bool hasOpenNeighbours = false;
            for (auto &neighbour_point: neighbours) {
                const Pixel *neighbour_pixel = getPixelAtPoint(neighbour_point);
                if (!neighbour_pixel->is_filled) {
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


private:
    std::default_random_engine rng;
    unsigned int seed = std::random_device{}();

    int pixels_wide = 256;
    int pixels_high = 256;
    int colour_depth = 0;
    int num_start_points = INT32_MAX;
    StartType start_type = StartType::START_TYPE_CENTRE;
    FillMode fill_mode = FillMode::FILL_MODE_EDGE;
    std::vector<ColourOrdering> colour_ordering;

    float (*difference_function)(const Colour &, const Colour &) = getColourAbsoluteDiff;

    std::vector<Colour> colours;
    std::vector<Pixel> pixels;
    std::list<Point> available_edges;
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
    std::vector<Point> getNeighboursOfPoint(const Point &point) const {
        std::vector<Point> neighbours;
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
        return neighbours;
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
        std::cout << "Colour depth " << this->colour_depth << " makes " << this->colours.size() << " colours (of "
                  << (this->pixels_wide * this->pixels_high) << " pixels)" << std::endl;

        // Default colour ordering if the user doesn't supply any
        if (this->colour_ordering.empty()) {
            this->colour_ordering.push_back({COLOUR_ORDER_LUM, false});
            this->colour_ordering.push_back({COLOUR_ORDER_HUE, false});
            this->colour_ordering.push_back({COLOUR_ORDER_SAT, true});
        }

        for (auto order: this->colour_ordering) {
            bool (*compare_func)(const Colour &c1, const Colour &c2);
            switch (order.ordering_type) {
                case COLOUR_ORDER_RANDOM:
                    std::shuffle(std::begin(colours), std::end(colours), rng);
                    continue;
                case COLOUR_ORDER_HUE:
                    compare_func = compareHue;
                    break;
                case COLOUR_ORDER_SAT:
                    compare_func = compareSat;
                    break;
                case COLOUR_ORDER_LUM:
                    compare_func = compareLum;
                    break;
                default:
                    std::cerr << "Unknown colour ordering " << order.ordering_type << std::endl;
                    return;
            }
            if (order.reverse) {
                std::sort(std::rbegin(colours), std::rend(colours), compare_func);
            } else {
                std::sort(std::begin(colours), std::end(colours), compare_func);
            }

        }
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
