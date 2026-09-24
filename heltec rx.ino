#include <RadioLib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- PINS LORA (Heltec V3) ---
#define NSS 8
#define DIO1 14
#define NRST 12
#define BUSY 13
SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);

// --- PINS OLED (Heltec V3) ---
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define VEXT_PIN 36
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

// --- BLE CONFIG ---
#define DEVICE_NAME "DLXR_RX_1"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
int lastRssi = 0;
String lastId = "Aucun";

// 1. On déclare la fonction d'affichage EN PREMIER
void updateOLED() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  if (deviceConnected) {
    u8g2.drawStr(0, 12, "BLE : Connecte (DLXR)");
  } else {
    u8g2.drawStr(0, 12, "BLE : Attente...");
  }

  u8g2.drawLine(0, 16, 128, 16);

  u8g2.setCursor(0, 32);
  u8g2.print("Tracker : ");
  u8g2.print(lastId);

  u8g2.setCursor(0, 48);
  u8g2.print("RSSI : ");
  if (lastRssi != 0) {
    u8g2.print(lastRssi);
    u8g2.print(" dBm");
  } else {
    u8g2.print("--");
  }

  u8g2.sendBuffer();
}

// 2. ENSUITE on déclare les callbacks qui utilisent la fonction
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      updateOLED(); // Le compilateur la connaît maintenant !
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      updateOLED();
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(VEXT_PIN, OUTPUT);
digitalWrite(VEXT_PIN, LOW); // Sur Heltec V3, l'état LOW active l'alimentation de l'écran
delay(50); // Le temps que l'électricité se stabilise dans la dalle
  
  // Init OLED
  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 30, "Init RX en cours...");
  u8g2.sendBuffer();

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
    updateOLED();
  } else {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 30, "ERREUR LORA !");
    u8g2.sendBuffer();
  }
}

void loop() {
  String payload;
  int state = radio.receive(payload);
  
  if (state == RADIOLIB_ERR_NONE) {
    lastRssi = (int)radio.getRSSI();
    lastId = "fil_blanc"; 
    
    payload.replace("\"rssi\":0", "\"rssi\":" + String(lastRssi));
    
    Serial.print("Reçu : ");
    Serial.println(payload);

    if (deviceConnected) {
      pCharacteristic->setValue(payload.c_str());
      pCharacteristic->notify();
    }

    updateOLED();
  }
}
