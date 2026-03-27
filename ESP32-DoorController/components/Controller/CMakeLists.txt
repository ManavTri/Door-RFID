idf_component_register(SRCS "discord_bot.cpp" "fsm.cpp" "lcd.cpp" "motor.cpp" "passcode.cpp" "rfid.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_err esp_timer esp_log)
