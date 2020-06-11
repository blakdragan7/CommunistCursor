#ifndef CC_DISPLAY_H
#define CC_DISPLAY_H

#include "BasicTypes.h"
#include "../OSInterface/OSTypes.h"

struct NativeDisplay;
class CCDisplay
{
private:
    NativeDisplay display; // native OS deisplay resolution and offset
    Rect bounds; // bounds used to collision checking

public:
    CCDisplay(NativeDisplay display);
    CCDisplay(NativeDisplay display, const Rect& bounds);

    // Sets the collision bounds of this display
    void SetBounds(const Rect& bounds);
    // Sets collision bounds with {topLeft} point calculating bottomRight using 
    // the nativeDislay width and height
    void SetBounds(const Point& topLeft);
    // returns if p is within bounds using box collision
    bool PointIsInBounds(const Point& p)const;

    // Getter for the native display, mainly used for UI
    const NativeDisplay& GetNativeDisplay()const { return display; }
    // Getter for collision bounds
    const Rect GetCollision()const { return bounds; }
};

#endif