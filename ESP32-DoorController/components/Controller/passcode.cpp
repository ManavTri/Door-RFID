#include "passcode.h"
extern "C" {
    #include "esp_timer.h"
    #include "esp_log.h"
}

PasscodeManager::PasscodeManager(LCDDisplay& lcd) : lcd(lcd), rng(std::random_device{}()) {}

void PasscodeManager::begin() {
    generateNewPasscode();
}

void PasscodeManager::update() {
    if (esp_timer_get_time() - genTime >= MAX_TIME)
        generateNewPasscode();
}

void PasscodeManager::generateNewPasscode() {
    passcode = "";
    std::uniform_int_distribution<int> randInt(1, 9);
    for (byte i = 0; i < CODE_LEN; i++) {
        passcode += String(randInt(rng));
    }
    genTime = esp_timer_get_time();
    lcd.displayCode("Code: " + passcode);
    ESP_LOGI("PASSCODE", "%s", passcode);
}

bool PasscodeManager::checkPasscode(int submitted_code) {
    return String(submitted_code) == passcode;
}