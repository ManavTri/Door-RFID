#pragma once
#include <random>
#include "lcd.h"
class PasscodeManager
{
private:
    static constexpr byte CODE_LEN = 4;
    static constexpr unsigned long MAX_TIME = 30 * 60 * 1000;
    String passcode;
    LCDDisplay& lcd;
    std::mt19937 rng;
    unsigned long genTime;
public:
    PasscodeManager(LCDDisplay& lcd, std::random_device rd);
    void begin();
    void update();
    void generateNewPasscode();
    bool checkPasscode(int submitted_code);
};