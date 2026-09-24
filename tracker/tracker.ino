#include <RadioLib.h>
#include <TinyGPSPlus.h>
#include <SPI.h>

// --- CONFIGURATION GPS (Earhart - ESP32-C3) ---
#define GPS_RX_PIN 20 
#define GPS_TX_PIN 21 
#define GPS_BAUD 9600

// --- CONFIGURATION SPI & LORA ---
#define PIN_MISO 2
#define PIN_MOSI 3
#define PIN_SCK  4
#define PIN_NSS  5
#define DIO0     6  
#define NRST     7  

// On déclare un SX1276 (très probable pour un RFM95 DIY) au lieu d'un SX1262
SX1276 radio = new Module(PIN_NSS, DIO0, NRST, RADIOLIB_NC); 

TinyGPSPlus gps;
unsigned long lastTxTime = 0;
unsigned long lastDebugTime = 0;
const int txInterval = 10000;

void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("\n--- BOOT EARHART (ESP32-C3) ---");

  // 1. Initialisation GPS
  Serial0.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.printf("Ecoute GPS sur RX=%d a %d bauds\n", GPS_RX_PIN, GPS_BAUD);

  // 2. Initialisation du bus SPI
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_NSS);

  // 3. Initialisation LoRa (Paramètres adaptés au SX1276)
  int state = radio.begin(868.0, 125.0, 11, 7, 0x12, 10, 8, 0);
  if (state == RADIOLIB_ERR_NONE) {
    // Pas de setDio2AsRfSwitch ici, l'antenne est gérée physiquement !
    Serial.println("LoRa : OK (Puce SX1276 detectee !)");
  } else {
    Serial.printf("LoRa : ERREUR (Code %d)\n", state);
  }
}

void loop() {
  // Lecture du GPS
  while (Serial0.available() > 0) {
    gps.encode(Serial0.read());
  }

  // Debug Série : Bilan de santé GPS
  if (millis() - lastDebugTime > 1000) {
    Serial.printf("[DEBUG GPS] Sats: %d | Fix Valide: %s | Trames Decodees: %d\n", 
                  gps.satellites.value(), 
                  gps.location.isValid() ? "OUI" : "NON",
                  gps.sentencesWithFix());
    lastDebugTime = millis();
  }

  // Envoi LoRa toutes les 3 secondes
  if (millis() - lastTxTime > txInterval) {
    String payload = "{";
    payload += "\"drone_id\":\"Earhart\",";
    payload += "\"sats\":" + String(gps.satellites.value()) + ",";
    
    if (gps.location.isValid()) {
      payload += "\"etat\":\"VOL\",";
      payload += "\"lat\":" + String(gps.location.lat(), 6) + ",";
      payload += "\"lon\":" + String(gps.location.lng(), 6) + ",";
      payload += "\"alt\":" + String(gps.altitude.meters(), 1);
    } else {
      payload += "\"etat\":\"RECHERCHE\",";
      payload += "\"lat\":0,";
      payload += "\"lon\":0,";
      payload += "\"alt\":0";
    }
    payload += "}";

    Serial.print("[LORA TX] Envoi : ");
    Serial.print(payload);
    
    int state = radio.transmit(payload);
    
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(" -> SUCCES");
    } else {
      Serial.printf(" -> ECHEC (Erreur %d)\n", state);
    }
    
    lastTxTime = millis();
  }
}
