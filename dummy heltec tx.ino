#include <RadioLib.h>
#include <ArduinoJson.h>

// Broches internes spécifiques au Heltec LoRa 32 V3 (ESP32-S3 + SX1262)
#define NSS 8
#define DIO1 14
#define NRST 12
#define BUSY 13

// Initialisation du module radio
SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);

void setup() {
  Serial.begin(115200);
  delay(2000); 

  Serial.print("[TX] Initialisation LoRa... ");
  
  // Configuration : 868.0 MHz (Europe), Bande passante 125 kHz, Spreading Factor 9, Coding Rate 4/7
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 10, 8, 1.6, false);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK !");
    // Puissance d'émission (en dBm). 14 est un bon compromis test. (Max 22 sur SX1262)
    radio.setOutputPower(14); 
  } else {
    Serial.print("ERREUR, code ");
    Serial.println(state);
    while (true); // Arrêt si la puce radio ne répond pas
  }
}

void loop() {
  // 1. Création du document JSON
  JsonDocument doc;
  
  // 2. Remplissage avec des données de test autour de Fleury
  doc["drone_id"] = "DLXR_TX_1";
  doc["etat"]     = "HIGH";
  doc["alt"]      = random(10, 150); 
  doc["lat"]      = 47.854408 + (random(-200, 200) / 100000.0);
  doc["lon"]      = 3.434616 + (random(-200, 200) / 100000.0);
  doc["rssi"]     = 0; // Sera rempli par le RX à la réception

  // 3. Sérialisation en chaîne de caractères
  String payload;
  serializeJson(doc, payload);

  Serial.print("[TX] Envoi: ");
  Serial.println(payload);

  // 4. Transmission LoRa
  int state = radio.transmit(payload);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(" -> Succès");
  } else {
    Serial.println(" -> Échec");
  }

  delay(3000); // Émet une trame toutes les 3 secondes
}
