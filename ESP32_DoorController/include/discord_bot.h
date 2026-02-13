#pragma once
#include "event.h"
#include "passcode.h"
extern "C" {
    #include <discord.h>
    #include <_gateway.h>
    #include <message.h>
    #include <user.h>
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