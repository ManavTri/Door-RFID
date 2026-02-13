#pragma once
#include "event.h"
#include "passcode.h"
extern "C" {
    #include <discord.h>
    #include <private/_gateway.h>
    #include <discord_http.h>
    #include <discord_message.h>
    #include <discord_user.h>
}

class DiscordHandler
{
private:
    PasscodeManager& passcode_manager;
    EventCallback callback = nullptr;
public:
    DiscordHandler(PasscodeManager& passcode_manager);
    void begin();
    void update();
    void setCallback(EventCallback cb);
};