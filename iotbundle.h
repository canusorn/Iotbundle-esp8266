

#ifndef iotbundle_h
#define iotbundle_h

#include "Arduino.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "project/acmeter.h"

#define IOTBUNDLE_DEBUG
#define retryget_userid 60
// for select server
//#ifdef IOTKID
//
//#elif defined(IOTKIDDIE)
//
//#else
//#define IOTKID
//#endif

// #ifdef PROJECT
// #if PROJECT=="AC_METER"
// #include "project/acmeter.h"
// #endif
// #endif

class Iotbundle
{

private:
  String _server;
  uint8_t _project_id;
  String _email;
  String _pass;
  String _esp_id;
  uint16_t _user_id;
  uint32_t _previousMillis;
  uint8_t _get_userid;

  void clearvar();

public:
  Iotbundle(String server, String project);

  bool serverConnected;
  uint8_t sendtime = 2; // delay time to send in second
  float var_sum[10];    // store sum variables
  uint8_t var_index;

  //connect and login
  void init(String email, String pass);

  // send data to server
  void handle();
  void update(float var1 = NAN, float var2 = NAN, float var3 = NAN, float var4 = NAN, float var5 = NAN, float var6 = NAN, float var7 = NAN, float var8 = NAN, float var9 = NAN, float var10 = NAN);
};

// for set debug mode
#ifdef IOTBUNDLE_DEBUG

#ifndef IOTBUNDLE_DEBUG_SERIAL
#define IOTBUNDLE_DEBUG_SERIAL Serial
#endif

#define DEBUG(...) IOTBUNDLE_DEBUG_SERIAL.print(__VA_ARGS__)
#define DEBUGLN(...) IOTBUNDLE_DEBUG_SERIAL.println(__VA_ARGS__)

#else
// Debugging mode off, disable the macros
#define DEBUG(...)
#define DEBUGLN(...)

#endif

#endif
