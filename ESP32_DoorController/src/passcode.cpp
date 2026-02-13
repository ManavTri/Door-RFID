#include "passcode.h"

PasscodeManager::PasscodeManager(LCDDisplay& lcd, std::random_device rd) : lcd(lcd), rng(rd) {}

void PasscodeManager::begin() {
    generateNewPasscode();
}

void PasscodeManager::update() {
    if (millis() - genTime >= MAX_TIME)
        generateNewPasscode();
}

void PasscodeManager::generateNewPasscode() {
    passcode = "";
    std::uniform_int_distribution<int> randInt(1, 9);
    for (byte i = 0; i < CODE_LEN; i++) {
        passcode += String(randInt(rng));
    }
    genTime = millis();
    lcd.displayCode("Code: " + passcode);
}

bool PasscodeManager::checkPasscode(int submitted_code) {
    return String(submitted_code) == passcode;
}