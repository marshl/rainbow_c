#ifndef RAINBOW_C_POINT_H
#define RAINBOW_C_POINT_H

#include <iostream>

/// An X and Y point
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

std::ostream &operator<<(std::ostream &os, const Point &point) {
    return os << point.x << "," << point.y;
}

#endif //RAINBOW_C_POINT_H
