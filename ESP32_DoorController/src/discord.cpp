#include "discord.h"

DiscordHandler::DiscordHandler(PasscodeManager& passcode_manager)
    : passcode_manager(passcode_manager) {}

void DiscordHandler::begin() {
    passcode_manager.begin();
}

void DiscordHandler::update() {

}

void DiscordHandler::setCallback(EventCallback cb) {
    callback = cb;
}