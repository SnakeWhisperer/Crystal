
// === Crystal Parking - Entrance Code with Integrated Printer ===
// Date: 2025-07-14
// Uses: Serial1 (Printer), Serial2 (RS232 Scanner), Serial3 (RS485 PC), Button on pin 8

#include <SoftwareSerial.h>
#define BUTTON_PIN 8
#define OPEN_BARRIER 28
#define CLOSE_BARRIER 30
#define CAR_PRES_1 24
#define CAR_PRES_2 26

// RS-485 direction control (tie DE and /RE together to this pin)
#define RS485_DE_RE_PIN 22

// --- Entry flow state ---
bool CAR_ENTERING = false;        // Set when ticket is issued, means there's an ongoing entering process
bool cp1_now, cp2_now;            // Current raw states. cp1 is at the printer panel, cp2 is below the barrier
// These are used to detect the rising and falling edges
// They are set to HIGH when defined, but the first loop resets them, meaning that there was no car there
bool cp1_prev = HIGH, cp2_prev = HIGH; 
bool gateOpened = false;

uint32_t openAt = 0;              // When to open after ticket
uint32_t closeAt = 0;             // Millis() when we’re allowed to close (0 = no timer)
uint32_t guardExpireAt = 0;       // cancel/close if CP2 never arrives

const uint32_t CLEAR_HOLD_MS = 5000;   // 5 s after CP2 clears
const uint32_t OPEN_DELAY_MS = 1000;  // 1 s grace to print & grab ticket
const uint32_t ENTRY_GUARD_MS = 30000; // 30 s max to see CP2 after ticket


bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
String barcodeData = "";
String date = "";
String time = "";


void handleEntryFlow() {
  // Read current sensor levels. HIGH = There's a car there
  cp1_now = carPresent(CAR_PRES_1);
  cp2_now = carPresent(CAR_PRES_2);

  // Detect edges
  bool cp1_arrive = (!cp1_prev && cp1_now); // went from false→true → car arrived at loop 1
  bool cp1_leave  = (cp1_prev && !cp1_now); // went from true→false → car left loop 1

  bool cp2_arrive = (!cp2_prev && cp2_now); // same as above - car arrived under barrier
  bool cp2_leave  = (cp2_prev && !cp2_now); // same as above -  car left under barrier

  // === Main logic when we're handling a car that just got a ticket ===
  // CAR_ENTERING is set when a ticket is printed and an entry process starts
  if (CAR_ENTERING) {

    // If the car leaves before the gate is opened, the entry process needs to be canceled.
    // This is practically impossible, as the car would need to leave within 1 second after the ticket is printed
    if (!gateOpened && cp1_leave) {
      CAR_ENTERING = false;
      openAt = closeAt = guardExpireAt = 0;
      Serial.println("[Entrance] Panel loop cleared before open -> entry canceled.");
    }

    // When the time has passed (1s) after printing a ticket and the gate hasn't opened, open it
    if (!gateOpened && millis() >= openAt) {
      pulse(OPEN_BARRIER, 250);     // 200–300 ms pulse to the opener
      gateOpened = true;
      Serial.println("[Entrance] Gate opening (after ticket grace).");
    }

    // When the car gets under the barrier, cancel the 30s (change this)
    // wait to close if the car never gets there after the ticket is printed
    if (cp2_arrive && guardExpireAt){
      guardExpireAt = 0;
      Serial.println("[Entrance] CP2 arrived -> guard canceled.");
    }

    // When CP2 (under-barrier) goes from present -> clear, start 5 s hold
    if (cp2_leave) {
      closeAt = millis() + CLEAR_HOLD_MS;
      Serial.println("[Entrance] Under-barrier cleared -> hold timer started.");
    }
    // // If CP2 becomes present again before the timer expires, extend the hold
    // if (cp2_fall && closeAt != 0) {
    //   closeAt = millis() + CLEAR_HOLD_MS;
    //   Serial.println("[Entrance] Vehicle re-entered under-barrier -> extend hold.");
    // }

    // When the hold expires and CP2 is still clear, close the gate and finish
    if (closeAt != 0 && millis() >= closeAt && !cp2_now) {
      pulse(CLOSE_BARRIER, 250);
      CAR_ENTERING = false;
      closeAt = 0;
      Serial.println("[Entrance] Hold elapsed and area clear -> gate closing, entry complete.");
    }

    // When the guard time (30s - change this) passes, and there is still no car there, close the barrier
    if (guardExpireAt && millis() >= guardExpireAt && !cp2_now){
      // If the gate is opened, close it and print the message
      if (gateOpened) {
        pulse(CLOSE_BARRIER, 250);
        Serial.println("[Entrance] Guard timeout -> closing without CP2.");

      } else {
        Serial.println("[Entrance] Guard timeout -> entry canceled before open.");
      }
      // Whether the gate is opened or not, do the same - cancel the entry process
      CAR_ENTERING = false;
      gateOpened = false;
      openAt = closeAt = guardExpireAt = 0;
    }
  }

  // Latch previous for next loop
  cp1_prev = cp1_now;
  cp2_prev = cp2_now;
}


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

inline void pulse(uint8_t pin, uint16_t ms) {
  digitalWrite(pin, HIGH);
  delay(ms);
  digitalWrite(pin, LOW);
}

inline bool carPresent(uint8_t pin) {        // with INPUT_PULLUP: LOW = present
  return digitalRead(pin) == LOW;
}

// Call when you’ve just printed the ticket and want to open the gate
inline void startEntry() {
  if (!CAR_ENTERING) {
    CAR_ENTERING = true;
    gateOpened = false;
    openAt = millis() + OPEN_DELAY_MS;    // schedule open
    closeAt = 0;                  // not closing yet
    guardExpireAt = millis() + ENTRY_GUARD_MS;    // must see CP2 by then
    Serial.println("[Entrance] Ticket issued. Opening scheduled in 1s; waiting for CP2.");
  }
}



void setup() {
  pinMode(OPEN_BARRIER, OUTPUT);
  pinMode(CLOSE_BARRIER, OUTPUT);
  pinMode(CAR_PRES_1, INPUT_PULLUP);
  pinMode(CAR_PRES_2, INPUT_PULLUP);
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
    if (digitalRead(CAR_PRES_1) == LOW && !CAR_ENTERING) {
      // === Generate 18-char random barcode ===
      barcodeData = "CP";
      for (int i = 0; i < 7; i++) {
        char c = "0123456789"[random(10)];
        barcodeData += c;
      }

      // Serial3.println("BUTTON_PRESS " + barcodeData); // Notify PC via RS485
      RS485_sendLine("BUTTON_PRESS " + barcodeData);
      printTicket();                   // Print Ticket via Serial1
      startEntry();
      buttonPressed = false;
      // NOTE: This needs to change. See the exit
    } else if (digitalRead(CAR_PRES_1) == LOW) {
      Serial.println("[Entrance] Button pressed but no car detected. Ignoring.");
      buttonPressed = false;
    } else if (CAR_ENTERING) {
      Serial.println("[Entrance] Button pressed but a car is entering. Ignoring.");
      buttonPressed = false;
    } else {
      buttonPressed = false;
    }

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

  handleEntryFlow();
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

