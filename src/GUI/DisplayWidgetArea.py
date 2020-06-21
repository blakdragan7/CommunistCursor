from PySide2.QtWidgets import QWidget, QFrame
from PySide2.QtGui import QColor
from PySide2.QtCore import QPoint, QSize

from DisplayWidgetGroup import DisplayWidgetGroup

'''

    DisplayWidgetArea

    A specified area to be able to add network computers (DisplayWidgetGroup's)

'''
class DisplayWidgetArea(QFrame):
    def __init__(self, parent):
        super().__init__(parent)

        p = self.palette()
        p.setColor(self.backgroundRole(), QColor(205,205,255))
        self.setPalette(p)
        self.setAutoFillBackground(True)
        self.setFrameShape(QFrame.Box)

        self.displaysGroups = []
        self.globalBounds = [0,0,self.width(),self.height()]

    def SetGlobalBounds(self, bounds):
        self.globalBounds = bounds.copy()
        for group in self.displaysGroups:
            group.UpdateGlobalBounds(self.globalBounds)

    def GetAllGlobalOffsets(self):
        offsets = {}

        for group in self.displaysGroups:
            offsets[group.groupID] = group.GetGlobalOffset()

        return offsets

    def resizeEvent(self, event):
        super().resizeEvent(event)

        for group in self.displaysGroups:
            group.SetWidgetBounds([0, 0, self.width(), self.height()])
            group.UpdateGeometry()

    def AddDisplayGroup(self, displays, groupID):
        displayGroup = DisplayWidgetGroup(self, groupID)

        for display in displays:
            displayGroup.AddDisplay(display)
            
        self.displaysGroups.append(displayGroup)

