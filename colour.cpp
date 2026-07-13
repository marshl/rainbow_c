#include "colour.h"

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


//float hueToRgb(float p, float q, float t) {
//    if (t < 0.0f) t += 1.0f;
//    if (t > 1.0f) t -= 1.0f;
//    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
//    if (t < 1.0f / 2.0f) return q;
//    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
//    return p;
//}


/// An RGB colour value

Colour::Colour() {
    this->r = this->g = this->b = 0;
    this->hue = this->sat = this->lum = 0;
}

Colour::Colour(uint8_t _r, uint8_t _g, uint8_t _b) {
    this->r = _r;
    this->g = _g;
    this->b = _b;
    const auto t = rgbToHsl(_r, _g, _b);
    this->hue = std::get<0>(t);
    this->sat = std::get<1>(t);
    this->lum = std::get<2>(t);
}

bool Colour::operator<(const Colour &c) const {
    return this->r < c.r || this->g < c.g || this->b < c.b;
}


std::ostream &operator<<(std::ostream &os, const Colour &colour) {
    return os << (int) colour.r << "," << (int) colour.g << "," << (int) colour.b;
}

float getColourAbsoluteDiff(const Colour &colour_1, const Colour &colour_2) {
    int r = colour_1.r - colour_2.r;
    int g = colour_1.g - colour_2.g;
    int b = colour_1.b - colour_2.b;
    // Take sqrt to convert squared distance to actual Euclidean distance;
    // divide by sqrt(3) so max is 255 (when the diff is (255, 255, 255)).
    return std::sqrt(float(r * r + g * g + b * b)) / std::sqrt(3.0f);
}

float getColourHueDiff(const Colour &colour_1, const Colour &colour_2) {
    // Underlying value is in [0, 2] (hue and lum both in [0, 1]);
    // scale by 127.5 to hit [0, 255].
    return std::fabs((colour_1.hue - colour_1.lum) - (colour_2.hue - colour_2.lum)) * 127.5f;
}

float getColourLuminosityDiff(const Colour &colour_1, const Colour &colour_2) {
    // lum is in [0, 1], diff is in [0, 1], scale to [0, 255].
    return std::fabs(colour_1.lum - colour_2.lum) * 255.0f;
}

float getNaturalColourDiff(const Colour &colour_1, const Colour &colour_2) {
    int rmean = (colour_1.r + colour_2.r) / 2;
    int r = colour_1.r - colour_2.r;
    int g = colour_1.g - colour_2.g;
    int b = colour_1.b - colour_2.b;
    float squared = float((((512 + rmean) * r * r) >> 8)
                          + 4 * g * g
                          + (((767 - rmean) * b * b) >> 8));
    // sqrt to linear-perceptual, then scale. Empirical max is ~sqrt(650000) ≈ 806.
    return std::sqrt(squared) / std::sqrt(650000.0f) * 255.0f;
}

bool compareHue(const Colour &c1, const Colour &c2) {
    return c1.hue < c2.hue;
}

bool compareLum(const Colour &c1, const Colour &c2) {
    return c1.lum < c2.lum;
}

bool compareSat(const Colour &c1, const Colour &c2) {
    return c1.sat < c2.sat;
}
