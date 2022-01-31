#ifndef acmeter_h
#define acmeter_h

#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#define ACMERTER_DEBUG

class Acmeter
{

private:
    bool sentData();
    String _server;
    uint16_t _user_id;
    uint8_t _project_id;
    String _esp_id;

public:
    Acmeter(String server,uint16_t user_id,uint8_t project_id,String esp_id);

    bool update(float v, float i, float p, float e, float f, float pf);

    float voltage = NAN;
    float current = NAN;
    float power = NAN;
    float energy = NAN;
    float frequency = NAN;
    float pf = NAN;
    bool serverConnected;
};

// for set debug mode
#ifdef ACMERTER_DEBUG

#ifndef ACMERTER_DEBUG_SERIAL
#define ACMERTER_DEBUG_SERIAL Serial
#endif

#define DEBUG(...) ACMERTER_DEBUG_SERIAL.print(__VA_ARGS__)
#define DEBUGLN(...) ACMERTER_DEBUG_SERIAL.println(__VA_ARGS__)

#else
// Debugging mode off, disable the macros
#define DEBUG(...)
#define DEBUGLN(...)
#endif

#endif