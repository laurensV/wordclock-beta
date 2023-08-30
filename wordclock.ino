// standard libraries
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <time.h>
#include <coredecls.h>
#include <TZ.h>

// libraries from library manager
#include <DS3231.h> // https://github.com/NorthernWidget/DS3231
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

// own services
#include "src/udplogger.h"

// ----------------------------------------------------------------------------------
//                               WORDCLOCK LAYOUT
// ----------------------------------------------------------------------------------
char letters[13][13] = {
  {'H','E','T','V','I','S','N','E','E','N','Z','E','S'},
  {'T','W','E','E','D','R','I','E','Z','V','I','E','R'},
  {'V','I','J','F','Z','E','V','E','N','A','C','H','T'},
  {'N','E','G','E','N','-','T','I','E','N','E','L','F'},
  {'T','W','A','A','L','F','D','E','R','T','I','E','N'},
  {'V','E','E','R','T','I','E','N','K','W','A','R','T'},
  {'V','O','O','R','O','V','E','R','T','H','A','L','F'},
  {'E','E','N','X','T','W','E','E','N','D','R','I','E'},
  {'V','I','E','R','G','V','I','J','F','N','Z','E','S'},
  {'K','L','Z','E','V','E','N','M','A','C','H','T','R'},
  {'F','N','E','G','E','N','S','T','I','E','N','E','E'},
  {'T','W','A','A','L','F','I','T','O','T','V','*','*'},
  {'E','L','F','L','N','U','U','R','D','U','S','*','*'},
};

// ----------------------------------------------------------------------------------
//                                        CONSTANTS
// ----------------------------------------------------------------------------------
#define PERIOD_HEARTBEAT 1000UL
#define PERIOD_NTP_UPDATE 60 * 1000UL
#define AP_SSID "JouwWoordklok"
#define HOSTNAME "jouwwoordklok"
#define TIMEZONE TZ_Europe_Amsterdam

#define EEPROM_SIZE 4      // size of EEPROM to save persistent variables
#define ADR_RTC 0

// ----------------------------------------------------------------------------------
//                                    GLOBAL VARIABLES
// ----------------------------------------------------------------------------------
long lastHeartBeat = 0;

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

  setupTime();

  setupWifiManager();
  if (wifiManager.autoConnect(AP_SSID)) {
    log("Reonnected to WiFi");
    onWifiConnect();
  }
}

// ----------------------------------------------------------------------------------
//                                        LOOP
// ----------------------------------------------------------------------------------

void loop() {
  wifiManager.process();
  ArduinoOTA.handle();

  if (heartbeatInterval()) {
    log("heartbeat");
    int hours = rtc.getHour(h12, hPM);
    int minutes = rtc.getMinute();
    log(String(hours) + ":" + String(minutes));
    String message = timeToString(hours, minutes);
    log(message);
    checkWifiDisconnect();
  }
}

// ----------------------------------------------------------------------------------
//                                        OTHER FUNCTIONS
// ----------------------------------------------------------------------------------
void log(String logmessage) {
  logger.log(logmessage);
}
void log(uint32_t color) {
  logger.log(color);
}

bool heartbeatInterval() {
  bool interval = millis() - lastHeartBeat > PERIOD_HEARTBEAT;
  if (interval) lastHeartBeat = millis();
  return interval;
}

bool isWifiConnected() {
  return (WiFi.status() == WL_CONNECTED) && (WiFi.localIP().toString() != "0.0.0.0");
}

void checkWifiDisconnect() {
  if (!wifiManager.getConfigPortalActive()) {
    if (WiFi.status() != WL_CONNECTED) {
      log("no wifi connection..");
    } else if (WiFi.localIP().toString() == "0.0.0.0") {
      // detected bug: ESP8266 - WiFi status not changing
      // https://github.com/esp8266/Arduino/issues/2593#issuecomment-323801447
      log("no IP address, reconnecting WiFi..");
      WiFi.reconnect();
    }
  }
}

void printIP() {
  log("IP address: ");
  log(WiFi.localIP().toString());
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
  log("Entered AP config mode");
  log(WiFi.softAPIP().toString());

  log(wm->getConfigPortalSSID());
}
void saveConfigCallback() {
  log("Saved and connected to WiFi.");
  onWifiConnect();
}
void setupWifiManager() {
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setHostname(HOSTNAME);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
}

void onWifiConnect() {
  setupOTA();
  logger = UDPLogger(WiFi.localIP(), IPAddress(230, 120, 10, 2), 8123);
  printIP();
  configTime(TIMEZONE, "pool.ntp.org", "time.cloudflare.com", "time.google.com");
}

void setupOTA() {
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
}

void setupTime() {
  // Start the I2C interface (needed for DS3231 clock)
  Wire.begin();
  rtc.setClockMode(false);
  if (readIntEEPROM(ADR_RTC) && rtc.getYear()) {
    log("RTC already set, skipping set time");
  } else {
    if (readIntEEPROM(ADR_RTC)) {
      log("WARNING: RTC lost power. Compile again or connect the WiFi to sync the time");
    }
    DateTime compiled = DateTime(__DATE__, __TIME__);
    log("setting RTC to compile time + 33 seconds");
    rtc.setEpoch(compiled.unixtime() + 33);
    writeIntEEPROM(ADR_RTC, 1);
  }
  // callback for ntp
  settimeofday_cb(ntp_update);
}

void reset() {
  log("resetting WiFi settings");
  wifiManager.resetSettings();

  log("empty entire EEPROM");
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

// NTP functions
void ntp_update() {
  log("Time updated from NTP server!");
  rtc.setEpoch(time(nullptr), true); // set rtc to current local time
}
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
  return PERIOD_NTP_UPDATE;
}
uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000() {
  return 2000UL; // 2s, 60s limit is not a restriction anymore
}