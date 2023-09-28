// standard libraries
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <time.h>
#include <coredecls.h>
#include <TZ.h>
#include <ESP8266WebServer.h>
#include "LittleFS.h"

// libraries from library manager
#include <DS3231.h> // https://github.com/NorthernWidget/DS3231
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include <ArduinoOTA.h>
#include "ArduinoJson.h"

// own services
#include "wordclock.h"
#include "src/udplogger.h"
#include "src/esp8266fota.h"
#include "src/ledmatrix.h"

uint8_t VERSION = {
#include "VERSION.h"  
};

// ----------------------------------------------------------------------------------
//                                    GLOBAL VARIABLES
// ----------------------------------------------------------------------------------
long lastReadTime = 0, lastCheckUpdate = 0, lastStoreColors = 0;
bool storeColorTime = false, storeColorName = false;
WiFiManager wifiManager;
UDPLogger logger;
DS3231 rtc;
esp8266FOTA FOTA("wordclock", VERSION);
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN);
LEDMatrix matrix(&pixels);
ESP8266WebServer server(80);

// ----------------------------------------------------------------------------------
//                                        SETUP
// ----------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.printf("\nSketchname: %s\nBuild: %s\n", (__FILE__), (__TIMESTAMP__));
  Serial.print("Version: "); Serial.println(VERSION);
  //Init EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Uncomment and run it once, if you want to erase stored info
  // reset();

  EEPROM.get(ADR_COLOR_TIME, color_TIME);
  if (!color_TIME) {
    color_TIME = WHITE;
  }
  EEPROM.get(ADR_COLOR_NAME, color_NAME);
  if (!color_NAME) {
    color_NAME = WHITE;
  }

  EEPROM.get(ADR_BRIGHTNESS, brightness);
  if (!brightness) {
    brightness = 255;
  }

  EEPROM.get(ADR_NM, checkNightMode);
  EEPROM.get(ADR_NM_START_H, nightModeStartHour);
  EEPROM.get(ADR_NM_START_M, nightModeStartMin);
  EEPROM.get(ADR_NM_END_H, nightModeEndHour);
  EEPROM.get(ADR_NM_END_M, nightModeEndMin);
  if (!nightModeStartHour && !nightModeStartMin && !nightModeEndHour && !nightModeEndMin) {
    nightModeStartHour = 0;
    nightModeStartMin = 0;
    nightModeEndHour = 7;
    nightModeEndMin = 0;
  }

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  server.onNotFound([]() {
    if (!handleFile(server.urlDecode(server.uri())))
      server.send(404, "text/plain", "FileNotFound");
  });

  setupTime();

  pixels.begin();
  pixels.setBrightness(brightness);
  pixels.clear();
  for (int i = 0; i < NUM_PIXELS; i++) {
    uint16_t hue = (i * 65536) / NUM_PIXELS;
    uint32_t color = Adafruit_NeoPixel::ColorHSV(hue);
    color = pixels.gamma32(color);
    pixels.setPixelColor(i, color);
    pixels.show();
    delay(20);
  }
  setupWifiManager();
  if (wifiManager.autoConnect(AP_SSID)) {
    print("Reconnected to WiFi");
    onWifiConnect();
  }
}

// ----------------------------------------------------------------------------------
//                                        LOOP
// ----------------------------------------------------------------------------------

void loop() {
  if (wifiManager.process()) {
    print("Connected to WiFi");
    onWifiConnect();
  }
  ArduinoOTA.handle();
  server.handleClient();

  if (readTimeInterval()) {
    time_t timeNowUTC;
    struct tm* timeInfo;
    timeNowUTC = (RTClib::now()).unixtime(); // time(nullptr);
    timeInfo = localtime(&timeNowUTC);
    int hours = timeInfo->tm_hour;
    int minutes = timeInfo->tm_min;
    setNightMode(hours * 60 + minutes);
    print(String(hours) + ":" + String(minutes) + " - ", false);
    String timeString = timeToString(hours, minutes);
    print(timeString);
    if (!nightMode) {
      showTimeString(timeString);
    } else {
      matrix.clear();
      matrix.draw();
    }
    checkWifiDisconnect();
  }
  if (storeColorsInterval()) {
    if (storeColorTime) {
      EEPROM.put(ADR_COLOR_TIME, color_TIME);
      storeColorTime = false;
    }
    if (storeColorName) {
      EEPROM.put(ADR_COLOR_NAME, color_NAME);
      storeColorName = false;
    }
    EEPROM.commit();
  }
  if (checkUpdateInterval() && AUTO_UPDATE) {
    if (isWifiConnected()) {
      print("checking for new updates..");
      bool updatedNeeded = FOTA.execHTTPcheck();
      if (updatedNeeded) {
        print("New update available!");
        FOTA.execOTA();
      } else {
        print("Already running latest version");
      }
    }
  }
}

// ----------------------------------------------------------------------------------
//                                        OTHER FUNCTIONS
// ----------------------------------------------------------------------------------
void print(String message, bool newline) {
  if (newline) {
    Serial.println(message);
  } else {
    Serial.print(message);
  }
  logger.log(message);
}
void print(int number, bool newline) {
  print(String(number), newline);
}

void setNightMode(int timeInMinutes) {
  int nightModeStartTime = (nightModeStartHour * 60 + nightModeStartMin);
  int nightModeEndTime = (nightModeEndHour * 60 + nightModeEndMin);
  if (nightModeStartTime > nightModeEndTime) {
    // over midnight
    nightMode = checkNightMode && timeInMinutes >= nightModeStartTime || timeInMinutes < nightModeEndTime;
  } else {
    nightMode = checkNightMode && timeInMinutes >= nightModeStartTime && timeInMinutes < nightModeEndTime;
  }
}

void showTimeString(String timeString) {
  int messageStart = 0;
  String word = "";
  int lastLetterClock = 0;
  int positionOfWord = 0;
  int nextSpace = 0;
  int index = 0;

  // add space on the end of timeString for splitting
  timeString = timeString + " ";

  matrix.clear();

  while (true) {
    // extract next word from timeString
    word = split(timeString, ' ', index);
    index++;

    if (word.length() > 0) {
      positionOfWord = letters.indexOf(word, lastLetterClock);

      if (positionOfWord >= 0) {
        // word found on clock
        for (int i = 0; i < word.length(); i++) {
          int row = (positionOfWord + i) / CLOCK_WIDTH;
          int col = (positionOfWord + i) % CLOCK_WIDTH;
          matrix.setPixelType(row, col, LEDMatrix::TIME);
        }
        lastLetterClock = positionOfWord + word.length();
      } else {
        // word is not possible to show on clock
        print("word is not possible to show on clock: " + String(word));
      }
    } else {
      // end - no more word in message
      break;
    }
  }
  for (int i = 0; i < letters.length(); i++) {
    if (letters.charAt(i) == '*') {
      int row = i / CLOCK_WIDTH;
      int col = i % CLOCK_WIDTH;
      matrix.setPixelType(row, col, LEDMatrix::NAME);
    }
  }
  matrix.draw();
}

/**
 * @brief Splits a string at given character and return specified element
 *
 * @param s string to split
 * @param parser separating character
 * @param index index of the element to return
 * @return String
 */
String split(String s, char parser, int index) {
  String rs = "";
  int parserIndex = index;
  int parserCnt = 0;
  int rFromIndex = 0, rToIndex = -1;
  while (index >= parserCnt) {
    rFromIndex = rToIndex + 1;
    rToIndex = s.indexOf(parser, rFromIndex);
    if (index == parserCnt) {
      if (rToIndex == 0 || rToIndex == -1) return "";
      return s.substring(rFromIndex, rToIndex);
    } else parserCnt++;
  }
  return rs;
}

void pauseTimer(long* timer, unsigned long delay = 0) {
  if (!delay) {
    *timer = LONG_MAX; // infinite
  } else {
    *timer = millis() + delay;
  }
}

void resumeTimer(long* timer) {
  *timer = millis();
}

bool readTimeInterval() {
  bool interval = (long)millis() - lastReadTime > PERIOD_READTIME;
  if (interval) lastReadTime = millis();
  return interval;
}

bool checkUpdateInterval() {
  bool interval = (long)millis() - lastCheckUpdate > PERIOD_CHECK_UPDATE;
  if (interval) lastCheckUpdate = millis();
  return interval;
}

bool storeColorsInterval() {
  bool interval = (storeColorName || storeColorTime) && (long)millis() - lastCheckUpdate > PERIOD_STORE_COLORS;
  if (interval) lastCheckUpdate = millis();
  return interval;
}

bool isWifiConnected() {
  return (WiFi.status() == WL_CONNECTED) && (WiFi.localIP().toString() != "0.0.0.0");
}

void checkWifiDisconnect() {
  if (!wifiManager.getConfigPortalActive()) {
    if (WiFi.status() != WL_CONNECTED) {
      print("no wifi connection..");
      pixels.setPixelColor(100, pixels.Color(255, 0, 0, 0));
      pixels.show();
    } else if (WiFi.localIP().toString() == "0.0.0.0") {
      // detected bug: ESP8266 - WiFi status not changing
      // https://github.com/esp8266/Arduino/issues/2593#issuecomment-323801447
      print("no IP address, reconnecting WiFi..");
      WiFi.reconnect();
    }
  }
}

void printIP() {
  print("IP address: ");
  print(WiFi.localIP().toString());
  uint8_t address = WiFi.localIP()[3];
  pauseTimer(&lastReadTime, 10000);
  matrix.clear();
  matrix.print(FONT_I, 1, 0, LEDMatrix::OTHER);
  matrix.print(FONT_P, 5, 0, LEDMatrix::OTHER);
  uint8_t x = 0;
  if (address / 100 != 0) {
    matrix.print(FONT_NUMBERS[(address / 100)], x, 6, LEDMatrix::OTHER);
    x += 4;
  }
  if ((address / 10) % 10 != 0) {
    matrix.print(FONT_NUMBERS[(address / 10) % 10], x, 6, LEDMatrix::OTHER);
    x += 4;
  }
  matrix.print(FONT_NUMBERS[address % 10], x, 6, LEDMatrix::OTHER);
  matrix.draw();
}

void setupWifiManager() {
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setHostname(HOSTNAME);
}

void setupServer() {
  server.enableCORS(true);
  server.on("/api/color", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.send(204);
  });
  server.on("/api/settings", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.send(204);
  });
  server.on("/api/settings", HTTP_GET, getSettings);
  server.on("/api/settings", HTTP_POST, saveSettings);
  server.on("/api/color", HTTP_POST, setColor);
  server.on("/api/color", HTTP_GET, getColor);
  server.begin();
}

void onWifiConnect() {
  setupOTA();
  setupServer();
  logger = UDPLogger(WiFi.localIP(), IPAddress(230, 120, 10, 2), 8123);
  printIP();
  configTime(TIMEZONE, "pool.ntp.org", "time.cloudflare.com", "time.google.com");
}
bool saveSettings() {
  server.sendHeader("Access-Control-Allow-Headers", "*");
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "Body not received");
    return false;
  }
  StaticJsonDocument<300> JSONDocument;
  DeserializationError err = deserializeJson(JSONDocument, server.arg("plain"));
  if (err) { //Check for errors in parsing
    server.send(400, "application/json", "Body could not be parsed");
    return false;
  }
  if (JSONDocument["brightness"] >= 10 && JSONDocument["brightness"] <= 255) {
    brightness = JSONDocument["brightness"];
    pixels.setBrightness(brightness);
    EEPROM.put(ADR_BRIGHTNESS, brightness);
  }
  checkNightMode = JSONDocument["nm"];
  EEPROM.put(ADR_NM, checkNightMode);

  if (JSONDocument["nm_start_h"] >= 0 && JSONDocument["nm_start_h"] <= 23 &&
    JSONDocument["nm_start_m"] >= 0 && JSONDocument["nm_start_m"] <= 59 &&
    JSONDocument["nm_end_h"] >= 0 && JSONDocument["nm_end_h"] <= 23 &&
    JSONDocument["nm_end_m"] >= 0 && JSONDocument["nm_end_m"] <= 59) {
    nightModeStartHour = JSONDocument["nm_start_h"];
    nightModeStartMin = JSONDocument["nm_start_m"];
    nightModeEndHour = JSONDocument["nm_end_h"];
    nightModeEndMin = JSONDocument["nm_end_m"];
    EEPROM.put(ADR_NM_START_H, nightModeStartHour);
    EEPROM.put(ADR_NM_START_M, nightModeStartMin);
    EEPROM.put(ADR_NM_END_H, nightModeEndHour);
    EEPROM.put(ADR_NM_END_M, nightModeEndMin);
  }
  EEPROM.commit();
  matrix.draw();
  server.send(200, "application/json", server.arg("plain"));
  return true;
}

bool getSettings() {
  server.send(200, "application/json", "{\"brightness\": " + String(brightness) + ", \"nm\": " + String(checkNightMode) + ", \"nm_start_h\": " + String(nightModeStartHour) + ", \"nm_start_m\": " + String(nightModeStartMin) + ", \"nm_end_h\": " + String(nightModeEndHour) + ", \"nm_end_m\": " + String(nightModeEndMin) + "}");
  return true;
}

bool setColor() {
  server.sendHeader("Access-Control-Allow-Headers", "*");
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "Body not received");
    return false;
  }
  StaticJsonDocument<300> JSONDocument;
  DeserializationError err = deserializeJson(JSONDocument, server.arg("plain"));
  if (err) { //Check for errors in parsing
    server.send(400, "application/json", "Body could not be parsed");
    return false;
  }
  uint32_t color = Adafruit_NeoPixel::Color(JSONDocument["r"], JSONDocument["g"], JSONDocument["b"]);

  if (server.arg("type") == "time") {
    color_TIME = color;
    storeColorTime = true;
  } else if (server.arg("type") == "name") {
    color_NAME = color;
    storeColorName = true;
  } else {
    server.send(400, "application/json", "Color type unknown");
    return false;
  }
  matrix.draw();

  server.send(200, "application/json", server.arg("plain"));
  return true;
}
bool getColor() {
  uint32_t color;
  if (server.arg("type") == "time") {
    color = color_TIME;
  } else if (server.arg("type") == "name") {
    color = color_NAME;
  } else {
    server.send(400, "application/json", "Color type unknown");
    return false;
  }
  server.send(200, "application/json", (String)color);
  return true;
}

void onUpdateProgress(unsigned int progress, unsigned int total) {
  print("Progress: " + String(progress / (total / 100)));
  pixels.clear();
  for (int i = 0; i < progress / (total / NUM_PIXELS); i++) {
    int row = i / CLOCK_WIDTH;
    pixels.setPixelColor(row % 2 ? row * CLOCK_WIDTH + ((row + 1) * CLOCK_WIDTH - i - 1) : i, pixels.Color(0, 0, 255));
  }
  pixels.show();
}
void onUpdateFinished() {
  pixels.clear();
  for (int i = 0; i < NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 255, 0));
  }
  pixels.show();
  delay(200);
}

void setupOTA() {
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
  FOTA.checkURL = "https://raw.githubusercontent.com/laurensV/wordclock/main/firmware/version.json";
  FOTA.onProgress(onUpdateProgress);
  FOTA.onEnd(onUpdateFinished);
  ArduinoOTA.onProgress(onUpdateProgress);
  ArduinoOTA.onEnd(onUpdateFinished);
}

void setupTime() {
  // Set timezone
  setTZ(TIMEZONE);
  // Start the I2C interface (needed for DS3231 clock)
  Wire.begin();
  rtc.setClockMode(false);
  bool rtcSet;
  EEPROM.get(ADR_RTC_SET, rtcSet);
  if ((rtcSet && rtc.getYear())) {
    print("RTC already set, skipping set time");
  } else {
    if (rtcSet) {
      print("WARNING: RTC lost power. Compile again or connect the WiFi to sync the time");
    }
    DateTime compiled = DateTime(__DATE__, __TIME__);
    time_t local_compile = compiled.unixtime();
    struct tm* tmptime;
    tmptime = localtime(&local_compile);
    tmptime->tm_isdst = 0;
    int32_t offset = mktime(tmptime) - mktime(gmtime(&local_compile));
    print("setting RTC to compile UTC time + 33 seconds");
    rtc.setEpoch(local_compile - offset + 33);
    EEPROM.put(ADR_RTC_SET, true);
    EEPROM.commit();
  }

  // callback for set time updates
  settimeofday_cb(time_set);
}

void reset() {
  print("resetting WiFi settings");
  wifiManager.resetSettings();

  print("empty entire EEPROM");
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}


/**
 * @brief Converts the given time as sentence (String)
 *
 * @param hours hours of the time value
 * @param minutes minutes of the time value
 * @return String time as sentence
 */
String timeToString(uint8_t hours, uint8_t minutes) {
  String message = "HET IS ";

  String minuteWords[] = { "EEN", "TWEE", "DRIE", "VIER", "VIJF", "ZES", "ZEVEN", "ACHT", "NEGEN", "TIEN", "ELF", "TWAALF", "DERTIEN", "VEERTIEN", "KWART", "ZES -TIEN", "ZEVEN -TIEN", "ACHT -TIEN", "NEGEN-TIEN" };
  //show minutes
  if (minutes > 0 && minutes < 20) {
    message += minuteWords[minutes - 1] + " OVER ";
  } else if (minutes >= 20 && minutes < 30) {
    message += minuteWords[29 - minutes] + " VOOR HALF ";
  } else if (minutes == 30) {
    message += "HALF ";
  } else if (minutes > 30 && minutes < 45) {
    message += minuteWords[minutes - 31] + " OVER HALF ";
  } else if (minutes >= 45 && minutes <= 59) {
    message += minuteWords[59 - minutes] + " VOOR ";
  }

  //convert hours to 12h format
  if (hours >= 12) hours -= 12;
  if (minutes >= 20) hours++;
  if (hours == 0) hours = 12;
  message += minuteWords[hours - 1] + " ";

  if (minutes == 0) message += "UUR ";

  return message;
}

// NTP functions
void time_set(bool from_sntp) {
  if (from_sntp) {
    print("System time updated from NTP server");
    // Update RTC if time came from NTP server
    rtc.setEpoch(time(nullptr));
  } else {
    print("System time updated from RTC");
  }
}
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
  return PERIOD_NTP_UPDATE;
}
uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000() {
  return 2000UL; // 2s, 60s limit is not a restriction anymore
}

bool handleFile(String&& path) {
  if (path.endsWith("/")) path += "index.html";
  return LittleFS.exists(path) ? ({ File f = LittleFS.open(path, "r"); server.streamFile(f, mime::getContentType(path)); f.close(); true; }) : false;
}