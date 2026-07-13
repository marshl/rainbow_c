#ifndef RAINBOW_C_PIXEL_H
#define RAINBOW_C_PIXEL_H

#include "colour.h"

/// A colour at a position
struct Pixel {
    Colour colour = Colour(0, 0, 0);
    bool is_filled = false;
    bool is_available = false;
    // Index into RainbowRenderer::available_edges when this pixel is a
    // live edge candidate for edge_fill; -1 when the pixel is not in that
    // list. Lets us remove a dead edge in O(1) without a linear search.
    int edge_index = -1;
};

#endif //RAINBOW_C_PIXEL_H
