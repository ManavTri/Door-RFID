#pragma once
#include "event.h"
#include "passcode.h"
class DiscordHandler
{
private:
    PasscodeManager passcode_manager;
    EventCallback callback = nullptr;
public:
    DiscordHandler(PasscodeManager passcode_manager);
    void begin();
    void update();
    void setCallback(EventCallback cb);
};