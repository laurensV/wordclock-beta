#ifndef udplogger_h
#define udplogger_h

#include <Arduino.h>
#include <WiFiUdp.h>

class UDPLogger {

public:
    UDPLogger();
    UDPLogger(IPAddress interfaceAddr, IPAddress multicastAddr, int port);
    void log(String logmessage);
private:
    IPAddress _multicastAddr;
    IPAddress _interfaceAddr;
    int _port;
    WiFiUDP _Udp;
    char _packetBuffer[100];
    long _lastSend;
};

#endif