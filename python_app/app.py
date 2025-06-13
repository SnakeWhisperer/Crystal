import sys
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QTextEdit
from PyQt5.QtCore import QTimer
import serial

class BarcodeReaderApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Crystal Parking - RS485 Barcode Reader")
        self.setGeometry(300, 300, 500, 300)

        # UI Elements
        self.layout = QVBoxLayout()
        self.label = QLabel("Scanned Barcodes:")
        self.text_area = QTextEdit()
        self.text_area.setReadOnly(True)

        self.layout.addWidget(self.label)
        self.layout.addWidget(self.text_area)
        self.setLayout(self.layout)

        # Serial setup
        try:
            self.serial_port = serial.Serial(
                port='COM7',      # Change this to your actual COM port
                baudrate=9600,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1
            )
        except serial.SerialException as e:
            self.text_area.setPlainText(f"Failed to connect: {e}")
            self.serial_port = None

        # Timer to poll datatera
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial_data)
        self.timer.start(100)  # Check every 100 ms

    def read_serial_data(self):
        if self.serial_port and self.serial_port.in_waiting:
            data = self.serial_port.read(self.serial_port.in_waiting).decode(errors='ignore')
            if data.strip():
                self.text_area.append(data.strip())

if __name__ == '__main__':
    app = QApplication(sys.argv)
    reader_app = BarcodeReaderApp()
    reader_app.show()
    sys.exit(app.exec_())
