# Personal Development Log

**Name:** Viktor Medvedev
**Project:** Robert Cyro

---

## YYYY-MM-DD
## 2026-11-05

Pris en main l'ESP32 avec PlatformIO sous VS Code (configuré la carte esp32dev et le framework Arduino).

Réglé la connexion USB sur Ubuntu (droits d'accès sur /dev/ttyUSB0 via dialout/chmod + mis le Serial Monitor à 115200 bauds dans platformio.ini pour avoir des logs lisibles).

Testé le capteur tactile capacitif intégré à la carte sur la broche P4 / GPIO 4 (touchRead(T0)), ce qui permet de détecter un appui du doigt directement sur la pin sans bouton externe.

Affiché les valeurs brutes dans le Serial Monitor pour calibrer le seuil: à l'air libre ça tourne autour de 1500 et quand on pose le doigt ça descend autour de 100–300 (donc seuil fixé à < 600).

Programmé la LED bleue intégrée (GPIO 2) en mode interrupteur (toggle): à chaque appui sur P4, la LED change d'état (allumée/éteinte) avec digitalWrite.

Pendant le rdv, commencé de connecter et programmer les Servos avec le tutoriel

lien pour le tutoriel:
https://randomnerdtutorials.com/esp32-servo-motor-web-server-arduino-ide/
---

## 20206-11-12

Adapté le code pour piloter les 4 servomoteurs en même temps via un tableau sur les broches 13, 14, 27, 26 (remplacé la 12 car c'est une broche de boot sur l'ESP32 qui peut empêcher la carte de démarrer si un servo y est branché).

Configuré le PWM pour les moteurs MG90S (50 Hz, impulsions entre 500 et 2400 µs) avec un timer matériel dédié par servo.

Suivi le tuto du serveur web et adapté la page HTML pour avoir 4 sliders indépendants (0deg à 180deg), un par articulation (Base, Shoulder, Elbow, Wrist).

Remplacé les appels jQuery du tuto par fetch() en JavaScript natif pour envoyer les requêtes /?servo=X&value=Y& en arrière-plan sans recharger la page (et sans avoir besoin d'Internet), tout en gardant les positions affichées si on rafraîchit la page.

Passé l'ESP32 en mode Point d'Accès (WiFi.softAP, réseau RobertCyro-ESP32, IP 192.168.4.1) pour pouvoir s'y connecter directement depuis un téléphone ou un PC sans routeur.

Testé depuis un portable - les 4 sliders envoient bien les bonnes valeurs d'angle pour chaque broche dans le Serial Monitor.

---
