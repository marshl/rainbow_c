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

    bool operator==(const Point &other) const {
        return other.x == this->x && other.y == this->y;
    }

    bool operator<(const Point &other) const {
        return other.x < this->x || other.y < this->y;
    }

    friend std::ostream &operator<<(std::ostream &os, const Point &p) {
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }
};


#endif //RAINBOW_C_POINT_H
