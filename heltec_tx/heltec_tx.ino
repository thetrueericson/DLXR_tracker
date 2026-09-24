#include <RadioLib.h>
#include <TinyGPSPlus.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- CONFIGURATION GPS (Heltec V4 - Bernadette) ---
#define VGNSS_CTRL 34
#define GPS_RX_PIN 39
#define GPS_TX_PIN 38
#define GPS_BAUD 9600

// --- CONFIGURATION LORA (Heltec V3/V4) ---
#define NSS 8
#define DIO1 14
#define NRST 12
#define BUSY 13
SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);

// --- CONFIGURATION OLED ---
#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define VEXT_PIN 36 
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

TinyGPSPlus gps;
unsigned long lastTxTime = 0;
const int txInterval = 3000; // Envoi toutes les 3 secondes

void setup() {
  Serial.begin(115200);

  // 1. Allumage de l'écran OLED
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(50);
  u8g2.begin();
  u8g2.setContrast(255);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
<<<<<<< HEAD
  u8g2.drawStr(0, 15, "Boot Katarina...");
=======
  u8g2.drawStr(0, 15, "Boot Bernadette...");
>>>>>>> 0fd28b4278c68cf879967076a5c1bf916092695a
  u8g2.sendBuffer();

  // 2. Allumage et initialisation du GPS matériel
  pinMode(VGNSS_CTRL, OUTPUT);
  digitalWrite(VGNSS_CTRL, LOW); // Activation de l'alimentation du GPS
  delay(500);
  Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // 3. Initialisation LoRa
  int state = radio.begin(868.0, 125.0, 9, 7, 0x12, 10, 8, 1.6, false);
  if (state == RADIOLIB_ERR_NONE) {
    u8g2.drawStr(0, 30, "LoRa OK");
  } else {
    u8g2.drawStr(0, 30, "Erreur LoRa !");
  }
  u8g2.sendBuffer();
  delay(1000);
}

void loop() {
  // Lecture continue du port série matériel connecté au GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }

  // Envoi LoRa toutes les 3 secondes
  if (millis() - lastTxTime > txInterval) {
    String payload = "{";
<<<<<<< HEAD
    payload += "\"drone_id\":\"Katarina\",";
=======
    payload += "\"drone_id\":\"Bernadette\",";
>>>>>>> 0fd28b4278c68cf879967076a5c1bf916092695a
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

    // Transmission LoRa
    radio.transmit(payload);
    lastTxTime = millis();

    // Mise à jour de l'écran OLED
    u8g2.clearBuffer();
    u8g2.setCursor(0, 15);
<<<<<<< HEAD
    u8g2.print("ID: Katarina");
=======
    u8g2.print("ID: Bernadette");
>>>>>>> 0fd28b4278c68cf879967076a5c1bf916092695a
    
    u8g2.setCursor(0, 30);
    u8g2.print("Sats: ");
    u8g2.print(gps.satellites.value());

    if (gps.location.isValid()) {
      u8g2.setCursor(0, 45);
      u8g2.print(gps.location.lat(), 4);
      u8g2.print(", ");
      u8g2.print(gps.location.lng(), 4);
    } else {
      u8g2.setCursor(0, 45);
      u8g2.print("Recherche GPS...");
    }
    u8g2.sendBuffer();

    Serial.print("Envoi LoRa : ");
    Serial.println(payload);
  }
}
