

// RS-485 direction control (tie DE and /RE together to this pin)
#define RS485_DE_RE_PIN 8

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


void setup() {
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  RS485_beginRX();          // default to listening
  Serial.begin(115200);     // Debug
  Serial2.begin(19200);     // RS232 Scanner
  Serial3.begin(9600);      // RS485 to PC

  Serial.println("System Ready.");
}


void loop() {

  // 1) Check for commands from the PC (RS-485)
  pollHost();

  if (Serial2.available()) {
    String code = Serial2.readStringUntil('\n');  // read until LF
    code.trim();                                  // strip CR/LF if present
    if (code.length() > 0) {
      Serial.print("Scan: ");
      Serial.println(code);                       // print once, with newline

      // Send to the PC over RS-485 using the same medium-control logic as entrance
      // (TX window open -> send -> close -> back to RX)
      RS485_sendLine(String("EXIT SCAN ") + code);
    }
  }
}


void handleHostLine(const String& line) {
  // if (line == "HELLO") {
  //   // Do something visible in Serial Monitor:
  //   Serial.println("Hello from PC");
  //   // (Optional) and/or acknowledge back to the PC over RS-485:
  //   RS485_sendLine("HELLO_OK");
  // }
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


