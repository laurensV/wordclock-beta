#include "esp8266fota.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "ArduinoJson.h"

esp8266FOTA::esp8266FOTA(String firwmareType, int firwmareVersion) {
    _firwmareType = firwmareType;
    _firwmareVersion = firwmareVersion;
    useDeviceID = false;
}

// Utility to extract header value from headers
String esp8266FOTA::getHeaderValue(String header, String headerName) {
    return header.substring(strlen(headerName.c_str()));
}

void esp8266FOTA::onProgress(THandlerFunction_Progress fn) {
    Update.onProgress(fn);
}
void esp8266FOTA::onEnd(THandlerFunction fn) {
    _end_callback = fn;
}

// OTA Logic
void esp8266FOTA::execOTA() {
    WiFiClientSecure client;
    client.setInsecure();
    int contentLength = 0;
    bool isValidContentType = false;

    print("Connecting to: " + String(_host));
    // Connect to Webserver
    if (client.connect(_host.c_str(), _port)) {
        // Connection Succeed.
        // Fecthing the bin
        print("Fetching Bin: " + String(_bin));

        // Get the contents of the bin file
        client.print(String("GET ") + _bin + " HTTP/1.1\r\n" +
            "Host: " + _host + "\r\n" +
            "Cache-Control: no-cache\r\n" +
            "Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (client.available() == 0) {
            if (millis() - timeout > 5000) {
                print("Client Timeout !");
                client.stop();
                return;
            }
        }

        while (client.available()) {
            // read line till /n
            String line = client.readStringUntil('\n');
            // remove space, to check if the line is end of headers
            line.trim();

            if (!line.length()) {
                //headers ended
                break; // and get the OTA started
            }

            // Check if the HTTP Response is 200
            // else break and Exit Update
            if (line.startsWith("HTTP/1.1")) {
                if (line.indexOf("200") < 0) {
                    print("Got a non 200 status code from server. Exiting OTA Update.");
                    break;
                }
            }

            // extract headers here
            // Start with content length
            if (line.startsWith("Content-Length: ")) {
                contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
                print("Got " + String(contentLength) + " bytes from server");
            }

            // Next, the content type
            if (line.startsWith("Content-Type: ")) {
                String contentType = getHeaderValue(line, "Content-Type: ");
                print("Got " + contentType + " payload.");
                if (contentType == "application/octet-stream") {
                    isValidContentType = true;
                }
            }
        }
    } else {
        // Connect to webserver failed
        print("Connection to " + String(_host) + " failed. Please check your setup");
    }

    // Check what is the contentLength and if content type is `application/octet-stream`
    print("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

    // check contentLength and content type
    if (contentLength && isValidContentType) {
        // Check if there is enough to OTA Update
        bool canBegin = Update.begin(contentLength);

        // If yes, begin
        if (canBegin) {
            print("Begin OTA Update, this may take a couple of minutes..");
            // No activity would appear on the Serial monitor
            // So be patient. This may take 2 - 5mins to complete
            size_t written = Update.writeStream(client);

            if (written == contentLength) {
                print("Written : " + String(written) + " successfully");
            } else {
                print("Written only : " + String(written) + "/" + String(contentLength));
            }

            if (Update.end()) {
                print("OTA done!");
                if (Update.isFinished()) {
                    if (_end_callback) {
                        _end_callback();
                    }
                    print("Update successfully completed. Rebooting.");
                    ESP.restart();
                } else {
                    print("Update not finished? Something went wrong!");
                }
            } else {
                print("Error Occurred. Error #: " + String(Update.getError()));
            }
        } else {
            // not enough space to begin OTA
            // Understand the partitions and
            // space availability
            print("Not enough space to begin OTA");
            client.flush();
        }
    } else {
        print("There was no content in the response");
        client.flush();
    }
}

bool esp8266FOTA::execHTTPcheck() {
    String useURL;
    if (useDeviceID) {
        useURL = checkURL + "?id=" + getDeviceID();
    } else {
        useURL = checkURL;
    }

    WiFiClientSecure client;
    client.setInsecure();
    _port = 443;

    print("Getting HTTP");
    print(useURL);
    print("------");
    if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

        HTTPClient http;

        http.begin(client, useURL);        //Specify the URL
        int httpCode = http.GET(); //Make the request

        if (httpCode == 200) { //Check is a file was returned

            String payload = http.getString();

            int str_len = payload.length() + 1;
            char JSONMessage[str_len];
            payload.toCharArray(JSONMessage, str_len);

            StaticJsonDocument<300> JSONDocument; //Memory pool
            DeserializationError err = deserializeJson(JSONDocument, JSONMessage);

            if (err) { //Check for errors in parsing
                print("Parsing failed");
                return false;
            }

            const char* pltype = JSONDocument["type"];
            int plversion = JSONDocument["version"];
            const char* plhost = JSONDocument["host"];
            _port = JSONDocument["port"];
            const char* plbin = JSONDocument["firmware"];

            String jshost(plhost);
            String jsbin(plbin);

            _host = jshost;
            _bin = jsbin;

            String fwtype(pltype);
            print(_firwmareVersion);
            print(plversion);
            Serial.println(plversion);
            if (plversion > _firwmareVersion && fwtype == _firwmareType) {
                return true;
            } else {
                return false;
            }
        } else {
            print("Error on HTTP request");
            return false;
        }

        http.end(); //Free the resources
        return false;
    }
    return false;
}

String esp8266FOTA::getDeviceID() {
    char deviceid[21];
    uint64_t chipid;

    chipid = ESP.getChipId();

    sprintf(deviceid, "%" PRIu64, chipid);
    String thisID(deviceid);
    return thisID;
}

// Force a firmware update regardless on current version
bool esp8266FOTA::forceUpdate(String firwmareHost, int firwmarePort, String firwmarePath) {
    _host = firwmareHost;
    _bin = firwmarePath;
    _port = firwmarePort;
    execOTA();

    return true;
}
