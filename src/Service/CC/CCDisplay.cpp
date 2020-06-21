#include "CCDisplay.h"

unsigned int CCDisplay::nextID = 0;

CCDisplay::CCDisplay(NativeDisplay display) : display(display), bounds(display.posX, display.posY, \
                                        display.posX + display.width, display.posY + display.height), assignedID(nextID++)
{
}

CCDisplay::CCDisplay(NativeDisplay display, const Rect& bounds) : display(display), bounds(bounds), assignedID(nextID++)
{

}

void CCDisplay::SetBounds(const Rect& bounds)
{
    this->bounds = bounds;
}

void CCDisplay::SetBounds(const Point& topLeft)
{
    bounds.topLeft = topLeft;
    bounds.bottomRight = { topLeft.x + display.width, topLeft.y + display.height };
}

bool CCDisplay::PointIsInBounds(const Point& p)const
{
    return bounds.topLeft.x <= p.x && bounds.topLeft.y <= p.y \
    && bounds.bottomRight.x >= p.x && bounds.bottomRight.y >= p.y;
}

void CCDisplay::SetOffsets(int offsetX, int offsetY)
{
    bounds.topLeft = {display.posX + offsetX, display.posY + offsetY};
    bounds.bottomRight = { bounds.topLeft.x + display.width, bounds.topLeft.y + display.height };
}
