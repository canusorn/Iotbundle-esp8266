// bit command
// 10bit 0b1111111111  = pin A0,8,7,6,5,4,3,2,1,0  (pin 1,2 is i2c)   in/out = 1,readA0/0
// 9bit 0b111111111 = pin 8,7,6,5,4,3,2,1,0    HIGH/LOW = 1/0

#ifndef iotbundle_h
#define iotbundle_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#define IOTBUNDLE_DEBUG
#define retryget_userid 30
#define VERSION "0.0.5"

class Iotbundle
{

private:
  String _server;
  uint8_t _project_id;
  String _email;
  String _pass;
  String _esp_id;
  uint16_t _user_id;
  float var_sum[10]; // store sum variables
  uint8_t var_index; // number of store file
  uint32_t _previousMillis;
  uint8_t sendtime = 5;                 // delay time to send in second
  uint8_t _get_userid;                  // soft timer to retry login
  bool newio_s = false, newio_c = true; // flag new io from server,clients has change
  uint16_t io, previo;                  // current io output, previous io
  uint16_t _AllowIO;                    // pin to allow to write

  // clear sum variables
  void clearvar();

  // rest api method
  String getData(String data);
  String postData(String data);

  // data is url
  String getHttp(String data);
  String getHttps(String data);

  // data is json
  String postHttp(String data);
  String postHttps(String data);

  // handle io from server
  void iohandle_s();

  // read io and update to server
  void readio();

  // set input and low for allow pin
  void init_io();

  // parse json from payload
  int16_t Stringparse(String payload);

  // handle data acmeter'project
  void acMeter();

  // handle data pmmeter'project
  void pmMeter();

  // handle data DHT'project
  void DHT();

  // handle data smartFarmSolar'project
  void smartFarmSolar();

public:
  Iotbundle(String project);

  // flag connect to server
  bool serverConnected;

  // error masage from server
  String noti;

  // version
  String version = VERSION;

  // connect and login
  void begin(String email, String pass, String server = "https://iotkiddie.com");

  // send data to server
  void handle();

  // sumdata to cal average
  void update(float var1 = NAN, float var2 = NAN, float var3 = NAN, float var4 = NAN, float var5 = NAN, float var6 = NAN, float var7 = NAN, float var8 = NAN, float var9 = NAN, float var10 = NAN);

  // get status
  bool status();

  // set allow io pin
  void setAllowIO(uint16_t allowio);

  // fouce update data
  void fouceUpdate(bool settolowall = false);
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