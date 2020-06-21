from PySide2.QtWidgets import QFrame
from PySide2.QtGui import QColor
from PySide2.QtCore import Qt, QPoint

'''
    DisplayWidget represents a display on a 
    controllable network computer

    it's position will be used as a form of bounding box for mouse
    events in global space (the virtual coord system handle by CC Service)

'''

class DisplayWidget(QFrame):
    def __init__(self, parent, displayInfo):
        super().__init__(parent)

        dimensions = displayInfo["bounds"]

        p = self.palette()
        p.setColor(self.backgroundRole(), QColor(30,79,112))
        self.setPalette(p)

        self.setAutoFillBackground(True)
        self.setFrameShape(QFrame.Box)

        self.ID = displayInfo["id"]

        self.displayPosX    = dimensions[0]
        self.displayPosY    = dimensions[1]
        self.displayWidth   = dimensions[2]
        self.displayHeight  = dimensions[3]

        self.widgetBounds   = [0, 0, self.displayWidth, self.displayHeight]
        self.groupBounds  = self.widgetBounds.copy()
    
    def SetWidgetBounds(self, bounds):
        self.widgetBounds = bounds.copy()

    def SetGroupBounds(self, bounds):
        self.groupBounds = bounds.copy()

    def UpdateGeometry(self):
        
        scaleX = (self.widgetBounds[2] - self.widgetBounds[0]) / (self.groupBounds[2] - self.groupBounds[0])
        scaleY = (self.widgetBounds[3] - self.widgetBounds[1]) / (self.groupBounds[3] - self.groupBounds[1])

        posX = ((self.displayPosX - self.groupBounds[0]) * scaleX) + self.widgetBounds[0]
        posY = ((self.displayPosY - self.groupBounds[1]) * scaleY) + self.widgetBounds[1]

        width  = self.displayWidth  * scaleX
        height = self.displayHeight * scaleY

        self.setGeometry(posX, posY, width, height)
