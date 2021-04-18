from PySide2.QtWidgets import QWidget, QLabel, QFrame, QVBoxLayout
from PySide2.QtGui import QColor, QFont, QFontMetrics
from PySide2.QtCore import Qt, QPoint, QRect

from DisplayWidget import DisplayWidget

'''
    DisplayWidgetGroup is a GUI Representation a controllable 
    network computer and it's displays. 
'''

class DisplayWidgetGroup(QWidget):
    def __init__(self, parent, id):
        super().__init__(parent)
        self.displays = []
        self.groupID = id
        self.displayArea = parent

        self.minX = 1000000
        self.minY = 1000000

        self.maxX = 0
        self.maxY = 0

        self.labelFrame = QFrame(self)
        self.labelFrame.setFrameShape(QFrame.Box)
        self.labelFrame.setAutoFillBackground(True)

        p = self.labelFrame.palette()
        p.setColor(self.backgroundRole(), QColor(80,80,200,20))
        self.labelFrame.setPalette(p)

        self.label = QLabel(self.labelFrame)
        self.label.setText(self.groupID)
        self.label.setFont(QFont("arial", 32))
        self.label.setAlignment(Qt.AlignCenter)

        self.globalBounds = [0, 0, self.width(), self.height()]
        self.widgetBounds = [0, 0, self.width(), self.height()]

    def AddDisplay(self, displayInfo):
        display = DisplayWidget(self, displayInfo)
        self.displays.append(display)

        dimensions = displayInfo["bounds"]

        self.minX = min(self.minX, dimensions[0])
        self.minY = min(self.minY, dimensions[1])

        self.maxX = max(self.maxX, (dimensions[2] + dimensions[0]))
        self.maxY = max(self.maxY, (dimensions[3] + dimensions[1]))

        self.labelFrame.raise_()

        self.UpdateDisplaysGroupBounds()

    def MoveBounds(self, deltaX, deltaY):
        self.minX += deltaX
        self.maxX += deltaX
        self.minY += deltaY
        self.maxY += deltaY

        self.UpdateDisplaysGroupBounds()

    def MoveBoundsTo(self, newMinX, newMinY):
        sizeX = self.maxX - self.minX
        sizeY = self.maxY - self.minY

        self.minX = newMinX
        self.minY = newMinY
        self.maxX = self.minX + sizeX
        self.maxY = self.minY + sizeY

        self.UpdateDisplaysGroupBounds()

    def resizeEvent(self, event):
        self.labelFrame.setGeometry(0, 0, self.width(), self.height())
        self.label.setGeometry(0, 0, self.width(), self.height())

        f = QFont("ariel", self.FontSizeForWidth())
        self.label.setFont(f)

        for display in self.displays:
            display.SetWidgetBounds([0,0,self.width(),self.height()])
            display.UpdateGeometry()

        return super().resizeEvent(event)

    def FontSizeForWidth(self):
        text = self.label.text()
        textSize = len(text)
        f = self.label.font()
        metrics = QFontMetrics(f)
        width = metrics.width(text)
        myWidth = self.label.width()
        size = 0
        if width < myWidth:
            direction = 1
        else:
            direction = -1
        while(True):
            f = QFont("Ariel", size + 1)
            mf = QFontMetrics(f)
            self.label.setFont(f)
            width = mf.width(text)
            if direction > 0:
                if width >= myWidth:
                    break
            else:
                if width <= myWidth:
                    break
            size = f.pointSize() + direction
        return size

    def UpdateGlobalBounds(self, bounds):
        self.globalBounds = bounds.copy()

    def SetWidgetBounds(self, bounds):
        self.widgetBounds = bounds.copy()

        for display in self.displays:
            display.SetWidgetBounds([0,0,self.width(),self.height()])
            display.UpdateGeometry()

    def UpdateGeometry(self):
        
        scaleX = (self.widgetBounds[2] - self.widgetBounds[0]) / (self.globalBounds[2] - self.globalBounds[0])
        scaleY = (self.widgetBounds[3] - self.widgetBounds[1]) / (self.globalBounds[3] - self.globalBounds[1])

        posX = ((self.minX - self.globalBounds[0]) * scaleX) + self.widgetBounds[0]
        posY = ((self.minY - self.globalBounds[1]) * scaleY) + self.widgetBounds[1]

        width  = (self.maxX - self.minX) * scaleX
        height = (self.maxY - self.minY) * scaleY

        self.setGeometry(posX, posY, width, height)

    def UpdateDisplaysGroupBounds(self):
        bounds = [self.minX, self.minY, self.maxX - self.minX, self.maxY - self.minY]
        for display in self.displays:
            display.SetGroupBounds(bounds)

    def GetGlobalOffset(self):

        scaleX = (self.globalBounds[2] - self.globalBounds[0]) / (self.widgetBounds[2] - self.widgetBounds[0])
        scaleY = (self.globalBounds[3] - self.globalBounds[0]) / (self.widgetBounds[3] - self.widgetBounds[0])

        posX = ((self.pos().x() - self.widgetBounds[0]) * scaleX) + self.globalBounds[0]
        posY = ((self.pos().y() - self.widgetBounds[1]) * scaleY) + self.globalBounds[1]

        return [posX, posY]

    def CollidesWith(self, widget):
        bounds = [self.x(),  self.y(), self.x() + self.width(), self.y() + self.height()]
        other = [widget.x(),  widget.y(), widget.x() + widget.width(), widget.y() + widget.height()]

        return not (other[0] > bounds[2] or other[2] < bounds[0] or\
            other[1] > bounds[3] or other[3] < bounds[1])

    def mouseMoveEvent(self, event):
        if self.oldPos:
            deltaPos = (event.globalPos() - self.oldPos)
            self.move(self.x() + deltaPos.x(), self.y() + deltaPos.y())
            if self.displayArea.CheckCollision(self):
                self.move(self.x() - deltaPos.x(), self.y() - deltaPos.y())

        self.oldPos = event.globalPos()
        return super().mouseMoveEvent(event)

    def mousePressEvent(self, event):
        self.oldPos = event.globalPos()
        return super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        self.oldPos = None
        return super().mouseReleaseEvent(event)