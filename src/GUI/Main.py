from PySide2.QtWidgets import QApplication, QErrorMessage

import sys
import json
import socket

from MainWindow import MainWindow

if __name__ == "__main__":

    if len(sys.argv) == 3:
        socketIP = sys.argv[1]
        socketPort = sys.argv[2]
    else:
        socketIP = "127.0.0.1"
        socketPort = 10049

    values = None
    
    app = QApplication()

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((socketIP, socketPort))
        data = s.recv(1024)
        values = json.loads(data)
    except socket.error as e:
        errorDialog = QErrorMessage()
        errorDialog.showMessage(f"CC Service appears to not be running.\
        Please check the service and running and port {socketPort} is not blocked. \nError: {e.strerror}")

        app.exec_()
        sys.exit(0)
    
    window = MainWindow()
    window.SetServiceSocket(s)
    window.SetupDisplaysWithEntityData(values)
    #window.testSetupDisplays()

    window.show()
    app.exec_()