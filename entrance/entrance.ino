void setup() {
    Serial.begin(115200);
    Serial2.begin(19200, SERIAL_8N1);
    Serial.println("Waiting for barcode scanner input...");
}

void loop() {
    while (Serial2.available()) {  // Read data immediately
        char data = Serial2.read();
        Serial.write(data);  // Print character as it arrives
    }
}
