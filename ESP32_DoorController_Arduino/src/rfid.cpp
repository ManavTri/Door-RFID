#include "rfid.h"

RFIDReader::RFIDReader(int rst, int ss, int sck, int miso, int mosi)
    : rst_pin(rst_pin), ss_pin(ss_pin), sck_pin(sck), miso_pin(miso), mosi_pin(mosi), mfrc522(rst_pin, ss_pin) {}

void RFIDReader::begin() {
    EEPROM.begin(EEPROM_SIZE);
    Serial.println("EEPROM begin");
    Serial.println("Configuring WDT...");
    esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch
    SPI.begin(sck_pin, miso_pin, mosi_pin);
    Serial.println("SPI begin");
    mfrc522.PCD_Init();
    Serial.println("PCD Init");
    
    lastCheck = millis();
}

void RFIDReader::update() {
    unsigned long now = millis();
    if (now - lastCheck < CHECK_TIME) return;
    lastCheck = now;
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
    EEPROM.commit();
}

void RFIDReader::clearEEPROM(int index) {
    int base = index * UID_SIZE;
    for (int i = 0; i < UID_SIZE; i++) {
        EEPROM.write(base + i, 0x00);
    }
    EEPROM.commit();
}