# IRKit for ESPr IR (Arduino)

IR remote's receiving/sending application for ESPr IR module (ESP8266).
IRKit compatible.


## Description

You can buy ESPr IR module at SWITCH SCIENCE.
See https://www.switch-science.com/catalog/2740/

IRKit specifications at http://getirkit.com/
Refer "IRKit Device HTTP API".
You can use "GET /messages" for receiving IR signal
and "POST /messages" for sending IR signal.


## Before Compile

Need to write your WiFi SSID and Password in the code ESP_IRKit.ino.

----
// WiFi Router SSID and Password
const char *m_SSID = "Your WiFi SSID";
const char *m_Password = "Your WiFi Password";


## Demo

Access http://esp_irkit.local/ with web browser.
