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
    unsigned int assignedID; // used to ID this display, only used server side

    static unsigned int nextID;

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
    // sets the bounds using nativeDisplay.x + {offsetX} and same for y
    void SetOffsets(int offsetX, int offsetY);

    // Getter for the native display, mainly used for UI
    const NativeDisplay& GetNativeDisplay()const { return display; }
    // Getter for collision bounds
    const Rect& GetCollision()const { return bounds; }
    // Getter fir Assigned ID. Can be used to id displays server side
    const unsigned int GetAssignedID()const { return assignedID; }

};

#endif