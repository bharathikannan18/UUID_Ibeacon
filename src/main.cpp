#include <Arduino.h>
#include <bluefruit.h>
#include <SoftwareSerial.h>

void startAdv(void);
void onDataReceived(uint16_t conn_handle, uint8_t *data, uint16_t len);
void scan_callback(ble_gap_evt_adv_report_t* report);

#define MANUFACTURER_ID   0x004C
#define IBEACON_COMPANY_IDENTIFIER 0x004C

uint8_t beaconUuid[16] = {
  0x01, 0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78,
  0x89, 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0
};

BLEBeacon beacon(beaconUuid, 0x0102, 0x0304, -54);

BLEUart bleuart;
BLECharacteristic txCharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
BLECharacteristic rxCharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLENotify);

#define rxPin 3
#define txPin 4

SoftwareSerial mySerial(rxPin, txPin);

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600);
  while (!Serial) delay(10);

  Bluefruit.begin();
  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(0);
  Bluefruit.setName("Bobby");
  beacon.setManufacturer(MANUFACTURER_ID);

  // Initialize BLE UART
  bleuart.begin();

  // Start advertising
  startAdv();
  Serial.println("Broadcasting beacon, open your beacon app to test");

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.filterRssi(-80);

  Bluefruit.Scanner.setInterval(160, 80);
  Bluefruit.Scanner.useActiveScan(true);
  Bluefruit.Scanner.start(0);
}

void startAdv(void) {
  Bluefruit.Advertising.setBeacon(beacon);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 160);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void onDataReceived(uint16_t conn_handle, uint8_t *data, uint16_t len) {
  // Handle received data from BLE UART
  // (You can implement this if needed)
}

void scan_callback(ble_gap_evt_adv_report_t* report) {
  uint8_t buffer[32];
  memset(buffer, 0, sizeof(buffer));

  if (report->data.len >= 25 && report->data.p_data[0] == 0x02 && report->data.p_data[1] == 0x01
      && report->data.p_data[2] == 0x06 && report->data.p_data[3] == 0x1A 
      && report->data.p_data[4] == 0xFF && report->data.p_data[5] == IBEACON_COMPANY_IDENTIFIER) {
    uint8_t* data = report->data.p_data + 9; 
    String uuidString = "";

    // Extract the UUID
    for (int i = 0; i < 16; i++) {
      uuidString += String(data[i], HEX);
    }

    // Print the full UUID
    Serial.print("Full UUID: ");
    Serial.println(uuidString);

    // Extract the last 4 digits of the UUID
    String last6Digits = uuidString.substring(uuidString.length() - 6);

    Serial.print("Last 6 digits of UUID: ");
    Serial.println(last6Digits);

    // Send the last 4 digits to SoftwareSerial
    mySerial.println(last6Digits);
  }
  Bluefruit.Scanner.resume();
}

void loop() {
  // Check for incoming BLE UART data
  if (bleuart.available()) {
    String data = bleuart.readStringUntil('\n');
    Serial.print("Received data: ");
    Serial.println(data);

    // Send the received character to the rxCharacteristic using notifications
    rxCharacteristic.notify(data.c_str());
  }

  // Your loop code...
}
