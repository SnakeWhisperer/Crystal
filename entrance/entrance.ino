
// === Crystal Parking - Entrance Code with Integrated Printer ===
// Date: 2025-07-14
// Uses: Serial1 (Printer), Serial2 (RS232 Scanner), Serial3 (RS485 PC), Button on pin 8

#include <SoftwareSerial.h>
#define BUTTON_PIN 8

// RS-485 direction control (tie DE and /RE together to this pin)
#define RS485_DE_RE_PIN 22

inline void RS485_beginRX() {
  // LOW: receiver enabled, driver disabled
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

inline void RS485_beginTX() {
  // HIGH: receiver disabled, driver enabled
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  delayMicroseconds(100); // guard time so driver actually enables
}

// Safe send over Serial3 (handles TX/RX switching and flush)
inline void RS485_sendLine(const String& line) {
  RS485_beginTX();
  Serial3.print(line);
  Serial3.print('\n');        // line terminator for the PC
  Serial3.flush();            // ensure all bytes shifted out
  delayMicroseconds(200);     // let the last byte finish on the wire
  RS485_beginRX();
}


bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
String barcodeData = "";
String date = "";
String time = "";

void setup() {
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  RS485_beginRX();          // default to listening
  // Keep your existing Serial3.begin(…); value (use same baud as your PC)


  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);     // Debug
  Serial1.begin(9600);      // Printer
  Serial2.begin(9600);     // RS232 Scanner
  Serial3.begin(9600);      // RS485 to PC

  Serial.println("System Ready.");
}

void loop() {
  // === Handle Button Press with Debounce ===
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      if (currentButtonState == LOW) {
        buttonPressed = true;
      }
    }
  }

  lastButtonState = reading;

  // === If Button Was Pressed, Send Signal & Print Ticket ===
  if (buttonPressed) {
    // === Generate 18-char random barcode ===
    barcodeData = "CP";
    for (int i = 0; i < 7; i++) {
      char c = "0123456789"[random(10)];
      barcodeData += c;
    }

    // Serial3.println("BUTTON_PRESS " + barcodeData); // Notify PC via RS485
    RS485_sendLine("BUTTON_PRESS " + barcodeData);
    printTicket();                   // Print Ticket via Serial1
    buttonPressed = false;
  }

  // 1) Check for commands from the PC (RS-485)
  pollHost();

  // === Handle RS232 Scanner Input (Serial2) ===
  if (Serial2.available()) {
    String scannedData = Serial2.readStringUntil('\n');
    scannedData.trim();
    if (scannedData.length() > 0) {
      Serial.println("Scanned: " + scannedData);
      // Serial3.println(scannedData);
      RS485_sendLine(scannedData);
      // You can add logic here if needed
    }
  }
}

void handleHostLine(const String& line) {
  if (line == "HELLO") {
    // Do something visible in Serial Monitor:
    Serial.println("Hello from PC");
    // (Optional) and/or acknowledge back to the PC over RS-485:
    RS485_sendLine("HELLO_OK");
  }
  // You can extend with more commands later
  // else if (line.startsWith("PRINT:")) { ... }
}

void pollHost() {
  static String rxBuf;
  while (Serial3.available()) {
    char c = (char)Serial3.read();
    if (c == '\n') {
      rxBuf.trim();
      if (rxBuf.length()) handleHostLine(rxBuf);
      rxBuf = "";
    } else if (c != '\r') {
      rxBuf += c;
    }
  }
}


void printTicket() {
  Serial.println("I'm trying to print a ticket");

  Serial1.write(27); Serial1.write('@');  // ESC @: Initialize

  // Align center
  Serial1.write(0x1B); Serial1.write('a'); Serial1.write(1);

  // Set normal font (undo double size)
  Serial1.write(0x1D); Serial1.write('!'); Serial1.write(0x00);

  // Print header
  Serial1.println("Centro Comercial Cristal");
  Serial1.println("BIENVENIDO!");
  Serial1.println("========================");
  Serial1.println();

  // Print date and time
  Serial1.print(" FECHA: "); Serial1.println(__DATE__);
  Serial1.print(" HORA:  "); Serial1.println(__TIME__);
  Serial1.println("========================");
  Serial1.println();

  // === Print Barcode ===

  // Set HRI (text) position below
  Serial1.write(0x1D); Serial1.write('H'); Serial1.write(2);

  // Set barcode width (1-6)
  Serial1.write(0x1D); Serial1.write('w'); Serial1.write(2);

  // Set barcode height (max 255)
  Serial1.write(0x1D); Serial1.write('h'); Serial1.write(100);

  // Select CODE39 (value 4)
  Serial1.write(0x1D); Serial1.write('k'); Serial1.write(4);

  // Send barcode data (CODE39 must be printable ASCII)
  Serial1.print(barcodeData);  // e.g., "CP123456"
  Serial1.write(0x00);         // Null terminator required for CODE39

  Serial1.println();
  Serial1.println("========================");
  Serial1.println("  Que tenga un buen dia!");
  Serial1.write(10); Serial1.write(10);  // Feed lines
}

