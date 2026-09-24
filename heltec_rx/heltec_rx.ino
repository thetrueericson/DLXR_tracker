#include <RadioLib.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>

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

// SOLUTION ANTI-CRASH : Utilisation de l'I2C Logiciel (SW_I2C)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, OLED_RST);

// --- DESSIN DU PETIT AVION (16x15 pixels) ---
#define AVION_WIDTH 16
#define AVION_HEIGHT 15
static const unsigned char avion_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x80, 0x03, 0xc0, 0x03, 0xe0, 0x03, 
  0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0xff, 0xf8, 0x7f, 0x00, 0x1f, 0x00, 0x03, 
  0x00, 0x03, 0x00, 0x02, 0x00, 0x00
};

// --- BLE CONFIG ---
#define DEVICE_NAME "DLXR_RX"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool refreshDisplay = true;

// --- LISTE DES TRACKERS ---
struct Tracker {
  String id;
  int rssi;
};
Tracker trackers[3];
int trackerCount = 0;

void updateOLED() {
  u8g2.clearBuffer();
  if (deviceConnected) {
    u8g2.drawStr(0, 10, "BLE: Connecte (DLXR)");
  } else {
    u8g2.drawStr(0, 10, "BLE: Attente...");
  }
  u8g2.drawLine(0, 13, 128, 13);

  int y = 26;
  if (trackerCount == 0) {
    u8g2.drawStr(0, y, "Ecoute LoRa...");
  } else {
    for (int i = 0; i < trackerCount; i++) {
      String ligne = trackers[i].id + " : " + String(trackers[i].rssi) + " dBm";
      u8g2.setCursor(0, y);
      u8g2.print(ligne);
      y += 14; 
    }
  }
  u8g2.sendBuffer();
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      refreshDisplay = true; 
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      refreshDisplay = true;
      BLEDevice::startAdvertising(); 
    }
};

void setup() {
  // Allumage sécurisé de l'écran
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(50);
  
  u8g2.begin();
  u8g2.setContrast(255);
  
  // --- ANIMATION AIR FLEURY CLUB ---
  for (int x = 128; x > -160; x -= 15) {
    u8g2.clearBuffer();
    u8g2.drawXBM(x, 26, AVION_WIDTH, AVION_HEIGHT, avion_bits);
    u8g2.setFont(u8g2_font_ncenB10_tr); 
    u8g2.drawStr(x + 20, 38, "AIR FLEURY CLUB"); // Banderole attachée derrière l'avion
    u8g2.sendBuffer();
  }

  // Retour à la police standard pour la suite
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 30, "Init RX (SF11)...");
  u8g2.sendBuffer();
Serial.println("Étape 1 BLE");
  // Initialisation BLE
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
Serial.println("Étape 2 LORA");
  // Initialisation LoRa - Passage en SF11
  int state = radio.begin(868.0, 125.0, 11, 7, 0x12, 10, 8, 1.6, false);
  if (state == RADIOLIB_ERR_NONE) {
    radio.setDio2AsRfSwitch(true); 
    radio.startReceive(); 
    refreshDisplay = true;
  } else {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 30, "ERREUR LORA !");
    u8g2.sendBuffer();
  }
}

void loop() {
  if (refreshDisplay) {
    updateOLED();
    refreshDisplay = false;
  }

  // Vérification de la réception LoRa
  if (digitalRead(DIO1) == HIGH) {
    String payload;
    int state = radio.readData(payload); 
    
    if (state == RADIOLIB_ERR_NONE) {
      int rssi = (int)radio.getRSSI();
      payload.replace("\"rssi\":0", "\"rssi\":" + String(rssi));
      
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        String id = doc["drone_id"].as<String>();
        
        bool found = false;
        for (int i = 0; i < trackerCount; i++) {
          if (trackers[i].id == id) {
            trackers[i].rssi = rssi;
            found = true;
            break;
          }
        }
        if (!found && trackerCount < 3) {
          trackers[trackerCount].id = id;
          trackers[trackerCount].rssi = rssi;
          trackerCount++;
        }
        refreshDisplay = true; 
      }

      // Transfert instantané vers DLXR
      if (deviceConnected) {
        pCharacteristic->setValue(payload.c_str());
        pCharacteristic->notify();
      }
    }
    
    radio.startReceive(); // Se remet en écoute
  }
  
  delay(10); 
}
