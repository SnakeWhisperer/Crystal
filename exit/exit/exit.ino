

// RS-485 direction control (tie DE and /RE together to this pin)
#define RS485_DE_RE_PIN 9
#define OPEN_BARRIER 28
#define CLOSE_BARRIER 30
#define CAR_PRES_1 24
#define CAR_PRES_2 26
#define US_TRIG 31
#define US_ECHO 29


// --- Exit flow state ---
bool CAR_EXITING = false;        // set when ticket read and approved
bool cp1_now, cp2_now;            // current raw states (LOW = present)
bool cp1_prev = HIGH, cp2_prev = HIGH;
bool gateOpened = false;

uint32_t openAt = 0;              // when to open after ticket approval
uint32_t closeAt = 0;             // millis() when we’re allowed to close (0 = no timer)
uint32_t guardExpireAt = 0;       // cancel/close if CP2 never arrives

// (optional future) timeouts
const uint32_t CLEAR_HOLD_MS = 5000;   // 5 s after CP2 clears
const uint32_t OPEN_DELAY_MS = 1000;  // 1 s grace to press gas
const uint32_t EXIT_GUARD_MS = 30000; // 30 s max to see CP2 after ticket approval

void handleExitFlow() {
  // Read current sensor levels. HIGH = There's a car there
  cp1_now = carPresent(CAR_PRES_1);
  // cp2_now = carPresent(CAR_PRES_2);

  // Detect edges
  bool cp1_arrive = (!cp1_prev && cp1_now); // went from false→true → car arrived at loop 1
  bool cp1_leave  = (cp1_prev && !cp1_now); // went from true→false → car left loop 1

  bool cp2_arrive = (!cp2_prev && cp2_now); // same as above - car arrived under barrier
  bool cp2_leave  = (cp2_prev && !cp2_now); // same as above -  car left under barrier

  // === Main logic when we're handling a car that just presented a ticket ===
  // CAR_EXITING is set when a ticket is presented and an exit process starts
  if (CAR_EXITING) {

    // If the car leaves before the gate is opened, the exit process needs to be canceled.
    // This is practically impossible, as the car would need to leave within 1 second after the ticket is presented
    if (!gateOpened && cp1_leave) {
      CAR_EXITING = false;
      openAt = closeAt = guardExpireAt = 0;
      Serial.println("[Exit] Panel loop cleared before open -> exit canceled.");
    }

    // When the time has passed (1s) after presenting and approving a ticket and the gate hasn't opened, open it
    if (!gateOpened && millis() >= openAt){
      pulse(OPEN_BARRIER, 250);
      gateOpened = true;
      Serial.println("[Exit] Gate opening (after gas grace).");
    }

    // When the car gets under the barrier, cancel the 30s (change this)
    // wait to close if the car never gets there after the ticket is presented and approved
    if (cp2_arrive && guardExpireAt){
      guardExpireAt = 0;
      Serial.println("[Exit] CP2 arrived -> guard canceled.");
    }

    // When CP2 (under-barrier) goes from present -> clear, start 5 s hold
    if (cp2_leave) {
      closeAt = millis () + CLEAR_HOLD_MS;
      Serial.println("[Exit] Under-barrier cleared -> hold timer started.");
    }

    // When the hold expires and CP2 is still clear, close the gate and finish
    if (closeAt != 0 && millis() >= closeAt && !cp2_now) {
      pulse(CLOSE_BARRIER, 1050);
      CAR_EXITING = false;
      closeAt = 0;
      Serial.println("[Exit] Hodl elapsed and area clear -> gate closing, exit complete.");
    }

    // When the guard time (30s - change this) passes, and there is still no car there, close the barrier
    if (guardExpireAt && millis() >= guardExpireAt && !cp2_now) {
      // If the gate is opened, close it and print the message
      if (gateOpened) {
        pulse(CLOSE_BARRIER, 250);
        Serial.println("[Exit] Guard timeout -> closing without CP2.");

      } else {
        pulse(CLOSE_BARRIER, 250);
        Serial.println("[Exit] Guard timeout -> exit canceled before open");
      }
      // Whether the gate is opened or not, do the same - cancel the exit process
      CAR_EXITING = false;
      gateOpened = false;
      openAt = closeAt = guardExpireAt = 0;
    }
  }
  // Latch previous for next loop
  cp1_prev = cp1_now;
  cp2_prev = cp2_now;

}

inline void pulse(uint8_t pin, uint16_t ms) {
  digitalWrite(pin, HIGH);
  delay(ms);
  digitalWrite(pin, LOW);
}

inline bool carPresent(uint8_t pin) {        // with INPUT_PULLUP: LOW = present
  return digitalRead(pin) == LOW;
}


// Call when you’ve just approved the ticket and want to open the gate
inline void startExit() {
  if (!CAR_EXITING) {
    CAR_EXITING = true;
    gateOpened = false;
    openAt = millis() + OPEN_DELAY_MS;    // schedule open
    closeAt = 0;                  // not closing yet
    guardExpireAt = millis() + EXIT_GUARD_MS;    // must see CP2 by then
    Serial.println("[Exit] Ticket read (approved?). Opening scheduled in 1s; waiting for CP2.");
  }
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


long readUltrasonicCM() {
  // clear TRIG
  digitalWrite(US_TRIG, LOW);
  delayMicroseconds(2);

  // 10 us trigger pulse
  digitalWrite(US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(US_TRIG, LOW);

  // measure ECHO pulse length
  long duration = pulseIn(US_ECHO, HIGH, 30000UL); // 30 ms timeout (~5 m max)

  // Distance = (pulse time x speed of sound) / 2
  // convert to cm(speed of sound ~343 m/s)
  long distance = duration / 58; // us to cm

  if (duration == 0) return -1; // timeout / no echo
  return distance;
}

void setup() {
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  pinMode(OPEN_BARRIER, OUTPUT);
  pinMode(CLOSE_BARRIER, OUTPUT);
  pinMode(CAR_PRES_1, INPUT_PULLUP);
  pinMode(CAR_PRES_2, INPUT_PULLUP);
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  pinMode(US_TRIG, OUTPUT);
  pinMode(US_ECHO, INPUT);
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
    if (digitalRead(CAR_PRES_1) == LOW && !CAR_EXITING) {
        // Send to the PC over RS-485 using the same medium-control logic as entrance
        // (TX window open -> send -> close -> back to RX)
        RS485_sendLine(String("EXIT SCAN ") + code);
        startExit();
      }
    } else if (digitalRead(CAR_PRES_1) != LOW) {
      Serial.println("[Exit] Ticket presented, but there's no car there. Ignoring.");
    } else if (CAR_EXITING) {
      Serial.println("[Exit] Ticket presented but a car is exiting.");
      // NOTE: These conditionals will need to change when the ticket is evaluated
    }
  }

  

  long d = readUltrasonicCM();
  if (d > 0 && d < 120) { // within 1 m, car under barrier
    Serial.print("[Exit] Car detected at barrier");
    Serial.print(d);
    Serial.println(" cm");
    // set a boolean like EXIT_CAR_PRESENT = true;
    cp2_now = true;
  } else {
    // EXIT_CAR_PRESENT = false;
    cp2_now = false;
  }

  handleExitFlow();

  delay(100); // don't hammer it too fast, 10 Hz is plenty
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


