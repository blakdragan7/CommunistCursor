from PySide2.QtWidgets import QApplication, QWidget
from PySide2.QtGui import QColor
from PySide2.QtCore import Qt
import sys

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("CommunistCursor Display Setup")
        p = self.palette()
        p.setColor(self.backgroundRole(), QColor(205,205,205))
        self.setPalette(p)

app = QApplication()
window = MainWindow()

window.show()

app.exec_()