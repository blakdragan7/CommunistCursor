#include "CCDisplay.h"

CCDisplay::CCDisplay(NativeDisplay display) : display(display), bounds()
{
}

CCDisplay::CCDisplay(NativeDisplay display, const Rect& bounds) : display(display), bounds(bounds)
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