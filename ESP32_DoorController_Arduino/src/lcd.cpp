#include "lcd.h"

LCDDisplay::LCDDisplay(uint8_t address, uint8_t cols, uint8_t rows)
    : lcd(address, cols, rows), cols(cols), rows(rows) {}

void LCDDisplay::begin() {
    lcd.init();
    lcd.backlight();
    lcd.clear();

    clear_row.reserve(cols);
    clear_row = "";
    for (uint8_t i = 0; i < cols; i++) {
        clear_row += ' ';
    }
}

void LCDDisplay::displayCode(const String& msg) {
    lcd.setCursor(0, 0);
    lcd.print(clear_row);
    lcd.setCursor(0, 0);
    lcd.print(msg);
}

void LCDDisplay::displayStatus(const String& msg) {
    lcd.setCursor(0, 1);
    lcd.print(clear_row);
    lcd.setCursor(0, 1);
    lcd.print(msg);
}