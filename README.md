# 🎯 DLXR Tracker V6.1 - MULTI

**DLXR Tracker V6.1** est une interface web de type Station Sol (GCS - Ground Control Station) ultra-légère conçue pour le suivi GPS en temps réel d'une **flotte d'aéromodèles** (avions RC, ailes volantes, drones). 

Elle fonctionne entièrement dans le navigateur web sans aucune installation grâce à l'API Web Bluetooth, et permet désormais de **se connecter simultanément à plusieurs modules relais au sol** (ex: ESP32 Heltec LoRa).

---

## 🚀 Démarrage rapide / Lancer l'interface

👉 **[[CLIQUER ICI POUR LANCER L'INTERFACE MULTI-DRONES](https://thetrueericson.github.io/DLXR_tracker/index_multi.html)]** 👈

> ⚠️ **Note technique importante :** L'API Web Bluetooth est une fonctionnalité de sécurité critique des navigateurs. Le fichier `index_multi.html` **doit obligatoirement être hébergé sur un serveur sécurisé (HTTPS)** ou lancé en réseau local (`localhost` ou `127.0.0.1`) pour que le bouton de connexion Bluetooth fonctionne. Il ne fonctionnera pas si vous l'ouvrez simplement avec un double-clic (protocole `file://`).

---

## ✨ Fonctionnalités

*   **Zéro Installation :** Fonctionne directement via un navigateur web (Chrome/Edge/Opera) grâce à la `Web Bluetooth API`.
*   **Multi-Flotte (Nouveauté V6) :** Connexion à plusieurs récepteurs BLE simultanément et suivi de plusieurs aéronefs sur la même carte.
*   **Cartographie Fluide & Intégrée :** Basée sur Leaflet et OpenStreetMap, avec un rendu visuel sombre optimisé pour l'extérieur. Algorithme d'anti-jittering intégré pour un recadrage de caméra (Throttling) sans saccades.
*   **Télémétrie en Temps Réel :** Affichage du RSSI, de l'altitude sol, de la distance relative et du nombre de trames reçues, le tout organisé dans un panneau défilant par aéronef.
*   **Sécurité & Watchdog (Nouveauté V6.1) :** 
    *   Détection automatique de perte de signal (passage de la balise en `OFFLINE` après 4 secondes d'inactivité).
    *   Nettoyage des identifiants (Sanitization) pour éviter toute faille d'injection XSS via les trames.
*   **Suivi de Vol :**
    *   Position de chaque modèle avec dessin des trajectoires colorées individuellement.
    *   Position du pilote (géolocalisation du smartphone/PC).
    *   Lignes de liaison dynamiques indiquant la distance et la direction à vol d'oiseau.
*   **Gestion des États de Vol :** Indicateurs dynamiques (`INIT`, `BOOT`, `LOW`, `HIGH`, `GROUND`, `OFFLINE`).

## 🏗️ Architecture du Système

Le système repose sur un flux de données (Point-à-Multipoints) :

1.  **Les Modèles (Airborne) :** Des microcontrôleurs couplés à des GPS transmettent leur télémétrie via des émetteurs **LoRa** longue portée.
2.  **Les Relais au Sol (Ground Stations) :** Des cartes type **Heltec ESP32 LoRa** captent les trames LoRa. Elles reformatent ces données en JSON et les exposent via un serveur **Bluetooth Low Energy (BLE)**.
    *   *Note matérielle :* Chaque ESP32 génère désormais son propre nom Bluetooth dynamiquement en fonction de son adresse MAC (ex: `DLXR_RX_A1B2`) pour éviter les conflits réseau.
3.  **L'Interface (Client Web) :** L'application HTML/JS scanne les périphériques commençant par `DLXR_RX`, se connecte, parse les JSON et met à jour la carte et l'interface de manière asynchrone pour chaque drone.

## 📡 Spécifications Bluetooth & Données

Le client web filtre les appareils dont le nom commence par **`DLXR_RX`** et écoute les notifications BLE sur les UUIDs suivants :
*   **Service UUID :** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
*   **Characteristic UUID :** `beb5483e-36e1-4688-b7f5-ea07361b26a8`

### Format JSON attendu (Mise à jour V6)
Pour que l'interface puisse gérer plusieurs aéronefs, **la clé `drone_id` (ou `id`) est désormais requise** dans la trame JSON.

*Exemple de trame de télémétrie classique (Multi-émetteur) :*
```json
{
  "drone_id": "EZ-glider",
  "etat": "HIGH",
  "alt": 120.5,
  "dist": 850.2,
  "rssi": -80,
  "lat": 47.854298,
  "lon": 3.434910
}
