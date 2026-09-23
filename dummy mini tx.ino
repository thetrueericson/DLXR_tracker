#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------
// PARAMÈTRES DU MODULE (À modifier pour chaque ESP32)
// ---------------------------------------------------------
#define DEVICE_NAME "DLXR_RX_1" 
#define DRONE_ID    "fil_blanc" 

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// ---------------------------------------------------------
// SIMULATION DE DONNÉES (Centrée sur Air Fleury)
// ---------------------------------------------------------
float currentLat = 47.854408; // Point de départ (Piste principale)
float currentLon = 3.434616;
int currentAlt = 0;
int currentRssi = -40;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Démarrage du Serveur BLE...");

  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); 
  BLEDevice::startAdvertising();
  
  Serial.println("En attente de connexion client...");
}

void loop() {
  if (deviceConnected) {
    
    // Mouvement simulé depuis le centre de la piste
    currentLat += 0.00001;  
    currentLon += 0.00002; 
    currentAlt = (currentAlt < 120) ? currentAlt + 1 : 120; 
    currentRssi = random(-90, -40); 
    
    String etat = (currentAlt < 5) ? "GROUND" : "HIGH";

    JsonDocument doc;
    
    doc["drone_id"] = DRONE_ID;
    doc["etat"]     = etat;
    doc["alt"]      = currentAlt;
    doc["dist"]     = currentAlt * 2.5; 
    doc["rssi"]     = currentRssi;
    doc["lat"]      = currentLat;
    doc["lon"]      = currentLon;

    String jsonString;
    serializeJson(doc, jsonString);

    pCharacteristic->setValue(jsonString.c_str());
    pCharacteristic->notify(); 
    
    Serial.print("Trame envoyée : ");
    Serial.println(jsonString);

    delay(500); 
  }

  if (!deviceConnected && oldDeviceConnected) {
      delay(500);
      pServer->startAdvertising();
      Serial.println("Client déconnecté. Redémarrage de l'Advertising.");
      oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }
}
