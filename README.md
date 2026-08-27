# 🎯 DLXR Tracker V5

**DLXR Tracker V5** est une interface web de type Station Sol (GCS - Ground Control Station) ultra-légère conçue pour le suivi GPS en temps réel d'aéromodèles (avions RC, ailes volantes, drones). 

Elle fonctionne entièrement dans le navigateur web sans aucune installation grâce à l'API Web Bluetooth, en se connectant à un module relais au sol (ex: ESP32 Heltec LoRa).

## ✨ Fonctionnalités

*   **Zéro Installation :** Fonctionne directement via un navigateur web grâce à la `Web Bluetooth API`.
*   **Cartographie Intégrée :** Basée sur Leaflet et OpenStreetMap, avec un rendu visuel sombre (tactique) optimisé pour la lisibilité en extérieur.
*   **Télémétrie en Temps Réel :** Affichage du RSSI, de l'altitude sol, de la distance relative pilote/modèle et du nombre de trames reçues.
*   **Suivi de Vol :**
    *   Position du modèle avec dessin de la trajectoire (trace).
    *   Position du pilote (géolocalisation du smartphone/PC).
    *   Ligne de liaison dynamique indiquant la distance et la direction à vol d'oiseau.
*   **Gestion des États de Vol :** Indicateurs dynamiques (`BOOT`, `LOW`, `HIGH`, `GROUND`) pour identifier rapidement la phase de vol ou la position d'atterrissage.
*   **Conformité :** Réception et affichage de l'identifiant balise DGAC (Alpha Tango).

## 🏗️ Architecture du Système

Le système repose sur un flux de données unidirectionnel (Point-à-Point) :

1.  **Le Modèle (Airborne) :** Un microcontrôleur couplé à un GPS transmet sa télémétrie via un émetteur **LoRa** longue portée.
2.  **Le Relais au Sol (Ground Station) :** Une carte type **Heltec Automation ESP32 LoRa** capte les trames LoRa. Elle reformate ces données en JSON et les expose via un serveur **Bluetooth Low Energy (BLE)** (nom de l'appareil : `DLXR_RX`).
3.  **L'Interface (Client Web) :** L'application HTML/JS se connecte au BLE, parse le JSON et met à jour la carte et l'interface de manière asynchrone.

## 📡 Spécifications Bluetooth & Données

Le client web écoute les notifications BLE sur les UUIDs suivants :
*   **Service UUID :** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
*   **Characteristic UUID :** `beb5483e-36e1-4688-b7f5-ea07361b26a8`

### Format JSON attendu
L'interface s'attend à recevoir une chaîne de caractères (String) depuis le BLE, parsable en objet JSON. 

*Exemple de trame de télémétrie classique :*
```json
{
  "etat": "HIGH",
  "alt": 120.5,
  "dist": 850.2,
  "rssi": -85,
  "lat": 47.854298,
  "lon": 3.434910,
  "moved": true
}
