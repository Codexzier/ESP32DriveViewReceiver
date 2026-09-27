// ========================================================================================
//      Meine Welt in meinem Kopf
// ========================================================================================
// Projekt:       ESP32 Drive View - Receiver
// Author:        Johannes P. Langner
// Controller:    XIAO ESP32-S3 Sense with cam
// Description:   FPV Fernsteuerung fuer ein Modellauto.
//                Der ESP32 startet einen eigenen WLAN Access Point und hostet eine
//                Webseite mit Videostream als Hintergrund und Steuerelementen
//                (Gas stufenlos, Lenkung, Licht, Hupe) sowie eine Setup-Seite.
//
//                WLAN:     SSID "ESP32-FPV-Driver", Passwort "Esp32FpsDriver"
//                Webseite: http://192.168.4.1/        (Steuerung)
//                          http://192.168.4.1/setup   (Einstellungen)
//
//                Ausgaenge:
//                  D0 (GPIO1) Vorwaerts/Rueckwaerts  Servo-PWM 50 Hz
//                  D1 (GPIO2) Lenkung links/rechts   Servo-PWM 50 Hz
//                  D2 (GPIO3) Licht                  Servo-PWM 50 Hz (Aus/An Pulsbreite)
//                  D3 (GPIO4) Hupe                   Rechteck fuer Piezo (Standard 2,7 kHz)
//
//                Arduino IDE: Board "XIAO_ESP32S3", PSRAM "OPI PSRAM",
//                             Partition Scheme "Huge APP" oder "Default with spiffs"
// Stand:         27.09.2026
// ========================================================================================

// ==================================================
// Camera
// Tip: enable PSRAM
#define CAMERA_MODEL_XIAO_ESP32S3

#include "esp_camera.h"
#include "camera_pins.h"

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "esp_http_server.h"

// Hinweis: alle Includes stehen hier in der Haupt-Datei, damit die von der
// Arduino IDE erzeugten Funktionsprototypen die Typen kennen.
#include "config.h"
#include "web_pages.h"

// ==================================================
// Access Point Adresse
IPAddress apIP(192, 168, 4, 1);
IPAddress apSubnet(255, 255, 255, 0);

// ==================================================
// globaler Zustand
Settings _settings;
ControlState _control = { 0, 0, false, 0, 0 };
const char *_settingsStorage = "Standard";

void setup() {

  Serial.begin(115200);
  delay(200);

  Serial.println("FPV Receiver Controller");

  // Einstellungen laden (NVS, sonst SD-Karte, sonst Standard)
  _settingsStorage = settingsLoad(_settings);
  Serial.printf("Einstellungen geladen aus: %s\n", _settingsStorage);

  // Ausgaenge sofort in Neutralstellung bringen
  outputsInit(_settings);
  outputsUpdate(_control, _settings);

  // init camera
  cameraInit(_settings);

  // WLAN Access Point starten
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, apSubnet);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CLIENTS)) {
    Serial.println("Access Point konnte nicht gestartet werden!");
  }
  WiFi.setSleep(false);   // geringere Latenz fuer Stream und Steuerung

  Serial.printf("Access Point: %s  Passwort: %s\n", AP_SSID, AP_PASSWORD);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  webServerStart();
}

void loop() {
  outputsUpdate(_control, _settings);
  delay(5);
}
