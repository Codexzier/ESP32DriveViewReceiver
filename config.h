// ========================================================================================
//      Konfiguration, Pins und Datenstrukturen
// ========================================================================================
#pragma once

#include <Arduino.h>

// ==================================================
// WLAN Access Point
#define AP_SSID             "ESP32-FPV-Driver"
#define AP_PASSWORD         "Esp32FpsDriver"
#define AP_CHANNEL          6
#define AP_MAX_CLIENTS      2

// ==================================================
// Webserver Ports
#define HTTP_PORT           80      // Webseiten + API
#define STREAM_PORT         81      // MJPEG Videostream (eigener Server, blockiert die Steuerung nicht)

// ==================================================
// Ausgaenge (XIAO ESP32-S3: D0..D3, nicht I2C (D4/D5), nicht SD-Karte (D8..D10))
#define PIN_THROTTLE        D0      // GPIO1 - Vorwaerts/Rueckwaerts (Servo-PWM 50 Hz)
#define PIN_STEERING        D1      // GPIO2 - Links/Rechts          (Servo-PWM 50 Hz)
#define PIN_LIGHT           D2      // GPIO3 - Licht                 (Servo-PWM 50 Hz)
#define PIN_HORN            D3      // GPIO4 - Hupe                  (Rechteck fuer Piezo)

// LEDC Kanaele. Kanal 0 / Timer 0 benutzt die Kamera (XCLK).
// Timer-Zuordnung beim ESP32-S3: timer = (kanal / 2) % 4
#define LEDC_CH_THROTTLE    2       // Timer 1 (50 Hz)
#define LEDC_CH_STEERING    3       // Timer 1 (50 Hz)
#define LEDC_CH_LIGHT       4       // Timer 2 (50 Hz)
#define LEDC_CH_HORN        6       // Timer 3 (Piezo-Frequenz)

#define SERVO_FREQ_HZ       50
#define SERVO_RES_BITS      14      // 20 ms / 16384 = ca. 1.22 us Aufloesung
#define HORN_RES_BITS       10

// Failsafe: kommt laenger keine Steuerung vom Client, geht Gas auf Neutral
#define CONTROL_TIMEOUT_MS  600

// ==================================================
// SD-Karte (XIAO ESP32-S3 Sense Erweiterungsboard)
#define SD_CS_PIN           21
#define SD_CONFIG_FILE      "/fpv_config.txt"

// ==================================================
// Einstellungen (werden im NVS "EEPROM" bzw. auf der SD-Karte gespeichert)
struct ServoChannel {
  int16_t minUs;      // Pulsbreite bei -100 %
  int16_t centerUs;   // Pulsbreite bei 0 % (Trimmung / Offset)
  int16_t maxUs;      // Pulsbreite bei +100 %
  bool    invert;     // Richtung umkehren
};

struct Settings {
  ServoChannel throttle;
  ServoChannel steering;
  int16_t lightOffUs;     // Pulsbreite Licht aus
  int16_t lightOnUs;      // Pulsbreite Licht an
  uint16_t hornFreqHz;    // Tonfrequenz fuer Piezo
  uint8_t  hornDuty;      // Tastverhaeltnis in % (50 % = lautester Rechteckton)
  uint16_t hornDurationMs;// Dauer der Hupe
  uint8_t  frameSize;     // framesize_t der Kamera
  uint8_t  jpegQuality;   // 4 (beste) .. 63 (schlechteste)
};

// Laufzeitzustand der Steuerung
struct ControlState {
  float    throttle;      // -1.0 .. +1.0
  float    steering;      // -1.0 .. +1.0
  bool     light;
  uint32_t hornUntil;     // millis() bis wann die Hupe laeuft (0 = aus)
  uint32_t lastUpdate;    // millis() der letzten Steuernachricht
};
