#ifndef esp8266fota_h
#define esp8266fota_h

#include "Arduino.h"

class esp8266FOTA {
public:
  esp8266FOTA(String firwmareType, int firwmareVersion);
  bool forceUpdate(String firwmareHost, int firwmarePort, String firwmarePath);
  void execOTA();
  bool execHTTPcheck();
  bool useDeviceID;
  String checkURL;

private:
  String getHeaderValue(String header, String headerName);
  String getDeviceID();
  String _firwmareType;
  int _firwmareVersion;
  String _host;
  String _bin;
  int _port;
};

#endif