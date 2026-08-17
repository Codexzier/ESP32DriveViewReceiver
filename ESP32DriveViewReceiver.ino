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

#include <Arduino.h>

// ==================================================
// Camera
// TIP: need an enabled PSRAM
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#include "esp_camera.h"
#include "camera_pins.h"

// ========================================================================================
// Access Point and network server
#include <WiFi.h>
#include <NetworkClient.h>
#include <WiFiAP.h>

const char *ssid = "fpv_remotecontroller";
const char *password = "12345678";

const char* serverIP = "192.168.4.2";  // IP des ESP32-Servers
const uint16_t serverPort = 8080;         // Port des Servers
WiFiClient _client;

long _timeoutForReconnect = 30;

// ========================================================================================
// any


long _lastMillis = 0;


void setup() {
  
  Serial.begin(115200);

  Serial.println("FPV Receiver Controller");

  // ----------------------------------------------
  // init camera
  cameraInit();

  // ----------------------------------------------
  // setup access point
  if (!WiFi.softAP(ssid, password)) {
    log_e("Soft AP creation failed.");
    Serial.println("Soft AP creation failed.");
    while (1);
  }

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // ----------------------------------------------
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
