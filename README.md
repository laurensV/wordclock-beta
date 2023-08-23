## Wordclock Arduino
#### Components:
 * ESP8266
 * Neopixelstrip
 * DS3231

#### Board settings:
 * Board: NodeMCU 1.0 (ESP-12E Module)
 * Flash Size: 4MB (FS:2MB OTA:~1019KB)
 * Upload Speed: 115200

Board can be installed by adding this additional board url:
http://arduino.esp8266.com/stable/package_esp8266com_index.json

### Libraries
* https://github.com/tzapu/WiFiManager
* https://github.com/NorthernWidget/DS3231

### Logger
After establishing WiFi connection, you can also see the logs in your local network with the following python script:
```
python logger.py
```