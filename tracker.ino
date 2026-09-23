#include <RadioLib.h>
#include <SPI.h>

// ---------------------------------------------------------
// 1. CÂBLAGE LORA (À CORRIGER AVEC TES VRAIES BROCHES C3)
// ---------------------------------------------------------
#define PIN_MISO 5  // Remplacer par ta vraie broche
#define PIN_MOSI 6  // Remplacer par ta vraie broche
#define PIN_SCK  4  // Remplacer par ta vraie broche

#define PIN_NSS  21 // GPIO 21 (D'après ton message)
#define DIO0     9  // GPIO 9
#define NRST     18 // GPIO 18

// On déclare bien un module SX1276 ! Le dernier paramètre est non-connecté (RADIOLIB_NC)
SX1276 radio = new Module(PIN_NSS, DIO0, NRST, RADIOLIB_NC);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.print("[TX] Initialisation SPI & LoRa... ");

  // Initialisation du bus SPI spécifique pour l'ESP32-C3
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_NSS);
  
  // Initialisation du SX1276 (Fréquence, Bande passante, SF, CR, SyncWord, Preamble)
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
  String payload = "{\"drone_id\":\"fil_blanc\",\"etat\":\"HIGH\",\"lat\":47.8544,\"lon\":3.4346,\"alt\":100}";
  
  Serial.print("[TX] Envoi: ");
  Serial.println(payload);

  int state = radio.transmit(payload);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(" -> Succès");
  } else {
    Serial.println(" -> Échec");
  }

  delay(3000);
}
