#include "rfid.h"

RFIDReader::RFIDReader(int rst_pin, int ss_pin)
    : rst_pin(rst_pin), ss_pin(ss_pin), mfrc522(rst_pin, ss_pin) {}

void RFIDReader::begin() {
    SPI.begin();
    mfrc522.PCD_Init();
    EEPROM.begin(EEPROM_SIZE);
}

void RFIDReader::update() {
    byte uid[UID_SIZE];

    if (!readCard(uid)) return;

    if (isMaster(uid)) {
        adminMode = !adminMode;
        return;
    }

    if (adminMode) {
        if (!removeTag(uid)) {
            addTag(uid);
        }
        return;
    }

    if (isAuthorized(uid)) {
        if (callback) callback(EVENT_RFID_AUTH);
    }
}

void RFIDReader::setCallback(EventCallback cb) {
    callback = cb;
}

bool RFIDReader::uidEquals(const byte a[UID_SIZE], const byte b[UID_SIZE]) {
    for (int i = 0; i < UID_SIZE; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

bool RFIDReader::readCard(byte uidOut[UID_SIZE]) {
    if (!mfrc522.PICC_IsNewCardPresent()) return false;
    if (!mfrc522.PICC_ReadCardSerial()) return false;

    for (int i = 0; i < UID_SIZE; i++) {
        if (i < mfrc522.uid.size) {
            uidOut[i] = mfrc522.uid.uidByte[i];
        } else {
            uidOut[i] = 0x00;
        }
    }

    mfrc522.PICC_HaltA();
    return true;
}

bool RFIDReader::isMaster(const byte uid[UID_SIZE]) {
    byte master[UID_SIZE];
    readEEPROM(master, 1);
    return uidEquals(uid, master);
}

bool RFIDReader::isAuthorized(const byte uid[UID_SIZE]) {
    byte stored[UID_SIZE];

    for (int i = 2; i < MAX_TAGS; i++) { 
        readEEPROM(stored, i);

        bool empty = true;
        for (int j = 0; j < UID_SIZE; j++) {
            if (stored[j] != 0x00) {
                empty = false;
                break;
            }
        }
        if (empty) continue;

        if (uidEquals(uid, stored)) return true;
    }

    return false;
}

bool RFIDReader::addTag(const byte uid[UID_SIZE]) {
    byte stored[UID_SIZE];

    for (int i = 2; i < MAX_TAGS; i++) {
        readEEPROM(stored, i);

        bool empty = true;
        for (int j = 0; j < UID_SIZE; j++) {
            if (stored[j] != 0x00) {
                empty = false;
                break;
            }
        }

        if (empty) {
            writeEEPROM(uid, i);
            return true;
        }
    }

    return false;
}

bool RFIDReader::removeTag(const byte uid[UID_SIZE]) {
    byte stored[UID_SIZE];

    for (int i = 2; i < MAX_TAGS; i++) {
        readEEPROM(stored, i);
        if (uidEquals(uid, stored)) {
            clearEEPROM(i);
            return true;
        }
    }
    return false;
}

void RFIDReader::readEEPROM(byte out[UID_SIZE], int index) {
    int base = index * UID_SIZE;
    for (int i = 0; i < UID_SIZE; i++) {
        out[i] = EEPROM.read(base + i);
    }
}

void RFIDReader::writeEEPROM(const byte uid[UID_SIZE], int index) {
    int base = index * UID_SIZE;
    for (int i = 0; i < UID_SIZE; i++) {
        EEPROM.write(base + i, uid[i]);
    }
}

void RFIDReader::clearEEPROM(int index) {
    int base = index * UID_SIZE;
    for (int i = 0; i < UID_SIZE; i++) {
        EEPROM.write(base + i, 0x00);
    }
}