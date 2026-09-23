#include <RadioLib.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>

// ---------------------------------------------------------
// 1. CÂBLAGE LORA SX1276 SUR ESP32-C3
// ---------------------------------------------------------
#define PIN_MISO 2
#define PIN_MOSI 3
#define PIN_SCK  4

#define PIN_NSS  5  // Chip Select
#define DIO0     6  // Interruption
#define NRST     7  // Reset LoRa

// ---------------------------------------------------------
// 2. CÂBLAGE GPS
// ---------------------------------------------------------
#define GPS_RX_PIN 21 // Fil TX du GPS branché ici
#define GPS_TX_PIN 20 // Fil RX du GPS (s'il est branché, sinon peu importe)
#define GPS_BAUD 9600 

SX1276 radio = new Module(PIN_NSS, DIO0, NRST, RADIOLIB_NC);
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); 

unsigned long lastTransmission = 0;
const int INTERVALLE_ENVOI = 3000; 

void setup() {
  Serial.begin(115200);
  
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  delay(2000); 
  Serial.print("[TX] Initialisation SPI & LoRa... ");

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_NSS);
  
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 10, 8, 0);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("OK !");
    radio.setOutputPower(14); 
  } else {
    Serial.print("ERREUR, code: ");
    Serial.println(state);
    while (true);
  }
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (millis() - lastTransmission > INTERVALLE_ENVOI) {
    lastTransmission = millis();
    
    JsonDocument doc;
    doc["drone_id"] = "fil_blanc";
    doc["sats"] = gps.satellites.value();
    
    if (gps.location.isValid() && gps.location.age() < 2000) {
      doc["etat"] = "HIGH";
      doc["lat"]  = gps.location.lat();
      doc["lon"]  = gps.location.lng();
      doc["alt"]  = gps.altitude.meters();
    } else {
      doc["etat"] = "BOOT";
      doc["lat"]  = 0.0;
      doc["lon"]  = 0.0;
      doc["alt"]  = 0.0;
    }
    
    doc["rssi"] = 0; 
    
    String payload;
    serializeJson(doc, payload);

    Serial.print("[TX] Envoi: ");
    Serial.println(payload);

    radio.transmit(payload);
  }
}
