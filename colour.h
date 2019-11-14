#ifndef RAINBOW_C_COLOUR_H
#define RAINBOW_C_COLOUR_H

#include <cstdint>
#include <algorithm>
#include <iostream>

/// Gets the hue of a given RGB value
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

/// Gets the luminosity of given RGB value
/// \param r
/// \param g
/// \param b
/// \return
int getLum(int r, int g, int b) {
    return (std::max(r, std::max(g, b)) + std::min(r, std::min(g, b))) / 2;
}

/// An RGB colour value
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

int getColourAbsoluteDiff(Colour *colour_1, Colour *colour_2) {
    int r = (int) colour_1->r - (int) colour_2->r;
    int g = (int) colour_1->g - (int) colour_2->g;
    int b = (int) colour_1->b - (int) colour_2->b;
    return r * r + g * g + b * b;
}

int getColourHueDiff(Colour *colour_1, Colour *colour_2) {
    return abs(colour_1->hue - colour_2->hue);
}

int getColourLuminosityDiff(Colour *colour_1, Colour *colour_2) {
    return abs(colour_1->lum - colour_2->lum);
}

#endif //RAINBOW_C_COLOUR_H
