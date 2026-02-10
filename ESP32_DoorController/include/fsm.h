#pragma once
#include "motor.h"
#include "lcd.h"
#include "event.h"
class FSM
{
private:
    enum DoorState {CLOSED, OPENING, OPEN, CLOSING, ERROR};
    DoorState state;
    MotorController& motor;
    LCDDisplay& lcd;

public:
    FSM(MotorController& motor, LCDDisplay& lcd);
    void begin();
    void update();
    void handleEvent(Event event);
};