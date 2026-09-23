#include <RadioLib.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>

// ---------------------------------------------------------
// 1. CÂBLAGE DE LA PUCE LORA (À VERIFIER SUR TON DIY)
// ---------------------------------------------------------
// Exemple de pins génériques pour un ESP32-C3 couplé à un SX1262
#define NSS  7
#define DIO1 3
#define NRST 10
#define BUSY 2

// ---------------------------------------------------------
// 2. CÂBLAGE DU MODULE GPS
// ---------------------------------------------------------
// Le fil TX du GPS va sur la broche RX de l'ESP32-C3
// Le fil RX du GPS va sur la broche TX de l'ESP32-C3
#define GPS_RX_PIN 20 
#define GPS_TX_PIN 21 
#define GPS_BAUD 9600 

SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // UART 1 matériel

unsigned long lastTransmission = 0;
const int INTERVALLE_ENVOI = 3000; // Envoi de la télémétrie toutes les 3s

void setup() {
  Serial.begin(115200);
  
  // Démarrage de la liaison série avec le GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  Serial.print("[TX] Initialisation LoRa... ");
  
  // Configuration LoRa (Europe 868MHz, identique au récepteur)
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 10, 8, 1.6, false);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK !");
    radio.setOutputPower(14); // Puissance d'émission
  } else {
    Serial.print("ERREUR LoRa, code: ");
    Serial.println(state);
    while (true); // Stoppe le programme si la puce LoRa n'est pas détectée
  }
}

void loop() {
  // 1. Lecture ininterrompue des données GPS entrantes
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // 2. Timer d'envoi LoRa (ne bloque pas la lecture du GPS)
  if (millis() - lastTransmission > INTERVALLE_ENVOI) {
    lastTransmission = millis();
    
    JsonDocument doc;
    doc["drone_id"] = "fil_blanc"; // Ton identifiant de drone
    
    // 3. Validation de la qualité du signal GPS
    if (gps.location.isValid() && gps.location.age() < 2000) {
      doc["etat"] = "HIGH";
      doc["lat"]  = gps.location.lat();
      doc["lon"]  = gps.location.lng();
      doc["alt"]  = gps.altitude.meters();
    } else {
      doc["etat"] = "BOOT"; // Cherche les satellites
      doc["lat"]  = 0.0;
      doc["lon"]  = 0.0;
      doc["alt"]  = 0.0;
    }
    
    doc["rssi"] = 0; 
    
    String payload;
    serializeJson(doc, payload);

    Serial.print("[TX] Envoi: ");
    Serial.println(payload);

    // 4. Transmission de la trame JSON
    radio.transmit(payload);
  }
}
