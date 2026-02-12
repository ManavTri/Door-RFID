#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include "event.h"
class RFIDReader
{
private:
    static constexpr int EEPROM_SIZE = 512;
    static constexpr int UID_SIZE = 8;
    static constexpr int MAX_TAGS = EEPROM_SIZE / UID_SIZE;
    MFRC522 mfrc522;
    int ss_pin, rst_pin;
    EventCallback callback = nullptr;
    bool adminMode = false;
    bool uidEquals(const byte a[UID_SIZE], const byte b[UID_SIZE]);
    bool readCard(byte uidOut[UID_SIZE]);
    bool isMaster(const byte uid[UID_SIZE]);
    bool isAuthorized(const byte uid[UID_SIZE]);
    bool addTag(const byte uid[UID_SIZE]);
    bool removeTag(const byte uid[UID_SIZE]);
    void readEEPROM(byte out[UID_SIZE], int index);
    void writeEEPROM(const byte uid[UID_SIZE], int index);
    void clearEEPROM(int index);
public:
    RFIDReader(int rst_pin, int ss_pin);
    void begin();
    void update();
    void setCallback(EventCallback cb);
};