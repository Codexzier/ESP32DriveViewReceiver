// ========================================================================================
//      Meine Welt in meinem Kopf
// ========================================================================================
// Projekt:       ESP32 Drive View - Remote Controller
// Author:        Johannes P. Langner
// Controller:    XIAO ESP32-S3
// Actor:         TFT GC9A01, XY-Analog Stick
// Description:   
// Stand:         17.08.2026
// ========================================================================================

// ESP32-S3 Sense with camera

// ==================================================
// Camera
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#include "esp_camera.h"
#include "camera_pins.h"


const char *ssid = "fpv_remotecontroller";
const char *password = "12345678";

long _lastMillis = 0;

#include <Arduino.h>
#include <WiFi.h>
// Festgelegte IP-Konfiguration
IPAddress localIP(192, 168, 4, 2);    // Gewünschte IP des ESP32
IPAddress gateway(192, 168, 4, 1);      // Router-IP
IPAddress subnet(255, 255, 255, 0);     // Subnetzmaske
IPAddress dns(255, 255, 255, 0);              // DNS (z. B. Google DNS)

int _serverPort = 8080;  
WiFiServer _server;


void setup() {
  
  // activ signal LED
  //pinMode(LED_BUILTIN, OUTPUT);

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

    while(localclient.connected()){
      Serial.println("connected ");

      //digitalWrite(LED_BUILTIN, true);

      camera_fb_t *fb = esp_camera_fb_get();
      if(fb){
        Serial.println("");
      }
      localclient.write(fb->buf, fb->len);
      esp_camera_fb_return(fb);
      //digitalWrite(LED_BUILTIN, false);

      delay(10);
    }

    localclient.stop();
    Serial.println("Connection break!");

    for(int i = 0; i < 4; i++) {
      //digitalWrite(LED_BUILTIN, true);
      delay(500);
      //digitalWrite(LED_BUILTIN, false);
      delay(500);
      Serial.print("Countdown: "); Serial.println(i, DEC);
    }
    Serial.println();
  }
  // else {
  //   Serial.println("try in two second");
  //   _server.stop();
  //   delay(2000);
  //   _server.setTimeout(3);
  //   _server.begin(_serverPort);
  // }

  Serial.println("try in two second");  
}

// void sendNextPicture() {

//   // long actual = millis();
//   // if(actual < _lastMillis + 10) {
//   //   return;
//   // }
//   // _lastMillis = actual;

//   digitalWrite(LED_BUILTIN, true);

//   camera_fb_t *fb = esp_camera_fb_get();
//   if(fb){
//     Serial.println("");
//   }
//   _client.write(fb->buf, fb->len);

//   digitalWrite(LED_BUILTIN, false);
// }
