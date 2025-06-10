void setup() {
    Serial.begin(115200);                                       // Debug to Serial Monitor
    Serial2.begin(19200, SERIAL_8N1);                           // Scanner
    Serial3.begin(9600);                                        // RS485 (via MAX485)
    Serial.println("System ready...");
}

void loop() {
    while (Serial2.available()) {  // Read data immediately
        char data = Serial2.read();
        Serial.write(data);  // Print character as it arrives


        Serial3.write(data);            // Send via RS485 to PC
    }
}
