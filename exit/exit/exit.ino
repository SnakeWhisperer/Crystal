#include <hidboot.h>
#include <usbhub.h>
#include <usbhid.h>

USB     Usb;
USBHub  Hub(&Usb);
HIDBoot<USB_HID_BOOT_PROTOCOL> HidKeyboard(&Usb);

class KbdRptParser : public KeyboardReportParser {
  void OnKeyDown(uint8_t mod, uint8_t key) override {
    uint8_t ascii = OemToAscii(mod, key);
    if (ascii) {
      Serial.print((char)ascii);
    }
    // Special case: Enter key
    if (key == 40) {
      Serial.println(); // Barcode scanner sends Enter at end
    }
  }
};

KbdRptParser parser;

void PrintAllDevices(UsbDevice *dev, void *arg) {
  Serial.print("Device address: ");
  uint8_t address = dev->address.devAddress;
  Serial.println(address);
}

void setup() {
  Serial.begin(115200);
  Usb.Init();
  Usb.ForEachUsbDevice([](UsbDevice *dev) {
    Serial.print("📦 Device address: ");
    Serial.println(dev->address.devAddress);
  });
  Usb.ForEachUsbDevice(printDeviceInfo);
  HidKeyboard.SetReportParser(0, &parser);
  Serial.println("Ready to scan");
}

void loop() {
  Usb.Task();
}
