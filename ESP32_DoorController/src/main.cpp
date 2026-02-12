#include <Arduino.h>
#include "fsm.h"
#include "event.h"
#include "rfid.h"
#include "discord.h"
#include "passcode.h"
#include "motor.h"
#include "lcd.h"
#include "pins.h"

MotorController motor(MOTOR_STEP_PIN, MOTOR_DIR_PIN, MOTOR_RELAY_PIN);
LCDDisplay lcd(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS);
FSM fsm(motor, lcd);
RFIDReader rfid(RFID_RST_PIN, RFID_SS_PIN);
void setup() {
    Serial.begin(115200);
    fsm.begin();
    rfid.setCallback([](Event e) {fsm.handleEvent(e);});
    rfid.begin();
}

void loop() {
    rfid.update();
    motor.update();
    fsm.update();
}