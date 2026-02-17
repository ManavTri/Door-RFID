#pragma once
#include "event.h"
#include "passcode.h"
#include <WiFi.h>
#include <string>
#include <vector>
#include <cstdint>
#include "esp_wpa2.h"
extern "C" {
    #include <discord.h>
    #include <_gateway.h>
    #include <message.h>
    #include <user.h>
}

class DiscordHandler {
public:
    DiscordHandler(PasscodeManager& passcodeManager);
    esp_err_t begin();
    void update();
    void setCallback(EventCallback cb);
    void setAuthorizedChannelId(const std::string& channelId);
    void setRequiredRoleId(const std::string& roleId);
    void setRequiredPermissionBits(uint64_t bits);
    void setWiFiPSK(const std::string& ssid, const std::string& pw);
    void setWiFiEnterprise(const std::string& ssid, const std::string& identity, const std::string& username, const std::string& password);
private:
    static void eventTrampoline(void* arg, esp_event_base_t base, int32_t event_id, void* event_data);
    void handleEvent(discord_event_t event, discord_event_data_t* data);
    void handleMessage(discord_message_t* msg);
    void handleUserCommand(const std::vector<std::string>& tokens, discord_message_t* msg);
    void handleAdminCommand(const std::vector<std::string>& tokens, discord_message_t* msg);
    bool isAdmin(discord_message_t* msg);
    bool hasRequiredRole(discord_member_t* member) const;
    bool hasRequiredPermissions(discord_member_t* member, const char* guild_id) const;
    void sendSimpleMessage(const char* channel_id, const std::string& content);
    static std::vector<std::string> tokenize(const char* content);
    static constexpr int MAX_WIFI_RETRIES = 20;
    static constexpr int MAX_LOGIN_RETRIES = 5;
    const String TAG = "DiscordHandler";
    std::string SSID = "";
    std::string password = "";
    discord_handle_t client;
    PasscodeManager& passcodeManager;
    EventCallback callback;
    std::string authorizedChannelId;
    std::string requiredRoleId;
    uint64_t requiredPermissionBits;
    unsigned long lastWiFiAttempt;
    unsigned long lastDiscordAttempt;
    bool discordConnected;
    bool wifiEnterpriseMode;
    std::string eapIdentity;
    std::string eapUsername;
    std::string eapPassword;
};