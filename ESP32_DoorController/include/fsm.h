#pragma once
#include <motor.h>
#include <lcd.h>
#include <event.h>

enum DoorState {CLOSED, OPENING, OPEN, CLOSING, ERROR};

void fsmBegin(MotorController& motor, LCDDisplay& lcd);

void fsmLoop();

void emitEvent(Event event);

DoorState getState();