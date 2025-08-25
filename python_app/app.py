from datetime import datetime
import sys
import random
import re
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QLabel, QTextEdit
from PyQt5.QtCore import QTimer
import serial

class BarcodeReaderApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Estacionamiento")
        self.setGeometry(300, 300, 500, 300)

        # UI Elements
        self.layout = QVBoxLayout()
        self.label = QLabel("Códigos de barra:")
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
            self.text_area.setPlainText(f"No se pudo establecer conexión: {e}")
            self.serial_port = None

        # Timer to poll datatera
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial_data)
        self.timer.start(100)  # Check every 100 ms

    def generate_barcode_with_current_time(self):
        """
        Generates a unique barcode string using current date and time.
        Format: CPYYYYMMDDHHMMSS
        """
        now = datetime.now()
        return now.strftime("CP%Y%m%d%H%M%S")

    def read_serial_data(self):
        if self.serial_port and self.serial_port.in_waiting > 0:
            data = self.serial_port.readline().decode(errors='ignore').strip()
            if not data:
                return  # Skip empty data
            print(f"[DEBUG] Raw serial input: {data}")
            if "BUTTON_PRESS" in data:
                generated = self.generate_barcode_with_current_time()
                # self.serial_port.write(f"{generated}\n".encode())
                match = re.match(r'^BUTTON_PRESS\s+(.+)$', data)

                if match:
                    barcode = match.group(1)
                    self.text_area.append(f"\nNuevo código de barras generado: {barcode}\n")
                # generated = self.generate_barcode()
                # self.text_area.append(f"\nNuevo código de barras generado: {generated}\n")
            elif data.strip():
                print(data)
                self.text_area.append(f"\nNuevo código de barras leído: {data.strip()}\n")
            # if "SCAN:" in data:
            #     scanned = data.strip().replace("SCAN:", "")
            #     self.text_area.append(f"📦 Código escaneado: {scanned}")
            
    def generate_barcode(self):
        return f"CP-{random.randint(100000, 999999)}"

if __name__ == '__main__':
    app = QApplication(sys.argv)
    reader_app = BarcodeReaderApp()
    reader_app.show()
    sys.exit(app.exec_())
