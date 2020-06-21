#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

struct Point
{
    int x,y;
    Point() :x(0), y(0) {}
    Point(int x, int y) : x(x), y(y) {}
};

struct Rect
{
    Point topLeft;
    Point bottomRight;

    Rect() {}
    Rect(int tlx, int tly, int brx, int bry) : topLeft(tlx,tly), bottomRight(brx,bry) {}
    Rect(Point tl, Point br) : topLeft(tl), bottomRight(br) {}
};

#endif