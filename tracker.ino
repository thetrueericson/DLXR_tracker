#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------
// PARAMÈTRES DU MODULE (À modifier pour chaque ESP32)
// ---------------------------------------------------------
// Le nom DOIT commencer par "DLXR_RX" pour être vu par l'interface Web
#define DEVICE_NAME "DLXR_RX_1" 
#define DRONE_ID    "fil_blanc" // Identifiant unique du drone affiché sur l'UI

// Les UUIDs doivent correspondre EXACTEMENT à ceux du code JavaScript
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// ---------------------------------------------------------
// SIMULATION DE DONNÉES (Pour tester l'interface)
// ---------------------------------------------------------
float currentLat = 48.8566; // Point de départ (Paris)
float currentLon = 2.3522;
int currentAlt = 0;
int currentRssi = -40;

// Callbacks pour gérer la connexion/déconnexion BLE
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

  // 1. Initialisation du périphérique BLE
  BLEDevice::init(DEVICE_NAME);

  // 2. Création du serveur
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 3. Création du Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 4. Création de la Caractéristique (Lecture + Notifications)
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  //  Indispensable pour que Web Bluetooth reçoive les notifications (StartNotifications)
  pCharacteristic->addDescriptor(new BLE2902());

  // 5. Démarrage du service et de l'annonce (Advertising)
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // Aide à la découverte sur iPhone/Mac
  BLEDevice::startAdvertising();
  
  Serial.println("En attente de connexion client...");
}

void loop() {
  // Si le téléphone/navigateur est connecté, on génère et envoie la trame
  if (deviceConnected) {
    
    // --- 1. SIMULATION DES MOUVEMENTS (À remplacer par la lecture de tes données réelles) ---
    currentLat += 0.0001;  // Le drone avance vers le Nord
    currentLon += 0.00005; // Le drone avance vers l'Est
    currentAlt = (currentAlt < 120) ? currentAlt + 1 : 120; // Monte jusqu'à 120m
    currentRssi = random(-90, -40); // Bruit radio simulé
    
    String etat = (currentAlt < 5) ? "GROUND" : "HIGH";

    // --- 2. CRÉATION DU JSON ---
    // Utilisation d'ArduinoJson 7
    JsonDocument doc;
    
    doc["drone_id"] = DRONE_ID;
    doc["etat"]     = etat;
    doc["alt"]      = currentAlt;
    doc["dist"]     = currentAlt * 2.5; // Distance simulée
    doc["rssi"]     = currentRssi;
    doc["lat"]      = currentLat;
    doc["lon"]      = currentLon;

    // Sérialisation du JSON dans une chaîne de caractères
    String jsonString;
    serializeJson(doc, jsonString);

    // --- 3. ENVOI VIA BLUETOOTH ---
    pCharacteristic->setValue(jsonString.c_str());
    pCharacteristic->notify(); // Déclenche l'événement côté JavaScript
    
    Serial.print("Trame envoyée : ");
    Serial.println(jsonString);

    // On envoie une trame toutes les 500ms (2 Hz)
    delay(500); 
  }

  // Gestion propre de la déconnexion (redémarre l'advertising pour pouvoir se reconnecter)
  if (!deviceConnected && oldDeviceConnected) {
      delay(500);
      pServer->startAdvertising();
      Serial.println("Client déconnecté. Redémarrage de l'Advertising.");
      oldDeviceConnected = deviceConnected;
  }
  
  // Gestion propre de la nouvelle connexion
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }
}
