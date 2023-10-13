#ifndef esp8266fota_h
#define esp8266fota_h

#include "Arduino.h"
#include "../wordclock.h"

class esp8266FOTA {
public:
  typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;
  typedef std::function<void(void)> THandlerFunction;
  esp8266FOTA(String firwmareType, int firwmareVersion, int filesystemVersion);
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
  int _filesystemVersion;
  bool _fsUpdate;
  String _host;
  String _bin;
  int _port;
};

#endif