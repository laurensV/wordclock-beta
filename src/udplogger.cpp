#include "udplogger.h"

UDPLogger::UDPLogger() {}

UDPLogger::UDPLogger(IPAddress interfaceAddr, IPAddress multicastAddr, int port) {
    _multicastAddr = multicastAddr;
    _port = port;
    _interfaceAddr = interfaceAddr;
    _Udp.beginMulticast(_interfaceAddr, _multicastAddr, _port);
}

void UDPLogger::log(String logmessage) {
    // wait 5 milliseconds if last send was less than 5 milliseconds before 
    if (millis() < (_lastSend + 5)) {
        delay(5);
    }
    if (_interfaceAddr) {
        _Udp.beginPacketMulticast(_multicastAddr, _port, _interfaceAddr);
        logmessage.toCharArray(_packetBuffer, 100);
        _Udp.print(_packetBuffer);
        _Udp.endPacket();
        _lastSend = millis();
    }
}
