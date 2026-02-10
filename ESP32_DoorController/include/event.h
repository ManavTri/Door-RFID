#pragma once
enum Event {
    EVENT_RFID_AUTH,
    EVENT_DISCORD_AUTH,
    EVENT_DOOR_OPENED,
    EVENT_DOOR_CLOSED,
    EVENT_ERROR
};

typedef void (*EventCallback)(Event);