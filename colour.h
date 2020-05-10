#ifndef RAINBOW_C_COLOUR_H
#define RAINBOW_C_COLOUR_H

#include <cmath>
#include <cstdint>
#include <tuple>
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


std::tuple<float, float, float> rgbToHsl(int pR, int pG, int pB) {
    float r = (float) pR / 255;
    float g = (float) pG / 255;
    float b = (float) pB / 255;

    float max = std::max(r, std::max(g, b));
    float min = std::min(r, std::min(g, b));

    float hue, sat, lum;
    lum = (max + min) / 2.0f;

    if (max == min) {
        hue = sat = 0.0f;
    } else {
        float diff = max - min;
        sat = (lum > 0.5f) ? diff / (2.0f - max - min) : diff / (max + min);

        if (r > g && r > b) {
            hue = (g - b) / diff + (g < b ? 6.0f : 0.0f);
        } else if (g > b) {
            hue = (b - r) / diff + 2.0f;
        } else {
            hue = (r - g) / diff + 4.0f;
        }

        hue /= 6.0f;
    }
    return {hue, sat, lum};
}


/// An RGB colour value
struct Colour {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    float hue;
    float sat;
    float lum;

    Colour() { this->r = this->g = this->b = this->hue = this->sat = this->lum = 0; }

    Colour(uint8_t _r, uint8_t _g, uint8_t _b) {
        this->r = _r;
        this->g = _g;
        this->b = _b;
//        this->hue = getHue(_r, _g, _b);
//        this->lum = getLum(_r, _g, _b);
        auto t = rgbToHsl(_r, _g, _b);
        this->hue = std::get<0>(t);
        this->sat = std::get<1>(t);
        this->lum = std::get<2>(t);
    }


};

std::ostream &operator<<(std::ostream &os, const Colour &colour) {
    return os << (int) colour.r << "," << (int) colour.g << "," << (int) colour.b;
}

float getColourAbsoluteDiff(const Colour *const colour_1, const Colour *const colour_2) {
    int r = (int) colour_1->r - (int) colour_2->r;
    int g = (int) colour_1->g - (int) colour_2->g;
    int b = (int) colour_1->b - (int) colour_2->b;
    return float(r * r + g * g + b * b);
}

float getColourHueDiff(const Colour *const colour_1, const Colour *const colour_2) {
    return std::fabs((colour_1->hue - colour_1->lum) - (colour_2->hue - colour_2->lum));
//    return std::fabs((colour_1->hue) - (colour_2->hue));
}

float getColourLuminosityDiff(const Colour *const colour_1, const Colour *const colour_2) {
    return std::fabs(colour_1->lum - colour_2->lum);
}

bool compareHue(const Colour &c1, const Colour &c2) {
    return c1.hue < c2.hue;
//    return c1.hue - c1.sat < c2.hue - c2.sat;
//    return c1.hue < c2.hue && c1.lum > c2.lum;
}

bool compareLum(const Colour &c1, const Colour &c2) {
    return c1.lum < c2.lum;
}

bool compareSat(const Colour &c1, const Colour &c2) {
    return c1.sat < c2.sat;
}

#endif //RAINBOW_C_COLOUR_H
