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

// ==================================================
// WiFi
#include <WiFi.h>

const char *ssid = "fpv_remotecontroller";
const char *password = "12345678";

// setup static ip adress
IPAddress localIP(192, 168, 4, 2);      // target IP adress for the ESP32
IPAddress gateway(192, 168, 4, 1);      // Router-IP, but it dosen't matter
IPAddress subnet(255, 255, 255, 0);     // Subnetzmaske
IPAddress dns(255, 255, 255, 0);        // base setup 

const char* _serverIP = "192.168.4.1";  // IP des ESP32-Servers
const uint16_t _serverPort = 8080;         // Port des Servers
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
  // setup WLAN
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //WiFi.config(localIP, gateway, subnet, dns); // Statische IP setzen
  //WiFi.setAutoReconnect(true);

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

  connectToServer();
  Serial.println("New Client.");  // print a message out the serial port

    _lastMillis = millis();
}

void loop() {
  
  long actual = millis();
  if(actual < _lastMillis + 2000) {
     return;
   }
   _lastMillis = actual;
   connectToServer();

  if(_client) {

    Serial.println("connection accept");

    while(_client.connected()){
      Serial.println("connected ");

      camera_fb_t *fb = esp_camera_fb_get();
      if(fb){
        Serial.println("");
      }
      _client.write(fb->buf, fb->len);
      esp_camera_fb_return(fb);

      delay(10);
    }

    _client.stop();
    Serial.println("Connection break!");

    for(int i = 0; i < 4; i++) {
      delay(1000);
      Serial.print("Countdown: "); Serial.println(i, DEC);
    }
    Serial.println();
  }

  Serial.println("try in two second");  
}

void connectToServer() {
  Serial.print("connect to server with port number ");
  char buffer[12];
  sprintf(buffer, "%d", _serverPort);
  Serial.println(buffer);

  while(!_client.connect(_serverIP, _serverPort)){

    Serial.println("No connection with a client!");
    delay(1000);
    Serial.print("Connecto to IP "); Serial.println(_serverIP);
  }

Serial.print("IP: "); Serial.println(WiFi.localIP());

}