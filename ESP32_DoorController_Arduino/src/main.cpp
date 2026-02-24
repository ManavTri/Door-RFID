#include <Arduino.h>
#include "fsm.h"
#include "event.h"
#include "rfid.h"
#include "discord_bot.h"
#include "passcode.h"
#include "motor.h"
#include "lcd.h"
#include "pins.h"
#include "wifi_credentials.h"
#include "discord_config.h"

MotorController motor(MOTOR_STEP_PIN, MOTOR_DIR_PIN, MOTOR_RELAY_PIN);
LCDDisplay lcd(LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS);
FSM fsm(motor, lcd);
RFIDReader rfid(RFID_RST_PIN, RFID_SS_PIN, SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
PasscodeManager passcodeManager(lcd);
DiscordHandler discordHandler(passcodeManager);
void setup() {
    Serial.begin(115200);
    Serial.println("BOOTED");
    fsm.begin();
    Serial.println("FSM done");
    // rfid.setCallback([](Event e) {fsm.handleEvent(e);});
    // rfid.begin();
    Serial.println("RFID done");
    discordHandler.setCallback([](Event e) {fsm.handleEvent(e);});
    discordHandler.setWiFiPSK(wifi_psk_SSID, wifi_psk_password);
    // discordHandler.setWiFiEnterprise(wifi_ent_SSID, wifi_ent_identity, wifi_ent_username, wifi_ent_password);
    discordHandler.begin(token);
    Serial.println("Discord done");
}

void loop() {
    discordHandler.update();
    // rfid.update();
    motor.update();
    fsm.update();
    delay(5);
}