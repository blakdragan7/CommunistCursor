from PySide2.QtWidgets import QWidget, QFrame, QGridLayout, QPushButton
from PySide2.QtGui import QColor
from PySide2.QtCore import Qt, Signal

import sys
import json

from DisplayWidgetArea import DisplayWidgetArea

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.socket = None

        self.setWindowTitle("CommunistCursor Display Setup")
        p = self.palette()
        p.setColor(self.backgroundRole(), QColor(240,240,245))
        self.setPalette(p)
        self.setGeometry(200,200,500,500)

        self.displayArea = DisplayWidgetArea(self)
        self.cancelButton = QPushButton("Cancel", self)
        self.okButton = QPushButton("Ok", self)

        self.cancelButton.clicked.connect(self.CancelPressed)
        self.okButton.clicked.connect(self.OkPressed)

        layout = QGridLayout(self)
        layout.addWidget(self.displayArea,  0,0,10,10)
        layout.addWidget(self.cancelButton, 10,4,1,3)
        layout.addWidget(self.okButton,     10,7,1,3)

        self.setLayout(layout)

    def CancelPressed(self):
        sys.exit(0)

    def OkPressed(self):
        offsets = self.displayArea.GetAllGlobalOffsets()
        if self.socket:
            self.socket.sendall(json.dumps(offsets).encode())

        sys.exit(0)

    def SetServiceSocket(self, socket):
        self.socket = socket

    def SetupDisplaysWithEntityData(self, entityData):
        entityArray = entityData["entites"]
        for entity in entityArray:
            self.displayArea.AddDisplayGroup(entity["displays"], entity["id"], entity["name"])
        self.displayArea.MoveGroupsThatAreColliding();

        self.displayArea.SetGlobalBounds(entityData["globalBounds"])

    def testSetupDisplays(self):
        displays = [[0,0,1920,1080],[1920,0,1920,1080]]
        self.displayArea.AddDisplayGroup(displays, "0")
        #displays = [[0,0,1920,1200],[0,1200,1920,1080]]
        #self.displayArea.AddDisplayGroup(displays, "1")

        self.displayArea.SetGlobalBounds([-3840, -3840, 3840, 3840])

