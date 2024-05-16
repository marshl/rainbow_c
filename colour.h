#ifndef RAINBOW_C_COLOUR_H
#define RAINBOW_C_COLOUR_H

#include <cmath>
#include <cstdint>
#include <tuple>
#include <algorithm>
#include <iostream>


std::tuple<float, float, float> rgbToHsl(int pR, int pG, int pB);


/// An RGB colour value
struct Colour {
    int r;
    int g;
    int b;
    float hue;
    float sat;
    float lum;

    Colour();

    Colour(uint8_t _r, uint8_t _g, uint8_t _b);

    bool operator<(const Colour &c) const;

};

std::ostream &operator<<(std::ostream &os, const Colour &colour);

float getColourAbsoluteDiff(const Colour &colour_1, const Colour &colour_2);

float getColourHueDiff(const Colour &colour_1, const Colour &colour_2);

float getColourLuminosityDiff(const Colour &colour_1, const Colour &colour_2);

float getNaturalColourDiff(const Colour &colour_1, const Colour &colour_2);

bool compareHue(const Colour &c1, const Colour &c2);

bool compareLum(const Colour &c1, const Colour &c2);

bool compareSat(const Colour &c1, const Colour &c2);

#endif //RAINBOW_C_COLOUR_H
