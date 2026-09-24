#include <Arduino.h>
#include <WiFi.h>
#include <AccelStepper.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h> // used to include the html,css,js webpage code
#include <string>
#include "cred.h" // Sets access point credentials (ignored by Git)

// Motor pins
const int stepPin = 26;
const int dirPin = 27;
const int enPin = 25; // A4988 ENABLE pin -- active LOW (LOW = driver enabled)

// Flag for motor (volatile as can be changed from webpage)
volatile bool openDoor = false;

// How long to hold the latch open before auto-closing, in milliseconds.
const unsigned long openDwellMs = 3000;

// Timestamp (millis()) of when the door entered the Open state.
unsigned long openStartTime = 0;

// How far the door needs to travel to fully open, in steps.
// TUNE THIS to your actual hardware (motor steps/rev * microstepping * gear ratio).
#define STEPS_TO_OPEN 1000

// Create the AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create stepper using NEMA 17 motor & A4988 Driver
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);

// Motor state machine
enum class MotorState {
  Closed,
  Opening,
  Open,
  Closing
};

MotorState state = MotorState::Closed;

// Human-readable status derived from `state`, used for the webpage poll.
// (Avoids sharing a mutable Arduino String across the web-server task and loop().)
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

  // Enable pin setup -- A4988 EN is active LOW, so LOW enables the driver.
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, LOW); // driver enabled; set HIGH to de-energize motor when idle

  // Check if LittleFS mounted correctly
  if (!LittleFS.begin(true)) {
    Serial.println("Error occurred while mounting LittleFS");
    return;
  }

  // Start the Access Point
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Serve the root page from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
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

  // Start the background server
  server.begin();
  Serial.println("Async Server started.");

  // Setup motor
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500); // tune to taste; remove accel calls if you want constant-speed only
  stepper.setCurrentPosition(0);
}

void loop() {
  // Kick off an opening move when requested
  if (state == MotorState::Closed && openDoor) {
    openDoor = false;
    Serial.println("OPENING DOOR");
    digitalWrite(enPin, LOW); // re-enable driver (was disabled while idle)
    stepper.moveTo(STEPS_TO_OPEN);
    state = MotorState::Opening;
  }

  // Kick off a closing move once the dwell time has elapsed
  if (state == MotorState::Open && millis() - openStartTime >= openDwellMs) {
    Serial.println("CLOSING DOOR");
    digitalWrite(enPin, LOW); // re-enable driver (was disabled while idle)
    stepper.moveTo(0); // back to the Closed reference position
    state = MotorState::Closing;
  }

  // Drive the stepper toward its target every loop iteration.
  // AccelStepper does nothing unless run() (or runSpeed()) is called continuously.
  if (state == MotorState::Opening || state == MotorState::Closing) {
    stepper.run();

    if (stepper.distanceToGo() == 0) {
      if (state == MotorState::Opening) {
        Serial.println("DOOR OPEN");
        state = MotorState::Open;
        openStartTime = millis(); // start the dwell timer
        digitalWrite(enPin, HIGH); // de-energize motor to reduce heat while idle
      } else if (state == MotorState::Closing) {
        Serial.println("DOOR CLOSED");
        state = MotorState::Closed;
        digitalWrite(enPin, HIGH);
      }
    }
  }
}