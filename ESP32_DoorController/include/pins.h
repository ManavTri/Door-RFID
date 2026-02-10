#pragma once

// LCD (I2C)
constexpr int I2C_SDA_PIN = 21;
constexpr int I2C_SCL_PIN = 22;
constexpr int LCD_I2C_ADDRESS = 0x27;
constexpr int LCD_COLS = 16;
constexpr int LCD_ROWS = 2;

// Stepper Motor Driver (A4988)
constexpr int stepPin = 3;
constexpr int dirPin  = 4;
constexpr int relayPin = 7;

// RFID (MFRC522)
constexpr int RST_PIN = 9;
constexpr int SS_PIN  = 10;

// SPI (ESP32 default VSPI)
constexpr int SPI_SCK_PIN  = 18;
constexpr int SPI_MISO_PIN = 19;
constexpr int SPI_MOSI_PIN = 23;