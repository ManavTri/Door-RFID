#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
class LCDDisplay
{
private:
    LiquidCrystal_I2C lcd;
    uint8_t cols;
    uint8_t rows;
    String clear_row;
public:
    LCDDisplay(uint8_t address, uint8_t cols, uint8_t rows);
    void begin();
    void displayCode(const String& msg);
    void displayStatus(const String& msg);
};