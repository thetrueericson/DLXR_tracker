#include <RadioLib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ArduinoJson.h> // Indispensable pour lire le nom du tracker

// --- PINS LORA (Heltec V3 - Station Sol) ---
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

// --- GESTION DE LA LISTE DES TRACKERS ---
struct Tracker {
  String id;
  int rssi;
};
Tracker trackers[3]; // On réserve de la place pour 3 trackers sur l'écran
int trackerCount = 0;

void updateOLED() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  if (deviceConnected) {
    u8g2.drawStr(0, 10, "BLE: Connecte (DLXR)");
  } else {
    u8g2.drawStr(0, 10, "BLE: Attente...");
  }
  u8g2.drawLine(0, 13, 128, 13);

  // Affichage dynamique de la liste
  int y = 26; // Position verticale initiale
  if (trackerCount == 0) {
    u8g2.drawStr(0, y, "Ecoute LoRa...");
  } else {
    for (int i = 0; i < trackerCount; i++) {
      String ligne = trackers[i].id + " : " + String(trackers[i].rssi) + " dBm";
      u8g2.setCursor(0, y);
      u8g2.print(ligne);
      y += 14; // On descend pour la ligne suivante
    }
  }
  u8g2.sendBuffer();
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      updateOLED();
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      updateOLED();
    }
};

void setup() {
  Serial.begin(115200);
  
  // Allumage propre de l'écran (VEXT)
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(50);
  
  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 30, "Init Station Sol...");
  u8g2.sendBuffer();

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

  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 10, 8, 1.6, false);
  if (state == RADIOLIB_ERR_NONE) {
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
    int rssi = (int)radio.getRSSI();
    payload.replace("\"rssi\":0", "\"rssi\":" + String(rssi));
    
    // Décodage du JSON pour trouver l'ID de l'avion/drone
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      String id = doc["drone_id"];
      
      // Recherche si le tracker est déjà dans la liste
      bool found = false;
      for (int i = 0; i < trackerCount; i++) {
        if (trackers[i].id == id) {
          trackers[i].rssi = rssi; // Mise à jour du RSSI
          found = true;
          break;
        }
      }
      // S'il est nouveau, on l'ajoute (limité à 3 pour l'écran)
      if (!found && trackerCount < 3) {
        trackers[trackerCount].id = id;
        trackers[trackerCount].rssi = rssi;
        trackerCount++;
      }
      
      updateOLED(); // Rafraîchissement de l'écran
    }

    Serial.print("Reçu : ");
    Serial.println(payload);

    if (deviceConnected) {
      pCharacteristic->setValue(payload.c_str());
      pCharacteristic->notify();
    }
  }
}