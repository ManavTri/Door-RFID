#include "fsm.h"

FSM::FSM(MotorController& motor, LCDDisplay& lcd) : motor(motor), lcd(lcd), state(CLOSED) {}

void FSM::begin() {
    state = CLOSED;
    motor.stop();
    lcd.displayStatus("Closed");
}

void FSM::update() {
    switch(state) {

    }
}

void FSM::handleEvent(Event event) {
    switch(event) {

    }
}