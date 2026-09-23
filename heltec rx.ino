#include <RadioLib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Broches Heltec V3 (ESP32-S3 + SX1262)
#define NSS 8
#define DIO1 14
#define NRST 12
#define BUSY 13
SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);

#define DEVICE_NAME "DLXR_RX_1"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

void setup() {
  Serial.begin(115200);
  
  // Init BLE
  BLEDevice::init(DEVICE_NAME);
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();

  // Init LoRa
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 10, 8, 1.6, false);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[RX] Prêt. En attente de trames LoRa...");
  } else {
    Serial.println("[RX] ERREUR LoRa");
  }
}

void loop() {
  String payload;
  int state = radio.receive(payload);
  
  if (state == RADIOLIB_ERR_NONE) {
    // Remplace le rssi:0 du TX par la vraie valeur mesurée par le RX
    payload.replace("\"rssi\":0", "\"rssi\":" + String(radio.getRSSI()));
    
    Serial.print("Reçu : ");
    Serial.println(payload);

    if (deviceConnected) {
      pCharacteristic->setValue(payload.c_str());
      pCharacteristic->notify();
    }
  }
}
