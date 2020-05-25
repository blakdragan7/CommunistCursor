#include "CCDisplay.h"
#include "../OSInterface/OSTypes.h"

CCDisplay::CCDisplay(NativeDisplay* display) : display(display)
{

}

CCDisplay::CCDisplay(NativeDisplay* display, const Rect& bounds) : display(display), bounds(bounds)
{

}

void CCDisplay::SetBounds(const Rect& bounds)
{
    this->bounds = bounds;
}

bool CCDisplay::PointIsInBounds(const Point& p)const
{
    return bounds.topLeft.x <= p.x && bounds.topLeft.y <= p.y \
    && bounds.bottomRight.x >= p.x && bounds.bottomRight.y >= p.y;
}