#ifndef CC_DISPLAY_H
#define CC_DISPLAY_H

#include "BasicTypes.h"

struct NativeDisplay;
class CCDisplay
{
private:
    NativeDisplay* display;
    Rect bounds;

public:
    CCDisplay(NativeDisplay* display);
    CCDisplay(NativeDisplay* display, const Rect& bounds);

    void SetBounds(const Rect& bounds);
    bool PointIsInBounds(const Point& p)const;
};

#endif