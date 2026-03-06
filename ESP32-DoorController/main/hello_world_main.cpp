/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <estr.h>
extern "C" {
    #include "sdkconfig.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_chip_info.h"
    #include "esp_flash.h"
    #include "esp_log.h"
    #include "esp_event.h"
    #include "esp_system.h"
    #include "nvs_flash.h"
    #include "protocol_examples_common.h"
    #include "discord.h"
    #include "discord/session.h"
    #include "discord/message.h"
}

static discord_handle_t bot;

extern "C" static void bot_event_handler(void* handler_arg, esp_event_base_t base, int32_t event_id, void* event_data) {
    discord_event_data_t* data = (discord_event_data_t*) event_data;

    switch(event_id) {
        case DISCORD_EVENT_CONNECTED:
            discord_session_t* session = (discord_session_t*) data->ptr;
            ESP_LOGI("BOT", "Bot %s#%s connected", session->user->username, session->user->discriminator);
            break;
        case DISCORD_EVENT_MESSAGE_RECEIVED:
            discord_message_t* msg = (discord_message_t*) data->ptr;
            ESP_LOGI("BOT", "New message (dm=%s, bot=%s, channel=%s, guild=%s, content=%s)", !msg->guild_id ? "true" : "false", msg->author->username, msg->author->discriminator, msg->author->bot ? "true" : "false", msg->channel_id, msg->guild_id ? msg->guild_id : "NULL", msg->content);
            char* echo_content = estr_cat("Hello ", msg->author->username, ", I am a bot!");
            discord_message_t echo = {
                .content = echo_content,
                .channel_id = msg->channel_id
            };
            if (discord_message_send(bot, &echo, NULL) == ESP_OK)
                ESP_LOGI("BOT", "Message sent successfully");
            else
                ESP_LOGE("BOT", "Message failed to send");
            break;
        case DISCORD_EVENT_DISCONNECTED:
            ESP_LOGW("BOT", "Bot disconnected.");
            break;
        default:
            ESP_LOGW("BOT", "Unknown Discord Event");
    }
}

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    discord_config_t config = {
        .intents = DISCORD_INTENT_GUILD_MESSAGES
    };

    bot = discord_create(&config);
    discord_register_events(bot, DISCORD_EVENT_ANY, bot_event_handler, NULL);
    discord_login(bot);
}
