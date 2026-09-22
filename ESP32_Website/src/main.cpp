#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> // used to include the html,css,js webpage code
#include <string>
#include "cred.h" // Sets access point credentials (ignored by Git)

// Flag for motor (volatile as can be changed from webpage)
volatile bool openDoor = false;

// Used to track and display status
String doorStatus = "Closed";

// Motor pin
const int MOTOR_PIN = 2;

// Create the AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_PIN, OUTPUT);
  
  // check if little fs had an error
  if (!LittleFS.begin(true)) {
    Serial.println("Error occurred while mounting LittleFS");
    return;
  }

  // Start the Access Point
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // HTTP request when a client requests the root IP URL ("/")
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // Sends the HTML response asynchronously without blocking code execution
    request->send_P(200, "/index.html", "text_html");
  });

  // HTTP request from webpage to open door
  server.on("/open-door", HTTP_GET, [](AsyncWebServerRequest *request){
    if (doorStatus == "Closed") {
      openDoor = true;
      doorStatus = "Opening...";
    }
    request->send(200, "text/plain", "Trigger received");
  });

  // Poll for door status to update webpage
  server.on("/get-status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", doorStatus);
  });

  // Start the background server
  server.begin();
  Serial.println("Async Server started.");
}

void loop() {
  // Open door 
  if (openDoor) {
    openDoor = false;

    // TODO: add code for opening motor (depends on which motor)
    Serial.println("OPENING DOOR");
    delay(1000);
  } 
}