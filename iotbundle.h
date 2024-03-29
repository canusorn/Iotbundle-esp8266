// bit command
// 10bit 0b1111111111  = pin A0,8,7,6,5,4,3,2,1,0  (pin 1,2 is i2c)   in/out = 1,readA0/0
// 9bit 0b111111111 = pin 8,7,6,5,4,3,2,1,0    HIGH/LOW = 1/0

#ifndef iotbundle_h
#define iotbundle_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecureBearSSL.h>

// #define IOTBUNDLE_DEBUG

#ifndef retryget_userid
#define retryget_userid 30
#endif

#ifndef SAMPLERATE
#define SAMPLERATE 30
#endif

#define VERSION "0.0.9" // for iotwebconfig -> don't change
#define IOTVERSION "0.1.0"

class Iotbundle
{

private:
  String _server;
  int8_t _project_id[5] = {-1, -1, -1, -1, -1};
  uint8_t activeProject = 0; // current activeProject to update var
  String _email;
  String _pass;
  String _esp_id;
  uint16_t _user_id;
  float var_sum[10][5];    // store sum variables
  uint8_t var_index[5];    // number of store
  float var_sum_3p[6][3];  // store sum variables in 3 phase
  uint8_t var_index_3p[3]; // number of store in 3 phase
  uint32_t _previousMillis;
  const uint8_t sendtime = SAMPLERATE;                 // delay time to send in second
  uint8_t _get_userid;                  // soft timer to retry login
  bool newio_s = false, newio_c = true; // flag new io from server,clients has change
  uint16_t io, previo;                  // current io output, previous io
  uint16_t _AllowIO = 0b111111111;      // pin to allow to write
  uint8_t _noConnect;                   // sample no connect to server
  String _json_update;                  // JSON update data
  String _login_url;
  String _update_url;
  bool daytimestamp_s = true, timer_c = false, timer_s = true; // today timestamp, timer from server updated , request timer from server
  uint32_t daytimestamp;                                       // today timestamp

  // pin
  uint8_t pin_mode[9];                     // 0-noactive 1-input 2-output 3-pwm
  bool pin_s = true, pin_c, pin_change;    // request pin from server,pin from server updated, new input pin change
  uint8_t value_pin[9], prev_value_pin[9]; // output pin state
  uint16_t pin_change_checksum = 0;        // checksum for pin state change in binary
  uint8_t wemosGPIO(uint8_t pin);          // get gpio number from 'D'pin

  // timer
  uint8_t timer_pin[10];
  uint32_t timer_start[10], timer_interval[10];
  bool timer_active[10];
  bool timer_state[10];

  // clear sum variables
  void login();

  // clear sum variables
  void clearvar();

  // set project id
  void setProjectID(String project, uint8_t array_project);

  // count project
  int8_t projectCount();

  // return String project
  void projectSort();

  // rest api method
  String getData(String data);
  String postData(String data, String url);

  // data is url
  String getHttp(String data);
  String getHttps(String data);

  // data is json
  String postHttp(String data, String url);
  String postHttps(String data, String url);

  // handle io from server
  void iohandle_s();
  void pinhandle_s(String pindata);

  // read io and update to server
  void readio();

  // data update function
  void updateProject();

  // parse json from payload
  void Stringparse(String payload);

  // parse timer from response
  void Timerparse(String timer);

  // timer handle
  void TimerHandle();

  // get project id form name
  uint8_t getProjectID(String project);

  // handle data custom'project
  void custom(uint8_t id = 0);

  // handle data acmeter'project
  void acMeter(uint8_t id = 0);

  // handle data pmmeter'project
  void pmMeter(uint8_t id = 0);

  // handle data dcmeter'project
  void dcMeter(uint8_t id = 0);

  // handle data DHT'project
  void DHT(uint8_t id = 0);

  // handle data smartFarmSolar'project
  void smartFarmSolar(uint8_t id = 0);

  // handle data acmeter 3 phase'project
  void acMeter_3p(uint8_t id = 0);

  // handle data battery'project
  void battery(uint8_t id);

public:
  Iotbundle(String project);

  // flag connect to server
  bool serverConnected;

  // error masage from server
  String noti;

  // version
  String version = IOTVERSION;

  // need ota update flag
  bool need_ota = false;

  // connect and login
  void begin(String email, String pass, String server = "https://iotkiddie.com");

  // send data to server
  void addProject(String project);

  // send data to server
  void handle();

  // interrupt to update timer every 1 sec
  void interrupt1sec();

  // set active project to update var
  void setProject(String projectname);

  // sumdata to cal average
  void update(float var1 = NAN, float var2 = NAN, float var3 = NAN, float var4 = NAN, float var5 = NAN, float var6 = NAN, float var7 = NAN, float var8 = NAN, float var9 = NAN, float var10 = NAN);
  void update(float v[3], float i[3], float p[3], float e[3], float f[3], float pf[3]);

  // get status
  bool status();

  // set allow io pin
  void setAllowIO(uint16_t allowio);

  // fouce update data before sleep
  void forceUpdate(bool settolowall = false);

  // ota update
  void otaUpdate(String optional_version = "", String url = "");

  // get today timestamp
  uint32_t getTodayTimestamp();
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