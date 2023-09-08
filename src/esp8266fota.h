#ifndef esp8266fota_h
#define esp8266fota_h

#include "Arduino.h"

class esp8266FOTA {
public:
  typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;
  typedef std::function<void(void)> THandlerFunction;
  esp8266FOTA(String firwmareType, int firwmareVersion);
  bool forceUpdate(String firwmareHost, int firwmarePort, String firwmarePath);
  void execOTA();
  bool execHTTPcheck();
  bool useDeviceID;
  String checkURL;
  //This callback will be called when OTA is receiving data
  void onProgress(THandlerFunction_Progress fn);
  //This callback will be called when OTA has finished
  void onEnd(THandlerFunction fn);
  THandlerFunction _end_callback = nullptr;


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