from datetime import datetime
import json
import sys
import random
import re
from PyQt5.QtWidgets import (QApplication, QWidget, QVBoxLayout, QLabel, QTextEdit,
                             QPushButton, QHBoxLayout, QLineEdit)
from PyQt5.QtCore import QTimer
import requests
import serial

class BarcodeReaderApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Estacionamiento")
        self.setGeometry(300, 300, 1000, 300)

        # Serial State
        self.serial_port = None
        self.connection_lost_logged = False
        self.connection_restored_logged = False
        self.serial_available = False

        # Main Layout
        self.layout = QVBoxLayout()

        self.label = QLabel("Registro:")
        self.text_area = QTextEdit()
        self.text_area.setReadOnly(True)

        # COM port row
        self.port_row = QHBoxLayout()
        self.port_label = QLabel("Puerto COM:")
        self.port_input = QLineEdit()
        self.port_input.setText("COM6") # default value
        self.btn_connect = QPushButton("Conectar")
        self.btn_connect.clicked.connect(self.on_connect_clicked)

        self.port_row.addWidget(self.port_label)
        self.port_row.addWidget(self.port_input)
        self.port_row.addWidget(self.btn_connect)


        # Hello button
        self.btn_send_hello = QPushButton("Send HELLO")
        self.btn_send_hello.clicked.connect(self.send_hello)

        self.layout.addLayout(self.port_row)
        self.layout.addWidget(self.label)
        self.layout.addWidget(self.text_area)
        self.layout.addWidget(self.btn_send_hello)  # NEW
        self.setLayout(self.layout)

        # Timer to poll datatera
        self.timer = QTimer()
        self.timer.timeout.connect(self.read_serial_data)
        self.timer.start(100)  # Check every 100 ms

        # Auto-connect on startup
        self.connect_serial(self.port_input.text().strip())


        # Serial setup
        try:
            self.serial_port = serial.Serial(
                port='COM6',      # Change this to your actual COM port
                baudrate=9600,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1
            )
        except serial.SerialException as e:
            self.log(f"No se pudo establecer conexión: {e}")
            # self.text_area.setPlainText(f"No se pudo establecer conexión: {e}")
            self.serial_port = None

    def log(self, message: str):
        self.text_area.append(f"{message}     |     {str(datetime.now())[0:-7]}")
        self.text_area.append("\n")
        self.trim_text_area()

    def set_serial_status(self, available: bool):
        if available and not self.serial_available:
            self.serial_available = True
            self.log("Comunicación serial disponible.")
        elif not available and self.serial_available:
            self.serial_available = False
            self.log("Comunicación serial perdida.")

    def disconnect_serial(self):
        """Close current serial connection if open."""
        if self.serial_port is not None:
            try:
                if self.serial_port.is_open:
                    self.serial_port.close()
                    self.log("Puerto serial cerrado.")
            except Exception as e:
                self.log(f"Error cerrando puerto: {e}")
            finally:
                self.serial_port = None

    def connect_serial(self, port_name: str):
        """Try to connect to the given COM port."""
        self.disconnect_serial()

        if not port_name:
            self.log("Debe ingresar un puerto COM.")
            return
    
        try:
            self.serial_port = serial.Serial(
                port=port_name,
                baudrate=9600,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1
            )
            self.log(f"Conectado a {port_name}")
        except serial.SerialException as e:
            self.serial_port = None
            self.log(f"No se pudo establecer conexión con {port_name}: {e}")
    
    def on_connect_clicked(self):
        """Reconnect using the COM port currently written in the input."""
        port_name = self.port_input.text().strip()
        self.connect_serial(port_name)

    

    # NEW: send HELLO to Arduino over RS-485
    def send_hello(self):
        if not self.serial_port or not self.serial_port.is_open:
            self.log("Serial no disponible.")
            return
        
        # if not self.serial_port:
        #     self.text_area.append("⚠️ Serial no disponible.")
        #     return

        try:
            self.serial_port.write(b"HELLO\n")
            self.serial_port.flush() # check this if there are errors here
            self.log("Enviado a Arduino: HELLO")
            # self.text_area.append("Enviado a Arduino: HELLO")

        except serial.SerialException as e:
            self.log(f"Error enviando: {e}")

    def generate_barcode_with_current_time(self):
        """
        Generates a unique barcode string using current date and time.
        Format: CPYYYYMMDDHHMMSS
        """
        now = datetime.now()
        return now.strftime("CP%Y%m%d%H%M%S")

    def read_serial_data(self):
        url_generate = "https://ingcoders.com/parking/index.php/api/generate_code"
        url_code = "https://ingcoders.com/parking/index.php/api/check_code"
        url_entry_code = "https://ingcoders.com/parking/index.php/api/card_code"

        if not self.serial_port:
            self.set_serial_status(False)
            return
        
        try:
            if not self.serial_port.is_open:
                self.set_serial_status(False)
                return
            
            self.set_serial_status(True)

            if self.serial_port.in_waiting <= 0:
                return
            
            data = self.serial_port.readline().decode(errors='ignore').strip()
            if not data:
                return  # Skip empty data
            
            print(f"[DEBUG] Raw serial input: {data}")

            if "BUTTON_PRESS" in data:
                payload_generate = {'number': '1'} # Send as string, like in curl
                headers = {
                    # Pretend to be curl
                    "User-Agent": "curl/8.0.0",
                    "Accept": "*/*",
                    "Content-Type": "application/x-www-form-urlencoded",
                }

                response = requests.post(url_generate, data=payload_generate, headers=headers, timeout=10)

                s = response.text
                print(f'Response: {response.text}')
                response_data = json.loads(s)
                code = response_data["code"]

                self.log("Se solicitó un código de barras al servidor")
                self.log(f"Se recibió el código de barras: {code}")
                # self.text_area.append("Se solicitó un código de barras al servidor")
                # self.text_area.append(f"Got the barcode: {code}")
                self.serial_port.write((code + "\n").encode("ascii"))
                self.serial_port.flush()


                generated = self.generate_barcode_with_current_time()
                # self.serial_port.write(f"{generated}\n".encode())
                # match = re.match(r'^BUTTON_PRESS\s+(.+)$', data)

                # if match:
                #     barcode = match.group(1)
                #     self.text_area.append(f"\nNuevo código de barras generado: {barcode}\n")
                # generated = self.generate_barcode()
                # self.text_area.append(f"\nNuevo código de barras generado: {generated}\n")

            elif "EXIT SCAN" in data:
                print(f"Barcode: {data}")
                trimmed_barcode = data[-9:]
                payload_code = {'code': f'{trimmed_barcode}'} # Send as string, like in curl
                headers = {
                    # Pretend to be curl
                    "User-Agent": "curl/8.0.0",
                    "Accept": "*/*",
                    "Content-Type": "application/x-www-form-urlencoded",
                }

                response_code = requests.post(url_code, data=payload_code, headers=headers, timeout=10)
                exit_s = response_code.text
                print(f'Response: {response_code.text}')
                exit_response_data = json.loads(exit_s)
                status = exit_response_data["status"]

                self.log(f"Se presentó el código de barras en la salida [{trimmed_barcode}]")
                # self.text_area.append(f"Se presentó el código de barras en la salida [{trimmed_barcode}]")

                if status == 0:
                    self.log(f"Código de barras en la salida fue rechazado [{trimmed_barcode}]\n")
                    # self.text_area.append(f"Código de barras en la salida fue rechazado [{trimmed_barcode}]\n")
                    print('Here')
                    # self.serial_port.write(b"EXIT_OK \n")
                    self.serial_port.write(b"EXIT_NO\n")
                    self.serial_port.flush()
                elif status == 1:
                    self.log(f"Código de barras en la salida fue aceptado [{trimmed_barcode}]\n")
                    # self.text_area.append(f"Código de barras en la salida fue aceptado [{trimmed_barcode}]\n")
                    self.serial_port.write(b"EXIT_OK \n")
                    self.serial_port.flush()

            elif "ENTRY_SCAN" in data:
                print(f"Entry barcode: {data}")
                trimmed_barcode_entry = data[-9:]
                payload_entry_code = {'code': f'{trimmed_barcode_entry}'} # Send as string, like in curl
                headers_entry_code = {
                    # Pretend to be curl
                    "User-Agent": "curl/8.0.0",
                    "Accept": "*/*",
                    "Content-Type": "application/x-www-form-urlencoded",
                }

                response_entry_code = requests.post(url_entry_code, data=payload_entry_code, headers=headers_entry_code, timeout=10)
                entry_s = response_entry_code.text
                print(f'Response entry code: {response_entry_code.text}')
                entry_code_response_data = json.loads(entry_s)
                entry_code_status = entry_code_response_data["status"]

                self.log(f"Se presentó el código de barras en la entrada [{trimmed_barcode_entry}]")
                # self.text_area.append(f"Se presentó el código de barras en la entrada [{trimmed_barcode_entry}]")

                if entry_code_status == 0:
                    self.log(f"Código de barras de entrada fue rechazado [{trimmed_barcode_entry}]\n")
                    # self.text_area.append(f"Código de barras de entrada fue rechazado [{trimmed_barcode_entry}]\n")
                    self.serial_port.write(b"ENTRY_NO\n")

                elif entry_code_status == 1:
                    self.log(f"Código de barras de entrada fue aceptado [{trimmed_barcode_entry}]\n")
                    # self.text_area.append(f"Código de barras de entrada fue aceptado [{trimmed_barcode_entry}]\n")
                    self.serial_port.write(b"ENTRY_OK\n")
            elif data.strip():
                print(data)
                self.log(f"\nNuevo código de barras leído: {data.strip()}\n")
                # self.text_area.append(f"\nNuevo código de barras leído: {data.strip()}\n")
            # if "SCAN:" in data:
            #     scanned = data.strip().replace("SCAN:", "")
            #     self.text_area.append(f"📦 Código escaneado: {scanned}")
        
        except serial.SerialException:
            self.set_serial_status(False)
        except requests.RequestException as e:
            self.log(f"Error del servidor: {e}")
            # self.text_area.append(f"Error del servidor: {e}")
        except Exception as e:
            self.log(f"Error inesperado: {e}")
            # self.text_area.append(f"Error inesperado: {e}")

        

    def trim_text_area(self, max_blocks=500):
        doc = self.text_area.document()
        
        while doc.blockCount() > max_blocks:
            cursor = self.text_area.textCursor()
            cursor.movePosition(cursor.Start)
            cursor.select(cursor.BlockUnderCursor)
            cursor.removeSelectedText()
            cursor.deleteChar()
            
    def generate_barcode(self):
        return f"CP-{random.randint(100000, 999999)}"

if __name__ == '__main__':
    app = QApplication(sys.argv)
    reader_app = BarcodeReaderApp()
    reader_app.show()
    sys.exit(app.exec_())
