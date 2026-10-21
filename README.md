# Sailbot Door Access

This is the GitHub Repository containing the code for our club's door access.

Previously, the door used RFID with physical Panther IDs, but since those are being phased out in favor of mobile IDs, we needed a new solution. A Discord Bot was considered, but we ultimately decided to implement door access using a website hosted on an ESP32 in Access Point (AP) mode. This means you can connect to the AP mode wifi and get a local website where you can open the door. The security comes from the password to the wifi. A DNS server is used to implement a captive portal, which is used by most modern OSs, particularly mobile OSs, to open the webiste automatically.
