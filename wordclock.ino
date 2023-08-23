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

// ----------------------------------------------------------------------------------
//                                    GLOBAL VARIABLES
// ----------------------------------------------------------------------------------
long lastHeartBeat = millis();
long lastNTPUpdate = millis();

WiFiManager wifiManager;
UDPLogger logger;

// ----------------------------------------------------------------------------------
//                                        SETUP
// ----------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\nSketchname: %s\nBuild: %s\n", (__FILE__), (__TIMESTAMP__));
  logger.log("before WiFi");
  // Uncomment and run it once, if you want to erase all the stored wifi information
  // wifiManager.resetSettings();

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
  logger.log(WiFi.softAPIP());

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