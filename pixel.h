#ifndef RAINBOW_C_PIXEL_H
#define RAINBOW_C_PIXEL_H

#include "colour.h"

/// A colour at a position
struct Pixel {
    Colour colour = Colour(0, 0, 0);
    bool is_filled = false;
    bool is_available = false;
};

#endif //RAINBOW_C_PIXEL_H
