#include "fsm.h"

FSM::FSM(MotorController& motor, LCDDisplay& lcd) : motor(motor), lcd(lcd) {}

void FSM::begin() {
    motor.begin();
    lcd.begin();
    transitionState(CLOSED);
}

void FSM::update() {
    switch(state) {
        case CLOSED:

            break;
        case OPENING:
            if (motor.isDone())
                transitionState(OPEN);
            break;
        case OPEN:
            if (millis() - openStartTime >= OPEN_HOLD_TIME)
                transitionState(CLOSING);
            break;
        case CLOSING:
            if (motor.isDone())
                transitionState(CLOSED);
            break;
        case ERROR:

            break;
        default:
            transitionState(ERROR);
    }
}

void FSM::handleEvent(Event event) {
    switch(event) {
        case EVENT_RFID_AUTH:
            transitionState(OPENING);
            break;
        case EVENT_DISCORD_AUTH:
            transitionState(OPENING);
            break;
        case EVENT_ERROR:
        default:
            transitionState(ERROR);
    }
}

void FSM::transitionState(DoorState newState) {
    state = newState;
    switch (state) {
        case CLOSED:
            motor.stop();
            lcd.displayStatus("Scan Card");
            break;
        case OPENING:
            motor.open();
            lcd.displayStatus("Opening...");
            break;
        case OPEN:
            lcd.displayStatus("Door Open");
            break;
        case CLOSING:
            motor.close();
            lcd.displayStatus("Closing...");
            break;
        case ERROR:
            lcd.displayStatus("ERROR!");
            motor.stop();
            break;
        default:
            transitionState(ERROR);
    }
}