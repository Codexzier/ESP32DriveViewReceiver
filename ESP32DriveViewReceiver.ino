// ========================================================================================
//      Meine Welt in meinem Kopf
// ========================================================================================
// Projekt:       ESP32 Drive View - Receiver
// Author:        Johannes P. Langner
// Controller:    XIAO ESP32-S3 Sense with cam
// Actor:         TFT GC9A01, XY-Analog Stick
// Description:   
// Stand:         17.08.2026
// ========================================================================================

// ==================================================
// Camera
// Tip: enable PSRAM
#define CAMERA_MODEL_XIAO_ESP32S3

#include "esp_camera.h"
#include "camera_pins.h"

const char *ssid = "fpv_remotecontroller";
const char *password = "12345678";

long _lastMillis = 0;

#include <Arduino.h>
#include <WiFi.h>
// setup static ip address
IPAddress localIP(192, 168, 4, 2);      // wish IP of ESP32
IPAddress gateway(192, 168, 4, 1);      // Router-IP
IPAddress subnet(255, 255, 255, 0);     // Subnetzmaske
IPAddress dns(255, 255, 255, 0);        // Base dns address

// ==================================================
// Server
int _serverPort = 5001;                 // 
WiFiServer _server;


void setup() {
  
  Serial.begin(115200);

  Serial.println("FPV Receiver Controller");

  // init camera
  cameraInit();

  // setup WLAN
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(localIP, gateway, subnet, dns); // Statische IP setzen
  WiFi.setAutoReconnect(true);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi Status: "); Serial.println(WiFi.status());
    Serial.print("Hostname: "); Serial.println(WiFi.getHostname());
    Serial.print("Auto Reconnect: "); Serial.println(WiFi.getAutoReconnect());

    for(int i = 0; i < 40; i++) {
      Serial.print(".");
    }

    Serial.println("");

    delay(100);
  }

  Serial.println("WiFi connected! ");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // start server
  Serial.print("start server with port number ");
  char buffer[12];
  sprintf(buffer, "%d", _serverPort);
  Serial.println(buffer);

  _server.begin(_serverPort); 

  _lastMillis = millis();

  Serial.println("Server started! ");
}

void loop() {

  
  long actual = millis();
  if(actual < _lastMillis + 2000) {
     return;
   }
   _lastMillis = actual;

  Serial.println("------------------------------");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
  Serial.println("wait for client connecting");
  WiFiClient localclient = _server.accept();

  if(localclient) {

    Serial.println("connection accept");
    camera_fb_t *fb = NULL;

    while(localclient.connected()){
      //Serial.println("connected ");

      fb = esp_camera_fb_get();
      if(!fb){
        //Serial.println("");
        continue;
      }

      // send picture size
      uint32_t jpgSize = fb->len;
      uint8_t sizeBytes[4] = {
        (jpgSize >> 24) & 0xFF,  // MSB
        (jpgSize >> 16) & 0xFF,
        (jpgSize >> 8) & 0xFF,
        jpgSize & 0xFF          // LSB
      };

      localclient.write(sizeBytes, (size_t)4);

      // Send jpeg
      localclient.write(fb->buf, fb->len);

      //char buffer[12];
      //sprintf(buffer, "%d", fb->len);
      //Serial.print("jpg size "); Serial.println(buffer);

      esp_camera_fb_return(fb);
      fb = NULL;

      updateFPS();
      delay(2);
    }

    localclient.stop();
    //Serial.println("Connection break!");

    for(int i = 0; i < 4; i++) {
      delay(1000);
      Serial.print("Countdown: "); Serial.println(i, DEC);
    }
    Serial.println();
  }

  Serial.println("try in two second");  
}

void updateFPS() {
  static uint32_t lastCheckTime = 0; // Zeitpunkt der letzten Messung
  static uint32_t frameCount = 0;    // Zähler für die Frames
  static float currentFPS = 0.0;      // Gespeicherter FPS-Wert

  frameCount++; // Wird bei jedem Aufruf (jedes gesendete Bild) erhöht

  // Prüfen, ob 1 Sekunde (1000 ms) vergangen ist
  if (millis() - lastCheckTime >= 1000) {
    // FPS berechnen (für den Fall, dass das Intervall leicht abweicht)
    currentFPS = (float)frameCount * 1000.0 / (millis() - lastCheckTime);
    
    // FPS auf der seriellen Schnittstelle ausgeben
    Serial.printf("Gesendete Bilder/Sekunde (FPS): %.2f\n", currentFPS);

    // Zähler und Timer zurücksetzen
    frameCount = 0;
    lastCheckTime = millis();
  }
}