# Sailbot Door Access

## Intro

This is the GitHub Repository containing the code for our club's door access.

Previously, the door used RFID with physical Panther IDs, but since those are being phased out in favor of mobile IDs, we needed a new solution. A Discord Bot was considered, but we ultimately decided to implement door access using a website hosted on an ESP32 in Access Point (AP) mode. This means you can connect to the AP mode wifi and get a local website where you can open the door. The security comes from the password to the wifi. A DNS server is used to implement a captive portal, which is used by most modern OSs, particularly mobile OSs, to open the website automatically.

## Usage

You can flash this code to an ESP32 using PlatformIO. To change the SSID and/or Password, you can update the `cred.h` file, specifying the following fields: `char* ssid` and `char* password`. Make sure to reflash the ESP32 for the updates to be reflected.

## What's Next

In the future, there are multiple items to do. The motor control code can be changed to be non-blocking, which would involve removing the for loop and stepping the motor on each call of `loop()`, which wasn't necessary using the Async Libraries for the web server and DNS server, but may be necessary for other solutions in the future. A Discord Bot can be used instead of an AP mode website. This would involve connecting the ESP32 to Pitt WiFi, such as Pitt MDA (requires communication with the Pitt Digital Help Desk), setting up the WebSocket communication with Discord, and then implementing the bot commands to open the door.
