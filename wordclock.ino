#include <EEPROM.h>
#include <DS3231.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoOTA.h>

#include "src/udplogger.h"

// ----------------------------------------------------------------------------------
//                                        CONSTANTS
// ----------------------------------------------------------------------------------
#define PERIOD_HEARTBEAT 1000
#define PERIOD_NTP_UPDATE 30000
#define AP_SSID "JouwWoordklok"
#define HOSTNAME "jouwwoordklok"

#define EEPROM_SIZE 4      // size of EEPROM to save persistent variables
#define ADR_RTC 0

// ----------------------------------------------------------------------------------
//                                    GLOBAL VARIABLES
// ----------------------------------------------------------------------------------
long lastHeartBeat = millis();
long lastNTPUpdate = millis();

WiFiManager wifiManager;
UDPLogger logger;
DS3231 rtc;
bool h12, hPM;

// ----------------------------------------------------------------------------------
//                                        SETUP
// ----------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\nSketchname: %s\nBuild: %s\n", (__FILE__), (__TIMESTAMP__));

  //Init EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Uncomment and run it once, if you want to erase stored info
  reset();

  setupRTC();

  setupWifiManager();
  if (wifiManager.autoConnect(AP_SSID)) {
    logger.log("Reonnected to WiFi");
    onWifiConnect();
  }
}

// ----------------------------------------------------------------------------------
//                                        LOOP
// ----------------------------------------------------------------------------------

void loop() {
  wifiManager.process();
  ArduinoOTA.handle();

  if (ntpUpdateInterval()) {
    if (isWifiConnected()) {
      logger.log("update NTP");
      // ntp update
    }
  }

  if (heartbeatInterval()) {
    logger.log("heartbeat");
    int hours = rtc.getHour(h12, hPM);
    int minutes = rtc.getMinute();
    logger.log(String(hours) + ":" + String(minutes));
    String message = timeToString(hours, minutes);
    logger.log(message);
    checkWifiDisconnect();
  }
}

// ----------------------------------------------------------------------------------
//                                        OTHER FUNCTIONS
// ----------------------------------------------------------------------------------
bool heartbeatInterval() {
  bool interval = millis() - lastHeartBeat > PERIOD_HEARTBEAT;
  if (interval) lastHeartBeat = millis();
  return interval;
}
bool ntpUpdateInterval() {
  bool interval = millis() - lastNTPUpdate > PERIOD_NTP_UPDATE;
  if (interval) lastNTPUpdate = millis();
  return interval;
}

bool isWifiConnected() {
  return (WiFi.status() == WL_CONNECTED) && (WiFi.localIP().toString() != "0.0.0.0");
}

void checkWifiDisconnect() {
  if (!wifiManager.getConfigPortalActive()) {
    if (WiFi.status() != WL_CONNECTED) {
      logger.log("no wifi connection..");
    } else if (WiFi.localIP().toString() == "0.0.0.0") {
      // detected bug: ESP8266 - WiFi status not changing
      // https://github.com/esp8266/Arduino/issues/2593#issuecomment-323801447
      logger.log("no IP address, reconnecting WiFi..");
      WiFi.reconnect();
    }
  }
}

void printIP() {
  logger.log("IP address: ");
  logger.log(WiFi.localIP().toString());
  uint8_t address = WiFi.localIP()[3];
  // ledmatrix.printChar(1, 0, 'I', maincolor_clock);
  // ledmatrix.printChar(5, 0, 'P', maincolor_clock);
  // if (address/100 != 0) {
  //   ledmatrix.printNumber(0, 6, (address/100), maincolor_clock);
  // }
  // ledmatrix.printNumber(4, 6, (address/10)%10, maincolor_clock);
  // ledmatrix.printNumber(8, 6, address%10, maincolor_clock);
  // ledmatrix.drawOnMatrixInstant();
  // delay(3000); // change to freeze matrix delay timer
}
void configModeCallback(WiFiManager* wm) {
  logger.log("Entered AP config mode");
  logger.log(WiFi.softAPIP().toString());

  logger.log(wm->getConfigPortalSSID());
}
void saveConfigCallback() {
  logger.log("Saved and connected to WiFi.");
  onWifiConnect();
}
void setupWifiManager() {
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setHostname(HOSTNAME);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
}

void onWifiConnect() {
  setupOTP();
  logger = UDPLogger(WiFi.localIP(), IPAddress(230, 120, 10, 2), 8123);
  printIP();
}

void setupOTP() {
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
}

void setupRTC() {
  // Start the I2C interface (needed for DS3231 clock)
  Wire.begin();
  rtc.setClockMode(false);
  if (readIntEEPROM(ADR_RTC) && rtc.getYear()) {
    logger.log("RTC already set, skipping set time");
  } else {
    if (readIntEEPROM(ADR_RTC)) {
      logger.log("WARNING: RTC lost power. Compile again or connect the WiFi to sync the time");
    }
    DateTime compiled = DateTime(__DATE__, __TIME__);
    logger.log("setting RTC to compile time");
    rtc.setYear(compiled.year() % 100);
    rtc.setMonth(compiled.month());
    rtc.setDate(compiled.day());
    rtc.setHour(compiled.hour());
    rtc.setMinute((compiled.minute() + 1) % 60);
    rtc.setSecond(0);
    writeIntEEPROM(ADR_RTC, 1);
  }
}

void reset() {
  logger.log("resetting WiFi settings");
  wifiManager.resetSettings();

  logger.log("empty entire EEPROM");
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

/**
 * @brief Write value to EEPROM
 *
 * @param address address to write the value
 * @param value value to write
 */
void writeIntEEPROM(int address, int value) {
  EEPROM.put(address, value);
  EEPROM.commit();
}

/**
 * @brief Read value from EEPROM
 *
 * @param address address
 * @return int value
 */
int readIntEEPROM(int address) {
  int value;
  EEPROM.get(address, value);
  return value;
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