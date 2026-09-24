#include <RadioLib.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- PINS LORA (Heltec V4 - SX1262) ---
#define NSS 8
#define DIO1 14
#define NRST 12
#define BUSY 13
SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);

// --- PINS OLED (Heltec V4) ---
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define VEXT_PIN 36
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

// --- PINS GPS ---
#define GPS_RX_PIN 47 // Fil TX du GPS à brancher ici
#define GPS_TX_PIN 48 // Fil RX du GPS à brancher ici
#define GPS_BAUD 9600 

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

unsigned long lastTransmission = 0;
const int INTERVALLE_ENVOI = 3000;

void updateOLED(int sats, float lat, float lon, String etat) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  
  u8g2.drawStr(0, 12, "TX : Chupito");
  u8g2.drawLine(0, 16, 128, 16);

  u8g2.setCursor(0, 32);
  u8g2.print("Sats: "); 
  u8g2.print(sats);
  u8g2.print(" ["); 
  u8g2.print(etat); 
  u8g2.print("]");

  u8g2.setCursor(0, 48);
  if (lat != 0.0) {
    u8g2.print(lat, 5); 
    u8g2.print(","); 
    u8g2.print(lon, 5);
  } else {
    u8g2.print("Recherche GPS...");
  }

  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  
  // Allumage de l'écran avec luminosité max
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(50);
  
  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 30, "Init TX LoRa...");
  u8g2.sendBuffer();

  // Initialisation du port série matériel pour le GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // Initialisation LoRa
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 10, 8, 1.6, false);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[TX] LoRa OK !");
    radio.setOutputPower(14); // Poussable jusqu'à 22 dBm sur le SX1262
    updateOLED(0, 0.0, 0.0, "BOOT");
  } else {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 30, "ERREUR LORA !");
    u8g2.sendBuffer();
    while (true);
  }
}

void loop() {
  // Lecture continue du GPS
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Envoi périodique
  if (millis() - lastTransmission > INTERVALLE_ENVOI) {
    lastTransmission = millis();
    
    JsonDocument doc;
    doc["drone_id"] = "Chupito"; 
    
    int sats = gps.satellites.value();
    doc["sats"] = sats;
    
    String etat = "BOOT";
    float lat = 0.0, lon = 0.0, alt = 0.0;

    // Validation du point GPS
    if (gps.location.isValid() && gps.location.age() < 2000) {
      etat = "HIGH";
      lat = gps.location.lat();
      lon = gps.location.lng();
      alt = gps.altitude.meters();
    } 
    
    doc["etat"] = etat;
    doc["lat"]  = lat;
    doc["lon"]  = lon;
    doc["alt"]  = alt;
    doc["rssi"] = 0; 
    
    String payload;
    serializeJson(doc, payload);

    Serial.print("[TX] Envoi: ");
    Serial.println(payload);

    radio.transmit(payload);
    
    // Rafraîchissement de l'écran
    updateOLED(sats, lat, lon, etat);
  }
}
