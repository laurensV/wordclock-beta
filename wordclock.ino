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
#include <Adafruit_NeoPixel.h> // https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_GFX.h> // https://github.com/adafruit/Adafruit_GFX
#include <Adafruit_NeoMatrix.h> // https://github.com/adafruit/Adafruit_NeoMatrix
#include <WiFiClientSecure.h>

// own services
#include "src/udplogger.h"
#include "src/esp8266fota.h"

// ----------------------------------------------------------------------------------
//                                        CONSTANTS
// ----------------------------------------------------------------------------------
#define PERIOD_READTIME 1000UL
#define PERIOD_WIFICHECK 20000UL
#define PERIOD_NTP_UPDATE 60 * 1000UL
#define AP_SSID "JouwWoordklok"
#define HOSTNAME "jouwwoordklok"
#define TIMEZONE TZ_Europe_Amsterdam

#define EEPROM_SIZE 4      // size of EEPROM to save persistent variables
#define ADR_RTC 0

#define CLOCK_WIDTH 13
#define CLOCK_HEIGHT 13
#define NUM_PIXELS (CLOCK_WIDTH * CLOCK_HEIGHT)

#define NEOPIXEL_PIN 0       // pin to which the NeoPixels are attached


// ----------------------------------------------------------------------------------
//                               WORDCLOCK LAYOUT
// ----------------------------------------------------------------------------------
const String letters = "\
HETVISNEENZES\
TWEEDRIEZVIER\
VIJFZEVENACHT\
NEGEN-TIENELF\
TWAALFDERTIEN\
VEERTIENKWART\
VOOROVERTHALF\
EENXTWEENDRIE\
VIERGVIJFNZES\
KLZEVENMACHTR\
FNEGENSTIENEE\
TWAALFITOTV**\
ELFLNUURDUS**";

// ----------------------------------------------------------------------------------
//                                    GLOBAL VARIABLES
// ----------------------------------------------------------------------------------
int VERSION = {
#include "VERSION.h"  
};


long lastReadTime = 0, lastWifiCheck = 0;

WiFiManager wifiManager;

UDPLogger logger;

DS3231 rtc;

esp8266FOTA FOTA("wordclock", VERSION);

Adafruit_NeoMatrix matrix(CLOCK_WIDTH, CLOCK_HEIGHT, NEOPIXEL_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB + NEO_KHZ800);

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


  setupTime();

  matrix.begin();
  matrix.clear();
  for (int i = 0; i < NUM_PIXELS; i++) {
    uint16_t hue = 0 + (i * 65536) / NUM_PIXELS;
    uint32_t color = Adafruit_NeoMatrix::ColorHSV(hue, 255, 255);
    color = matrix.gamma32(color);
    matrix.setPixelColor(i, color);
    matrix.show();
    delay(20);
  }

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

  if (readTimeInterval()) {
    time_t timeNowUTC;
    struct tm* timeInfo;
    timeNowUTC = (RTClib::now()).unixtime(); // time(nullptr);
    timeInfo = localtime(&timeNowUTC);
    int hours = timeInfo->tm_hour;
    int minutes = timeInfo->tm_min;
    log(String(hours) + ":" + String(minutes));
    String timeString = timeToString(hours, minutes);
    log(timeString);
    showTimeString(timeString);

  }
  if (checkWifiInterval()) {
    checkWifiDisconnect();
    if (isWifiConnected()) {
      log('checking for new updates..');
      FOTA.checkURL = "https://raw.githubusercontent.com/laurensV/wordclock/main/firmware/version.json";
      bool updatedNeeded = FOTA.execHTTPcheck();
      if (updatedNeeded) {
        log('New update available!');
        FOTA.execOTA();
      } else {
        log('Already running latest version');
      }
    }
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

void showTimeString(String timeString) {
  int messageStart = 0;
  String word = "";
  int lastLetterClock = 0;
  int positionOfWord = 0;
  int nextSpace = 0;
  int index = 0;

  // add space on the end of timeString for splitting
  timeString = timeString + " ";

  // empty the targetgrid
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
          int x = (positionOfWord + i) % CLOCK_WIDTH;
          int y = (positionOfWord + i) / CLOCK_WIDTH;
          matrix.drawPixel(x, y, matrix.Color(255, 0, 255));
        }
        lastLetterClock = positionOfWord + word.length();
      } else {
        // word is not possible to show on clock
        log("word is not possible to show on clock: " + String(word));
      }
    } else {
      // end - no more word in message
      break;
    }
  }
  matrix.show();
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

bool readTimeInterval() {
  bool interval = millis() - lastReadTime > PERIOD_READTIME;
  if (interval) lastReadTime = millis();
  return interval;
}

bool checkWifiInterval() {
  bool interval = millis() - lastWifiCheck > PERIOD_WIFICHECK;
  if (interval) lastWifiCheck = millis();
  return interval;
}

bool isWifiConnected() {
  return (WiFi.status() == WL_CONNECTED) && (WiFi.localIP().toString() != "0.0.0.0");
}

void checkWifiDisconnect() {
  if (!wifiManager.getConfigPortalActive()) {
    if (WiFi.status() != WL_CONNECTED) {
      log("no wifi connection..");
      matrix.drawPixel(3, 7, matrix.Color(255, 0, 0));
      matrix.show();
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
  // Set timezone
  setTZ(TIMEZONE);
  // Start the I2C interface (needed for DS3231 clock)
  Wire.begin();
  rtc.setClockMode(false);
  if ((readIntEEPROM(ADR_RTC) && rtc.getYear())) {
    log("RTC already set, skipping set time");
  } else {
    if (readIntEEPROM(ADR_RTC)) {
      log("WARNING: RTC lost power. Compile again or connect the WiFi to sync the time");
    }
    DateTime compiled = DateTime(__DATE__, __TIME__);
    time_t local_compile = compiled.unixtime();
    struct tm* tmptime;
    tmptime = localtime(&local_compile);
    tmptime->tm_isdst = 0;
    int32_t offset = mktime(tmptime) - mktime(gmtime(&local_compile));
    log("setting RTC to compile UTC time + 33 seconds");
    rtc.setEpoch(local_compile - offset + 33);
    writeIntEEPROM(ADR_RTC, 1);
  }

  // callback for set time updates
  settimeofday_cb(time_set);
  // set the system time to the RTC time
  // timeval tv = { (RTClib::now()).unixtime(), 0 };
  // settimeofday(&tv, nullptr);
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
void time_set(bool from_sntp) {
  if (from_sntp) {
    log("System time updated from NTP server");
    // Update RTC if time came from NTP server
    rtc.setEpoch(time(nullptr));
  } else {
    log("System time updated from RTC");
  }
}
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000() {
  return PERIOD_NTP_UPDATE;
}
uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000() {
  return 2000UL; // 2s, 60s limit is not a restriction anymore
}