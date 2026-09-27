# ESP32 Drive View – FPV-Fernsteuerung für ein Modellauto

Ein XIAO ESP32-S3 Sense startet einen eigenen WLAN Access Point und stellt eine Webseite bereit.
Auf der Seite läuft das Kamerabild als Hintergrund, darüber liegen die Steuerelemente.

## Verbindung

| | |
|---|---|
| WLAN (SSID) | `ESP32-FPV-Driver` |
| Passwort | `Esp32FpsDriver` |
| Steuerung | http://192.168.4.1/ |
| Setup | http://192.168.4.1/setup |
| Videostream | http://192.168.4.1:81/stream |

## Steuerseite (Smartphone im Querformat)

- **Links oben:** Setup (daneben ⛶: Bild füllen/einpassen umschalten)
- **Rechts oben:** Licht An/Aus, Hupe (ertönt für 0,5 s)
- **Links unten:** Vorwärts/Rückwärts – stufenloser Schieberegler, federt beim Loslassen auf Neutral zurück
- **Rechts unten:** Lenkung – stufenloser Schieberegler, federt auf die Mitte zurück
- Beide Regler lassen sich gleichzeitig bedienen (Multitouch).
- **Failsafe:** Kommt 600 ms lang kein Steuerbefehl an (WLAN weg, Seite geschlossen), gehen Gas und Lenkung auf Neutral.

## Ausgänge

| Pin | GPIO | Funktion | Signal |
|---|---|---|---|
| D0 | 1 | Vorwärts/Rückwärts | Servo-PWM 50 Hz, 1000–2000 µs |
| D1 | 2 | Lenkung | Servo-PWM 50 Hz, 1000–2000 µs |
| D2 | 3 | Licht | Servo-PWM 50 Hz, Aus 1000 µs / An 2000 µs |
| D3 | 4 | Hupe | Rechteck 2,7 kHz, 50 % (für Piezo-Summer) |

D4/D5 (I²C) und D8–D10 (SD-Karte) bleiben frei. Die Kamera belegt LEDC-Kanal/Timer 0,
die Ausgänge nutzen die Kanäle 2–6 (Timer 1–3).

> Hinweis: Ein *passiver* Piezo braucht das Rechtecksignal (Frequenz einstellbar). Ein *aktiver* Summer
> erzeugt den Ton selbst – dafür im Setup das Tastverhältnis auf 99 % stellen.

## Setup-Seite

- Gas und Lenkung: Min, Mitte (Offset/Trimmung), Max in µs, Richtung umkehren
- Licht: Pulsbreite für Aus und An
- Hupe: Frequenz, Tastverhältnis, Dauer
- Kamera: Auflösung per ComboBox. Nach einer Änderung wird der Videostream 5 Sekunden lang gemessen,
  das FPS-Ergebnis steht rechts neben der ComboBox. Zusätzlich JPEG-Qualität.
- Änderungen wirken sofort (zum Testen); **Speichern** legt sie dauerhaft ab.

Gespeichert wird im NVS-Flash (Preferences – beim ESP32 der Ersatz für ein EEPROM).
Schlägt das fehl, wird `/fpv_config.txt` auf die SD-Karte geschrieben. Beim Start wird zuerst
NVS, dann die SD-Karte gelesen, sonst gelten die Standardwerte.

## Arduino IDE

- Board-Paket „esp32“ von Espressif (getestet mit 2.0.9 und 3.3.1)
- Board: **XIAO_ESP32S3**
- PSRAM: **OPI PSRAM** (wird für höhere Auflösungen benötigt)
- Keine zusätzlichen Bibliotheken nötig

## Dateien

| Datei | Inhalt |
|---|---|
| `ESP32DriveViewReceiver.ino` | Setup/Loop, Access Point |
| `config.h` | Pins, WLAN-Daten, Datenstrukturen |
| `CameraManager.ino` | Kamera-Initialisierung, Auflösung/Qualität |
| `OutputManager.ino` | PWM-Ausgänge, Failsafe, Hupe |
| `SettingsManager.ino` | Laden/Speichern (NVS, SD-Karte) |
| `WebServer.ino` | HTTP-Server, API, MJPEG-Stream |
| `web_pages.h` | HTML/JS der Steuer- und Setup-Seite |
