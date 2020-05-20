#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

struct Point
{
    int x,y;
};

struct Rect
{
    Point topLeft;
    Point bottomRight;
};

#endif