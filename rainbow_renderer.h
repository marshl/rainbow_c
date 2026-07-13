#ifndef RAINBOW_C_RAINBOW_RENDERER_H
#define RAINBOW_C_RAINBOW_RENDERER_H

#include <vector>
#include <optional>
#include <random>

#include "colour.h"
#include "pixel.h"
#include "point.h"
#include "thread_pool.h"

enum OrderingType {
    COLOUR_ORDER_HUE,
    COLOUR_ORDER_SAT,
    COLOUR_ORDER_LUM,
    COLOUR_ORDER_RANDOM,
};

struct ColourOrdering {
    OrderingType ordering_type = OrderingType::COLOUR_ORDER_HUE;
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
        START_TYPE_HORIZONTAL_LINE,
        START_TYPE_VERTICAL_LINE,
    };

    enum FillMode {
        FILL_MODE_NEIGHBOUR,
        FILL_MODE_EDGE,
        FILL_MODE_NEIGHBOUR_AVERAGE,
    };

    void setSeed(unsigned int _seed);

    void setPixelsWide(int _pixels_wide);

    void setPixelsHigh(int _pixels_high);

    void setNumStartPoints(int _num_start_points);

    void setColourDepth(int _colour_depth);

    void setNumIntermediateFrames(int _num_frames);

    void setDifferenceFunction(float (*_func)(const Colour &, const Colour &));

    void setStartType(StartType _start_type);

    void setFillMode(FillMode _fill_mode);

    void addColourOrder(ColourOrdering ordering);

    void addStartingHue(int hue);

    void addStartingColour(const Colour &colour);

    void setMinimumLuminosity(float luminosity);

    void setMaximumLuminosity(float luminosity);

    void setMinimumSaturation(float saturation);

    void setMaximumSaturation(float saturation);

    /// Initialises starting pixels
    void init();

    void fill();

    /// Fills remaining spaces with pixels
    void edge_fill();

    void neighbour_fill(bool neighbour_average = false);

    float getNeighbourDifference(Point point, const Colour &colour, bool neighbour_average = false);

    /// Writes the current content of the pixel board out to file
    /// \param _filename
    void writeToFile(const std::string &_filename);

private:
    std::default_random_engine rng;
    unsigned int seed = std::random_device{}();

    int pixels_wide = 256;
    int pixels_high = 256;
    int colour_depth = 0;
    int num_intermediate_frames = 0;
    std::optional<int> num_start_points;
    StartType start_type = StartType::START_TYPE_CENTRE;
    FillMode fill_mode = FillMode::FILL_MODE_EDGE;
    std::vector<ColourOrdering> colour_ordering;
    std::vector<int> startingHues;
    std::vector<Colour> startingColours;

    float minimumLuminosity = 0.0f;
    float maximumLuminosity = 1.0f;

    float minimumSaturation = 0.0f;
    float maximumSaturation = 1.0f;

    float (*difference_function)(const Colour &, const Colour &) = getColourAbsoluteDiff;

    std::vector<Colour> colours;
    std::vector<Pixel> pixels;
    std::vector<Point> available_edges;
    std::size_t colour_index = 0;

    // Launched at construction with hardware_concurrency threads and reused
    // for every parallel min-reduction. Deleted-copy in ThreadPool makes
    // RainbowRenderer non-copyable transitively — that's fine, we never copy it.
    ThreadPool thread_pool_;

    /// Get the pixel at the given x and y coordinates
    /// \param x The x coordinate
    /// \param y The y coordinate
    /// \return The pixel at that position
    Pixel *getPixel(int x, int y);

    /// Gets the pixel at the given point
    /// \param point The point
    /// \return The pixel at that point
    Pixel *getPixelAtPoint(const Point &point);

    /// Gets the neighbour positions of the given point (points over the edge of the board aren ot returned)
    /// \param point The point to find the neighbours for
    /// \return The number of neighbours
    std::vector<Point> getNeighboursOfPoint(const Point &point) const;

    /// Fills the list of random colours
    /// \param colour_depth The number of each unique colours in each channel
    void fillColours();

    /// Fills the pixel at the given point
    /// \param point The pointto place the pixel at
    void fillPoint(Point &point);

    /// Push a point onto available_edges and record its index on the pixel
    /// so future removals can happen in O(1).
    void pushEdge(const Point &p);

    /// Swap-pop the edge at `idx` from available_edges, keeping the moved
    /// element's stored edge_index in sync.
    void popEdge(std::size_t idx);
};

#endif //RAINBOW_C_RAINBOW_RENDERER_H
