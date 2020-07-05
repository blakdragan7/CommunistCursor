#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

#include <ostream>

struct Point
{
    int x,y;
    Point() :x(0), y(0) {}
    Point(int x, int y) : x(x), y(y) {}

    Point operator-(const Point& rh)
    {
        return Point(x - rh.x, y - rh.y);
    }

    Point operator+(const Point& rh)
    {
        return Point(x + rh.x, y + rh.y);
    }

    Point operator/(const int& rh)
    {
        return Point(x / rh, y / rh);
    }
};

struct Rect
{
    Point topLeft;
    Point bottomRight;

    Rect() {}
    Rect(int tlx, int tly, int brx, int bry) : topLeft(tlx,tly), bottomRight(brx,bry) {}
    Rect(Point tl, Point br) : topLeft(tl), bottomRight(br) {}

    bool IntersectsRect(const Rect& other)
    {
        return !(other.topLeft.x > bottomRight.x || \
                 other.bottomRight.x < topLeft.x || \
                 other.topLeft.y > bottomRight.y || \
                 other.bottomRight.y < topLeft.y);
    }
};

#endif