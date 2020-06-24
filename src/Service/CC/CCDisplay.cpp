#include "CCDisplay.h"

unsigned int CCDisplay::_nextID = 0;

CCDisplay::CCDisplay(NativeDisplay display) : _display(display), _bounds(display.posX, display.posY, \
                                        display.posX + display.width, display.posY + display.height), _assignedID(_nextID++)
{
}

CCDisplay::CCDisplay(NativeDisplay display, const Rect& bounds) : _display(display), _bounds(bounds), _assignedID(_nextID++)
{

}

void CCDisplay::SetBounds(const Rect& bounds)
{
    _bounds = bounds;
}

void CCDisplay::SetBounds(const Point& topLeft)
{
    _bounds.topLeft = topLeft;
    _bounds.bottomRight = { topLeft.x + _display.width, topLeft.y + _display.height };
}

bool CCDisplay::PointIsInBounds(const Point& p)const
{
    return _bounds.topLeft.x <= p.x && _bounds.topLeft.y <= p.y \
    && _bounds.bottomRight.x >= p.x && _bounds.bottomRight.y >= p.y;
}

void CCDisplay::SetOffsets(int offsetX, int offsetY)
{
    _bounds.topLeft = {_display.posX + offsetX, _display.posY + offsetY};
    _bounds.bottomRight = { _bounds.topLeft.x + _display.width, _bounds.topLeft.y + _display.height };
}
