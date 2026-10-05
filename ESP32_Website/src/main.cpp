#include <Arduino.h>
#include <WiFi.h>
// #include <AccelStepper.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncDNSServer.h>
#include <LittleFS.h> // used to include the html,css,js webpage code
#include <string>
#include "cred.h" // Sets access point credentials (ignored by Git)

// Motor pins
constexpr int stepPin = 26;
constexpr int dirPin = 27;
constexpr int enPin = 25; // A4988 ENABLE pin is active LOW (LOW = driver enabled)

// Flag for motor (volatile as can be changed from async webpage)
volatile bool openDoor = false;

// How long to hold the latch open before closing, in milliseconds.
constexpr unsigned long openTime = 3000;

// Timestamp (millis()) of when the door entered the Open state.
unsigned long openStartTime = 0;

// How far the motor needs to travel to fully open the door, in steps.
// 200 steps/rev × 5 (gear ratio) = 1000 steps
constexpr int stepsToOpen = 1000;

// Create AsyncDNSServer object on port 53 
// Used for implementing a captive portal to redirect all traffic to this server's IP
AsyncDNSServer dnsServer;
constexpr byte dnsPort = 53;
constexpr int dnsTTL = 3600; // refresh cache every hour
String softAPIPStr; // store our ip as string for redirects

// Create the AsyncWebServer object on port 80
// Can handle HTTP requests despite blocking motor controls
AsyncWebServer server(80);

// Motor state machine
enum class MotorState {
  Closed,
  Opening,
  Open,
  Closing
};

MotorState state = MotorState::Closed;

// Human-readable status derived from `state`, used for the webpage poll
const char* statusText() {
  switch (state) {
    case MotorState::Closed:  return "Closed";
    case MotorState::Opening: return "Opening...";
    case MotorState::Open:    return "Open";
    case MotorState::Closing: return "Closing...";
  }
  return "Unknown";
}

void setup() {
  Serial.begin(115200);

  // Check if LittleFS mounted correctly
  if (!LittleFS.begin(true)) {
    Serial.println("Error occurred while mounting LittleFS");
    return;
  }

  // Start the Access Point
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Failed to start access point server");
    return;
  }
  Serial.println();
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Configure and Start DNS Server
  dnsServer.setTTL(3600);
  dnsServer.setErrorReplyCode(AsyncDNSReplyCode::ServerFailure);
  if (!dnsServer.start(dnsPort, "*", WiFi.softAPIP())) {
    Serial.println("Failed to start DNS server");
    return;
  }
  softAPIPStr = "http://" + WiFi.softAPIP().toString() + "/";

  // Serve the root page from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/index.html"))
      request->send(LittleFS, "/index.html", "text/html");
    else {
      Serial.println("index.html not found in LittleFS");
      request->send(404, "text/plain", "index.html not found in LittleFS");
    }
  });

  // HTTP request from webpage to open door
  server.on("/open-door", HTTP_GET, [](AsyncWebServerRequest *request){
    if (state == MotorState::Closed) {
      openDoor = true;
      request->send(200, "text/plain", "Trigger received");
    } else {
      request->send(409, "text/plain", "Door not closed, cannot open");
    }
  });

  // Poll for door status to update webpage
  server.on("/get-status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", statusText());
  });

  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/");

  // Implements captive portal for specific URLs that OS's check
  // A tier from https://github.com/LuanTechAutomation/esp-captive-portal/tree/main
  server.on("/generate_204", [](AsyncWebServerRequest *request) { request->redirect(softAPIPStr); });		   // android captive portal redirect
  server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(softAPIPStr); });			   // microsoft redirect
  server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(softAPIPStr); });  // apple call home
  server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(softAPIPStr); });	   // firefox captive portal call home
  server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });					   // firefox captive portal call home
  server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(softAPIPStr); });			   // windows call home

  // added redirects I noticed from the catch-all
  // redirect /favicon.ico and /gen_204
  server.on("/favicon.ico", [](AsyncWebServerRequest *request) { request->redirect(softAPIPStr); }); // an android phone used this
  server.on("/gen_204", [](AsyncWebServerRequest *request) { request->redirect(softAPIPStr); }); // an iphone used this

  // Implements captive portal as a catch-all
  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.print("Redirecting unmatched request for: ");
    Serial.println(request->url());
    request->redirect(softAPIPStr);
  });

  // Start the background server
  server.begin();
  Serial.println("Async Server started.");

  // Setup motor
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, HIGH);  // LOW = enabled, HIGH = disabled (start out disabled, enable when needed)
  digitalWrite(dirPin, HIGH);
}

void loop() {
  // Start opening door when requested (only if closed)
  if (state == MotorState::Closed && openDoor) {
    openDoor = false;
    Serial.println("OPENING DOOR");
    digitalWrite(enPin, LOW); // re-enable driver (was disabled while idle)
    state = MotorState::Opening;
  }

  // Start closing door when timer is done
  if (state == MotorState::Open && millis() - openStartTime >= openTime) {
    Serial.println("CLOSING DOOR");
    digitalWrite(enPin, LOW); // re-enable driver (was disabled while idle)
    digitalWrite(dirPin, !digitalRead(dirPin));  // reverse
    state = MotorState::Closing;
  }

  // Blocking motor movement
  // Async Web Server should be fine to run despite the blocking loop
  if (state == MotorState::Opening || state == MotorState::Closing) {
    // One full output revolution:
    for (int i = 0; i < stepsToOpen; i++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(800);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(800);
    }

    if (state == MotorState::Opening) {
      Serial.println("DOOR OPEN");
      state = MotorState::Open;
      openStartTime = millis(); // start the timer
      digitalWrite(enPin, HIGH); // de-energize motor to reduce heat while idle
    } else if (state == MotorState::Closing) {
      Serial.println("DOOR CLOSED");
      state = MotorState::Closed;
      digitalWrite(enPin, HIGH);
    }
  }
}