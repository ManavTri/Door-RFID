#pragma once
#include "motor.h"
#include "lcd.h"
#include "event.h"
class FSM
{
private:
    enum DoorState {CLOSED, OPENING, OPEN, CLOSING, ERROR};
    static constexpr unsigned long OPEN_HOLD_TIME = 3000;
    DoorState state;
    MotorController& motor;
    LCDDisplay& lcd;
    unsigned long openStartTime = 0;
public:
    FSM(MotorController& motor, LCDDisplay& lcd);
    void begin();
    void update();
    void handleEvent(Event event);
    void transitionState(DoorState newState);
};