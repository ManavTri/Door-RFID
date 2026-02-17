#include "discord_bot.h"
#include <cstring>
#include <cstdlib>

DiscordHandler::DiscordHandler(PasscodeManager& pcm)
    : client(nullptr),
      passcodeManager(pcm),
      callback(nullptr),
      authorizedChannelId(),
      requiredRoleId(),
      requiredPermissionBits(0),
      SSID(),
      password(),
      wifiEnterpriseMode(false) {}

void DiscordHandler::setWiFiPSK(const std::string& ssid,
                                const std::string& pw) {
    wifiEnterpriseMode = false;
    SSID = ssid;
    password = pw;
}

void DiscordHandler::setWiFiEnterprise(const std::string& ssid,
                                       const std::string& identity,
                                       const std::string& username,
                                       const std::string& password) {
    wifiEnterpriseMode = true;
    SSID = ssid;
    eapIdentity = identity;
    eapUsername = username;
    eapPassword = password;
}

esp_err_t DiscordHandler::begin() {
    if (SSID.empty()) {
        ESP_LOGE(TAG, "WiFi SSID not set.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Initializing WiFi (SSID: %s)...", SSID.c_str());

    WiFi.mode(WIFI_STA);
    
    if (wifiEnterpriseMode) {
        ESP_LOGI(TAG, "Connecting using WPA2-Enterprise...");

        esp_wifi_sta_wpa2_ent_set_identity(
            (uint8_t*)eapIdentity.c_str(), eapIdentity.length());
        esp_wifi_sta_wpa2_ent_set_username(
            (uint8_t*)eapUsername.c_str(), eapUsername.length());
        esp_wifi_sta_wpa2_ent_set_password(
            (uint8_t*)eapPassword.c_str(), eapPassword.length());
        esp_wifi_sta_wpa2_ent_enable();

        WiFi.begin(SSID.c_str());
    } else {
        ESP_LOGI(TAG, "Connecting using WPA2-PSK...");
        WiFi.begin(SSID.c_str(), password.c_str());
    }


    int wifiAttempts = 0;

    while (WiFi.status() != WL_CONNECTED && wifiAttempts < MAX_WIFI_RETRIES) {
        ESP_LOGW(TAG, "Connecting to WiFi... (%d/%d)",
                 wifiAttempts + 1, MAX_WIFI_RETRIES);
        delay(500);
        wifiAttempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        ESP_LOGE(TAG, "WiFi connection failed. Institutional/captive networks may not be supported.");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WiFi connected, IP: %s",
             WiFi.localIP().toString().c_str());

    discord_config_t cfg = {
        .intents = DISCORD_INTENT_GUILD_MESSAGES |
                   DISCORD_INTENT_MESSAGE_CONTENT
    };

    client = discord_create(&cfg);
    if (!client) {
        ESP_LOGE(TAG, "Failed to create Discord client");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = discord_register_events(
        client,
        DISCORD_EVENT_ANY,
        &DiscordHandler::eventTrampoline,
        this
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register Discord events");
        discord_destroy(client);
        client = nullptr;
        return err;
    }

    for (int attempt = 1; attempt <= MAX_LOGIN_RETRIES; attempt++) {
        ESP_LOGI(TAG, "Logging into Discord (attempt %d/%d)...",
                 attempt, MAX_LOGIN_RETRIES);

        err = discord_login(client);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Discord login successful");
            break;
        }

        ESP_LOGW(TAG, "Discord login failed (err=%d)", err);
        delay(3000);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Discord login failed after retries");
        discord_destroy(client);
        client = nullptr;
        return err;
    }

    return ESP_OK;
}

void DiscordHandler::update() {
    unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastWiFiAttempt > 5000) {
            lastWiFiAttempt = now;
            ESP_LOGW(TAG, "WiFi disconnected, attempting reconnect...");
            WiFi.disconnect();
            WiFi.begin(SSID.c_str(), password.c_str());
        }
        return;
    }

    if (!discordConnected) {
        if (now - lastDiscordAttempt > 5000) {
            lastDiscordAttempt = now;
            ESP_LOGW(TAG, "Discord disconnected, retrying login...");
            esp_err_t err = discord_login(client);
            if (err == ESP_OK) {
                discordConnected = true;
                ESP_LOGI(TAG, "Discord reconnected");
            }
        }
    }
}

void DiscordHandler::setCallback(EventCallback cb) {
    callback = cb;
}

void DiscordHandler::setAuthorizedChannelId(const std::string& channelId) {
    authorizedChannelId = channelId;
}

void DiscordHandler::setRequiredRoleId(const std::string& roleId) {
    requiredRoleId = roleId;
}

void DiscordHandler::setRequiredPermissionBits(uint64_t bits) {
    requiredPermissionBits = bits;
}

void DiscordHandler::eventTrampoline(void* arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    (void)base;
    auto* self = static_cast<DiscordHandler*>(arg);
    auto* data = static_cast<discord_event_data_t*>(event_data);
    self->handleEvent(static_cast<discord_event_t>(event_id), data);
}

void DiscordHandler::handleEvent(discord_event_t event, discord_event_data_t* data) {
    if (event == DISCORD_EVENT_MESSAGE_RECEIVED) {
        auto* msg = static_cast<discord_message_t*>(data->ptr);
        handleMessage(msg);
    }
    if (event == DISCORD_EVENT_CONNECTED) discordConnected = true;
    if (event == DISCORD_EVENT_DISCONNECTED) discordConnected = false;
}

std::vector<std::string> DiscordHandler::tokenize(const char* content) {
    std::vector<std::string> tokens;
    if (!content) return tokens;

    const char* p = content;
    while (*p) {
        while (*p && std::isspace(static_cast<unsigned char>(*p))) ++p;
        if (!*p) break;
        const char* start = p;
        while (*p && !std::isspace(static_cast<unsigned char>(*p))) ++p;
        tokens.emplace_back(start, p - start);
    }
    return tokens;
}

void DiscordHandler::handleMessage(discord_message_t* msg) {
    if (!msg || !msg->content || !msg->author) return;
    if (msg->author->bot) return;

    auto tokens = tokenize(msg->content);
    if (tokens.empty()) return;
    if (tokens[0].empty() || tokens[0][0] != '!') return;

    if (tokens[0] == "!open") {
        handleUserCommand(tokens, msg);
    } else if (tokens[0] == "!help") {
        std::string help =
            "Commands:\n"
            "!open <passcode>\n"
            "!help\n"
            "!admin setchannel <channel_id>\n"
            "!admin setrole <role_id>\n"
            "!admin setperm <permission_bits>\n"
            "!admin config";
        sendSimpleMessage(msg->channel_id, help);
    } else if (tokens[0] == "!admin") {
        handleAdminCommand(tokens, msg);
    }
}

void DiscordHandler::handleUserCommand(const std::vector<std::string>& tokens, discord_message_t* msg) {
    if (!msg->channel_id) return;

    if (authorizedChannelId.empty()) {
        return;  // not configured yet
    }

    if (std::strcmp(msg->channel_id, authorizedChannelId.c_str()) != 0) {
        return;  // silent ignore
    }

    if (tokens.size() < 2) {
        sendSimpleMessage(msg->channel_id, "Usage: !open <passcode>");
        return;
    }

    const std::string& passcodeStr = tokens[1];

    if (!hasRequiredRole(msg->member)) {
        sendSimpleMessage(msg->channel_id, std::string(msg->author->username) + ": You do not have the required role");
        return;
    }

    if (!hasRequiredPermissions(msg->member, msg->guild_id)) {
        sendSimpleMessage(msg->channel_id, std::string(msg->author->username) + ": You do not have the required permissions");
        return;
    }

    int submitted = std::atoi(passcodeStr.c_str());
    bool ok = passcodeManager.checkPasscode(submitted);

    if (ok) {
        passcodeManager.generateNewPasscode();

        if (callback) {
            callback(EVENT_DISCORD_AUTH);
        }

        sendSimpleMessage(msg->channel_id, std::string(msg->author->username) + ": Door unlocked");
    } else {
        sendSimpleMessage(msg->channel_id, std::string(msg->author->username) + ": Incorrect passcode");
    }
}

void DiscordHandler::handleAdminCommand(const std::vector<std::string>& tokens, discord_message_t* msg) {
    if (!isAdmin(msg)) {
        sendSimpleMessage(msg->channel_id, std::string(msg->author->username) + ": You are not an administrator");
        return;
    }

    if (tokens.size() < 2) {
        sendSimpleMessage(msg->channel_id, "Usage: !admin <setchannel|setrole|setperm|wifi|config> ...");
        return;
    }

    const std::string& sub = tokens[1];

    if (sub == "setchannel") {
        if (tokens.size() < 3) {
            sendSimpleMessage(msg->channel_id, "Usage: !admin setchannel <channel_id>");
            return;
        }
        setAuthorizedChannelId(tokens[2]);
        sendSimpleMessage(msg->channel_id, "Authorized channel set to: " + tokens[2]);
    } else if (sub == "setrole") {
        if (tokens.size() < 3) {
            sendSimpleMessage(msg->channel_id, "Usage: !admin setrole <role_id>");
            return;
        }
        setRequiredRoleId(tokens[2]);
        sendSimpleMessage(msg->channel_id,"Required role set to: " + tokens[2]);
    } else if (sub == "setperm") {
        if (tokens.size() < 3) {
            sendSimpleMessage(msg->channel_id, "Usage: !admin setperm <permission_bits>");
            return;
        }
        uint64_t bits = std::strtoull(tokens[2].c_str(), nullptr, 0);
        setRequiredPermissionBits(bits);
        sendSimpleMessage(msg->channel_id, "Required permissions set to: " + tokens[2]);
    } else if (sub == "wifi_psk") {
        if (tokens.size() < 4) {
            sendSimpleMessage(msg->channel_id, "Usage: !admin wifi <ssid> <password>");
            return;
        }
        setWiFiPSK(tokens[2], tokens[3]);
        sendSimpleMessage(msg->channel_id, "PSK WiFi credentials updated. Rebooting...");
        delay(1000);
        esp_restart();
    } else if (sub == "wifi_enterprise") {
        if (tokens.size() < 6) {
            sendSimpleMessage(msg->channel_id,
                "Usage: !admin wifi_enterprise <ssid> <identity> <username> <password>");
            return;
        }
        setWiFiEnterprise(tokens[2], tokens[3], tokens[4], tokens[5]);
        sendSimpleMessage(msg->channel_id,
            "Enterprise WiFi updated. Rebooting...");
        delay(1000);
        esp_restart();
    } else if (sub == "config") {
        std::string cfg = "Current config:\n";
        cfg += "Authorized channel: " + (authorizedChannelId.empty() ? std::string("<none>") : authorizedChannelId) + "\n";
        cfg += "Required role: " + (requiredRoleId.empty() ? std::string("<none>") : requiredRoleId) + "\n";
        cfg += "Required permissions: ";
        cfg += (requiredPermissionBits == 0) ? "<none>" : std::to_string(requiredPermissionBits);
        sendSimpleMessage(msg->channel_id, cfg);
    } else {
        sendSimpleMessage(msg->channel_id, "Unknown admin subcommand. Use !admin config or !help.");
    }
}

bool DiscordHandler::isAdmin(discord_message_t* msg) {
    if (!msg || !msg->member || !msg->guild_id) return false;

    bool is_admin = false;
    esp_err_t err = discord_member_has_permissions(
        client,
        msg->member,
        msg->guild_id,
        DISCORD_PERMISSION_ADMINISTRATOR,
        &is_admin
    );
    if (err != ESP_OK) return false;
    return is_admin;
}

bool DiscordHandler::hasRequiredRole(discord_member_t* member) const {
    if (requiredRoleId.empty()) return true;
    if (!member || !member->roles) return false;

    for (discord_role_len_t i = 0; i < member->_roles_len; ++i) {
        const char* role_id = member->roles[i];
        if (role_id &&
            std::strcmp(role_id, requiredRoleId.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

bool DiscordHandler::hasRequiredPermissions(discord_member_t* member, const char* guild_id) const {
    if (requiredPermissionBits == 0) return true;
    if (!member || !guild_id) return false;

    bool has_perm = false;
    esp_err_t err = discord_member_has_permissions(
        client,
        member,
        const_cast<char*>(guild_id),
        requiredPermissionBits,
        &has_perm
    );
    if (err != ESP_OK) return false;
    return has_perm;
}

void DiscordHandler::sendSimpleMessage(const char* channel_id, const std::string& content) {
    if (!channel_id) return;

    discord_message_t msg = {};
    msg.channel_id = const_cast<char*>(channel_id);
    msg.content = const_cast<char*>(content.c_str());

    discord_message_send(client, &msg, nullptr);
}