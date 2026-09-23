#include <RadioLib.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>

// --- PINS LORA (Heltec V3 / V4.3) ---
#define NSS 8
#define DIO1 14
#define NRST 12
#define BUSY 13

// --- PINS ET CONFIG GPS ---
// Modifie ces valeurs selon ton câblage exact
// Rappel : Le TX du GPS va sur le RX de l'ESP32, et le RX du GPS sur le TX de l'ESP32.
#define GPS_RX_PIN 47 
#define GPS_TX_PIN 48 
#define GPS_BAUD 9600 // La majorité des modules GPS (Beitian, BN-220, etc.) communiquent à 9600 bauds par défaut

SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // Utilisation de l'UART matériel 1 de l'ESP32

unsigned long lastTransmission = 0;
const int INTERVALLE_ENVOI = 3000; // Envoi toutes les 3 secondes

void setup() {
  Serial.begin(115200);
  
  // Initialisation du port série pour le GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  Serial.print("[TX] Initialisation LoRa... ");
  
  // Config: 868.0 MHz, BW 125 kHz, SF 9, CR 4/7
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 10, 8, 1.6, false);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK !");
    radio.setOutputPower(14); // Puissance d'émission (Max 22)
  } else {
    Serial.print("ERREUR, code ");
    Serial.println(state);
    while (true);
  }
}

void loop() {
  // 1. Lecture continue des données GPS entrantes
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // 2. Timer d'envoi non-bloquant
  if (millis() - lastTransmission > INTERVALLE_ENVOI) {
    lastTransmission = millis();
    
    JsonDocument doc;
    
    // Identifiant unique du Tracker
    doc["drone_id"] = "DLXR_TRACKER_1";
    
    // 3. Vérification du Fix GPS
    if (gps.location.isValid() && gps.location.age() < 2000) {
      // Le GPS a accroché les satellites et la donnée a moins de 2 secondes
      doc["etat"] = "HIGH"; // Indique un vol normal ou un bon fix
      doc["lat"]  = gps.location.lat();
      doc["lon"]  = gps.location.lng();
      doc["alt"]  = gps.altitude.meters();
      
      // Bonus: on peut utiliser gps.speed.kmph() ou gps.satellites.value() si besoin plus tard
    } else {
      // Pas de fix ou données trop vieilles
      doc["etat"] = "BOOT"; // Indique que le module cherche les satellites
      doc["lat"]  = 0.0;
      doc["lon"]  = 0.0;
      doc["alt"]  = 0.0;
    }
    
    doc["rssi"] = 0; // Réservé pour le RX
    
    String payload;
    serializeJson(doc, payload);

    Serial.print("[TX] Envoi: ");
    Serial.println(payload);

    // 4. Transmission
    int state = radio.transmit(payload);

    if (state != RADIOLIB_ERR_NONE) {
      Serial.print("[TX] Erreur transmission: ");
      Serial.println(state);
    }
  }
}
