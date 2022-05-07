#include "iotbundle.h"

Iotbundle::Iotbundle(String project)
{
  // set project id
  setProjectID(project, 0);

  // set all allow pin to low
  init_io();

  // get this esp id
  this->_esp_id = String(ESP.getChipId());
}

void Iotbundle::setProjectID(String project, uint8_t array_project)
{
  // set project id
  if (project == "AC_METER")
  {
    this->_project_id[array_project] = 1;
    _AllowIO &= 0b111100001;
  } // pin4,3=pzem  2,1=i2c
  else if (project == "PM_METER")
  {
    this->_project_id[array_project] = 2;
    _AllowIO &= 0b111100001;
  }
  else if (project == "DC_METER")
  {
    this->_project_id[array_project] = 3;
  }
  else if (project == "DHT")
  {
    this->_project_id[array_project] = 4;
    _AllowIO &= 0b101111001;
  }
  else if (project == "smartfarm_solar")
  {
    this->_project_id[array_project] = 5;
    _AllowIO &= 0b101100110;
  }
}

void Iotbundle::begin(String email, String pass, String server)
{

  // set server
  this->_server = server;
  if (this->_server == "")
    this->_server = "https://iotkiddie.com";

  // delete spacebar from email
  String _temp_email = email;
  uint8_t e = _temp_email.length();
  this->_email = "";
  for (int i = 0; i < e; i++)
  {
    if (_temp_email[i] != ' ')
    {
      this->_email += _temp_email[i];
    }
  }

  this->_pass = pass;

  DEBUGLN("Begin -> email:" + this->_email + " server:" + this->_server);

  String url = this->_server + "/api/connect.php";
  url += "?email=" + this->_email;
  url += "&pass=" + this->_pass;
  url += "&esp_id=" + this->_esp_id;
  url += "&project_id=" + String(this->_project_id[0]);

  // add in v0.0.5
  String v_int = "";
  uint8_t count = version.length();
  for (int i = 0; i < count; i++)
  {
    if (version[i] != '.')
    {
      v_int += version[i];
    }
  }
  url += "&version=" + v_int;
  // DEBUGLN(url);

  String payload = getData(url);

  if (payload.toInt() > 0)
  {
    this->serverConnected = true;
    _user_id = payload.toInt();
    DEBUGLN("get user_id : " + String(_user_id));
    this->noti = "-Login-\nlogin success";
  }
  else
  {
    this->serverConnected = false;
    this->noti = "-!Login-\n" + payload;
    DEBUGLN(payload);
  }
}

void Iotbundle::addProject(String project)
{
  uint8_t projectarray = projectCount();
  setProjectID(project, projectarray);

  // show all project
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) < 0)
    {
      return;
    }
    DEBUGLN("project " + String(i + 1) + " : " + String(_project_id[i]));
  }
}

void Iotbundle::handle()
{
  uint32_t currentMillis = millis();
  if (currentMillis - _previousMillis >= sendtime * 1000)
  {
    _previousMillis = currentMillis;
    if (this->_email && this->_server != "")
    {
      if (_user_id > 0)
      {
        updateProject();
      }
      else
      {
        _get_userid++;
        if (_get_userid >= retryget_userid / sendtime)
        {
          _get_userid = 0;

          DEBUGLN("retry login");
          begin(this->_email, this->_pass, this->_server);
        }
      }
    }
  }
}

void Iotbundle::updateProject()
{
  if (_json_update == "")
  {
    _json_update = "{\"esp_id\":" + _esp_id + ",";
    _json_update += "\"user_id\":" + String(_user_id) + ",";
    _json_update += "\"count\":1,";
    _json_update += "\"data\":[";
  }
  if (_project_id[0] == 1)
  {
    DEBUGLN("sending data to server");
    if (!newio_s)
      readio();
    acMeter();
  }
  else if (_project_id[0] == 2)
  {
    DEBUGLN("sending data to server");
    if (!newio_s)
      readio();
    pmMeter();
  }
  else if (_project_id[0] == 4)
  {
    DEBUGLN("sending data to server");
    if (!newio_s)
      readio();
    DHT();
  }
  else if (_project_id[0] == 5)
  {
    DEBUGLN("sending data to server");
    if (!newio_s)
      readio();
    smartFarmSolar();
  }
}

int8_t Iotbundle::projectCount()
{
  for (uint8_t i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) < 0)
    {
      DEBUGLN(i);
      return i;
    }
  }
  return -1;
}

void Iotbundle::fouceUpdate(bool settolowall)
{
  if (this->_email && this->_server != "")
  {
    if (_user_id > 0)
    {

      if (settolowall)
      { // set all pin to low for poweroff or sleep
        io = 0b000000000;
        newio_c = true;
      }

      if (_project_id[0] == 1)
      {
        DEBUGLN("fouce sending data to server");
        acMeter();
      }
      else if (_project_id[0] == 2)
      {
        DEBUGLN("fouce sending data to server");
        pmMeter();
      }
      else if (_project_id[0] == 4)
      {
        DEBUGLN("fouce sending data to server");
        DHT();
      }
      else if (_project_id[0] == 5)
      {
        DEBUGLN("fouce sending data to server");
        smartFarmSolar();
      }
    }
  }
}

void Iotbundle::update(float var1, float var2, float var3, float var4, float var5, float var6, float var7, float var8, float var9, float var10)
{
  if (!isnan(var1) || !isnan(var2) || !isnan(var3) || !isnan(var4) || !isnan(var5) || !isnan(var6) || !isnan(var7) || !isnan(var8) || !isnan(var9) || !isnan(var10)) // not update if all nan
  {
    var_index++;
    var_sum[0] += var1;
    var_sum[1] += var2;
    var_sum[2] += var3;
    var_sum[3] += var4;
    var_sum[4] += var5;
    var_sum[5] += var6;
    var_sum[6] += var7;
    var_sum[7] += var8;
    var_sum[8] += var9;
    var_sum[9] += var10;

    DEBUG("updated data " + String(var_index) + " -> ");
    for (int i = 0; i < sizeof(var_sum) / sizeof(var_sum[0]); i++)
    {
      DEBUG(String(var_sum[i]) + ", ");
    }
    DEBUGLN();
    DEBUGLN("FreeHeap : " + String(ESP.getFreeHeap()));
  }
}

void Iotbundle::clearvar()
{
  for (int i = 0; i < sizeof(var_sum) / sizeof(var_sum[0]); i++)
  {
    var_sum[i] = 0;
  }
  var_index = 0;
}

String Iotbundle::getData(String data)
{
  String payload;
  if (_server[4] == 's')
    return getHttps(data);
  else
    return getHttp(data);
}

String Iotbundle::postData(String data)
{
  String payload;
  if (_server[4] == 's')
    return postHttps(data);
  else
    return postHttp(data);
}

String Iotbundle::getHttp(String data)
{

  String payload;

  WiFiClient client;

  HTTPClient http;

  DEBUG("[HTTP] begin...\n");

  DEBUGLN(data);

  if (http.begin(client, data))
  { // HTTP

    // start timer
    uint32_t startGet = millis();

    DEBUG("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      DEBUGLN("[HTTPS] GET... code: " + String(httpCode));

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        payload = http.getString();
        DEBUGLN(payload);
        serverConnected = true;
      }
      else
      {
        payload = "code " + String(httpCode);
        serverConnected = false;
        DEBUGLN(http.getString());
      }
    }
    else
    {
      DEBUGLN("[HTTP] GET... failed, error:" + http.errorToString(httpCode));
      serverConnected = false;
    }

    http.end();

    // end timer and show update time
    uint32_t endGet = millis();
    DEBUGLN("update time : " + String(endGet - startGet) + " ms");
    DEBUGLN();
  }
  else
  {
    DEBUG("[HTTP} Unable to connect\n");
    serverConnected = false;
  }
  return payload;
}

String Iotbundle::getHttps(String data)
{
  String payload;

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  // const char *headerNames[] = {"Location"};
  // https.collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));

  DEBUG("[HTTPS] begin...\n");

  DEBUGLN(data);

  if (https.begin(*client, data))
  {

    // start timer
    uint32_t startGet = millis();

    DEBUG("[HTTPS] GET...\n");
    // start connection and send HTTP header
    int httpCode = https.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      DEBUGLN("[HTTPS] GET... code: " + String(httpCode));

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        payload = https.getString();
        DEBUGLN(payload);
        serverConnected = true;
      }
      else
      {
        payload = "code " + String(httpCode);
        serverConnected = false;
        DEBUGLN(https.getString());
      }
    }
    else
    {
      DEBUGLN("[HTTPS] GET... failed, error: " + https.errorToString(httpCode));
      serverConnected = false;
    }

    https.end();

    // end timer and show update time
    uint32_t endGet = millis();
    DEBUGLN("update time : " + String(endGet - startGet) + " ms");
    DEBUGLN();
  }
  else
  {
    DEBUGLN("[HTTPS] Unable to connect");
    serverConnected = false;
  }
  // }

  return payload;
}

String Iotbundle::postHttp(String data)
{
  String payload;
  WiFiClient client;
  HTTPClient http;

  String url = this->_server + "/api/v6/update.php";

  DEBUG("[HTTP] begin...\n" + url);
  // configure traged server and url
  if (http.begin(client, url))
  { // HTTP

    // start timer
    uint32_t startGet = millis();

    http.addHeader("Content-Type", "application/json");

    DEBUGLN("\n[HTTP] POST...\n" + data);
    // start connection and send HTTP header and body
    int httpCode = http.POST(data);

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      DEBUGLN("[HTTP] POST... code: " + String(httpCode));

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        payload = http.getString();
        DEBUGLN(payload);
        serverConnected = true;
      }
      else
      {
        payload = "code " + String(httpCode);
        serverConnected = false;
        DEBUGLN(http.getString());
      }
    }
    else
    {
      DEBUG("[HTTP] POST... failed, error: " + http.errorToString(httpCode));
      serverConnected = false;
    }

    http.end();

    // end timer and show update time
    uint32_t endGet = millis();
    DEBUGLN("update time : " + String(endGet - startGet) + " ms");
    DEBUGLN();
  }
  else
  {
    DEBUG("[HTTP} Unable to connect\n");
    serverConnected = false;
  }
  return payload;
}

String Iotbundle::postHttps(String data)
{
  String payload;

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  String url = this->_server + "/api/v6/update.php";

  DEBUGLN("[HTTPS] begin...\n" + url);
  // configure traged server and url
  if (https.begin(*client, url))
  { // HTTP

    // start timer
    uint32_t startGet = millis();

    https.addHeader("Content-Type", "application/json");

    DEBUGLN("[HTTPS] POST...\n" + data);
    // start connection and send HTTP header and body
    int httpCode = https.POST(data);

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      DEBUGLN("[HTTPS] POST... code: " + String(httpCode));

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        payload = https.getString();
        DEBUGLN(payload);
        serverConnected = true;
      }
      else
      {
        payload = "code " + String(httpCode);
        serverConnected = false;
        DEBUGLN(https.getString());
      }
    }
    else
    {
      DEBUG("[HTTP] POST... failed, error: " + https.errorToString(httpCode));
      serverConnected = false;
    }

    https.end();

    // end timer and show update time
    uint32_t endGet = millis();
    DEBUGLN("update time : " + String(endGet - startGet) + " ms");
    DEBUGLN();
  }
  else
  {
    DEBUG("[HTTPS] Unable to connect\n");
    serverConnected = false;
  }
  return payload;
}

bool Iotbundle::status()
{
  return serverConnected;
}

void Iotbundle::iohandle_s()
{ // handle io from server
  DEBUGLN("io:" + String(io, BIN));
  uint8_t wemosGPIO[] = {16, 5, 4, 0, 2, 14, 12, 13, 15}; // GPIO from d0 d1 d2 ... d8
  uint16_t useio = io ^ previo;                           // change only difference io
  DEBUG("writing io -> ");
  for (int i = 0; i < 9; i++)
  {
    if (bitRead(_AllowIO, i) && bitRead(useio, i))
    { // use only allow pin
      if (bitRead(io, i))
      {
        pinMode(wemosGPIO[i], OUTPUT);
        digitalWrite(wemosGPIO[i], HIGH);
        DEBUG("io:" + String(i) + "=HIGH, ");
      }
      else
      {
        pinMode(wemosGPIO[i], OUTPUT);
        digitalWrite(wemosGPIO[i], LOW);
        DEBUG("io:" + String(i) + "=LOW, ");
      }
    }
  }
  previo = io;
  DEBUGLN();
}

void Iotbundle::readio()
{
  uint8_t wemosGPIO[] = {16, 5, 4, 0, 2, 14, 12, 13, 15}; // GPIO from d0 d1 d2 ... d8
  // uint16_t useio = _AllowIO;
  uint16_t currentio;
  // pinMode(D5, INPUT);
  DEBUG("reading io -> ");
  for (int i = 0; i < 9; i++)
  {

    if (bitRead(_AllowIO, i))
    { // use only allow pin
      bitWrite(currentio, i, digitalRead(wemosGPIO[i]));
      DEBUG("io:" + String(i) + "=" + (String)digitalRead(wemosGPIO[i]) + ", ");
    }
  }
  DEBUGLN();
  DEBUGLN("io:" + String(io, BIN) + " currentio:" + String(currentio, BIN));

  if (io != currentio)
  {
    DEBUGLN("newio:" + String(currentio, BIN));
    newio_c = true;
    io = currentio;
  }
}

void Iotbundle::setAllowIO(uint16_t allowio)
{
  this->_AllowIO = allowio;
  init_io();
}

void Iotbundle::init_io()
{
  uint8_t wemosGPIO[] = {16, 5, 4, 0, 2, 14, 12, 13, 15}; // GPIO from d0 d1 d2 ... d8
  for (int i = 0; i < 9; i++)
  {
    if (bitRead(_AllowIO, i))
    { // use only allow pin
      // pinMode(wemosGPIO[i], OUTPUT);
      digitalWrite(wemosGPIO[i], LOW);
    }
  }
  DEBUGLN();
}

int16_t Iotbundle::Stringparse(String payload)
{
  int str_len = payload.length() + 1;
  char buff[str_len];
  payload.toCharArray(buff, str_len);

  int j = 0;
  String user_id_r_str, io_s_str;

  for (int i = 0; i < str_len; i++)
  {
    if (j == 0)
      user_id_r_str += buff[i];
    else if (j == 1)
      io_s_str += buff[i];

    if (buff[i] == '&')
      j++;
  }

  if (user_id_r_str.toInt() == _user_id) // check correct data
    return io_s_str.toInt();
  else if (user_id_r_str.toInt() == 0)
  {
    DEBUGLN("!error:" + io_s_str);
    return -1;
  }
  else
    return user_id_r_str.toInt();
}

void Iotbundle::acMeter()
{
  // calculate
  float v = var_sum[0] / var_index;
  float i = var_sum[1] / var_index;
  float p = var_sum[2] / var_index;
  float e = var_sum[3] / var_index;
  float f = var_sum[4] / var_index;
  float pf = var_sum[5] / var_index;

  // create string
  // String url = this->_server + "/api/";
  // url += String(_project_id[0]);
  // url += "/update.php";
  // url += "?user_id=" + String(_user_id);
  // url += "&esp_id=" + _esp_id;

  String data = _json_update;
  data += "{\"project_id\":" + String(_project_id[0]);

  if (var_index)
  { // validate
    if (v >= 60 && v <= 260 && !isnan(v))
      data += ",\"voltage\":" + String(v, 1);
    if (i >= 0 && i <= 100 && !isnan(i))
      data += ",\"current\":" + String(i, 3);
    if (p >= 0 && p <= 24000 && !isnan(p))
      data += ",\"power\":" + String(p, 1);
    if (e >= 0 && e <= 10000 && !isnan(e))
      data += ",\"energy\":" + String(e, 3);
    if (f >= 40 && f <= 70 && !isnan(f))
      data += ",\"frequency\":" + String(f, 1);
    if (pf >= 0 && pf <= 1 && !isnan(pf))
      data += ",\"pf\":" + String(pf, 2);
  }
  data += "}]";

  if (newio_c)
    data += ",\"io_c\":" + String(io);
  else if (newio_s)
    data += ",\"io_s\":" + String(io);

  data += "}";

  // String payload = getData(data);
  String payload = postData(data);

  if (payload != "")
  {
    int16_t newio = Stringparse(payload);
    if (newio == 32767) // io from server updated
    {
      newio_s = false;
    }
    else if (newio == 32766) // io from client updated
    {
      newio_s = false;
      newio_c = false;
    }
    else if (newio >= 0)
    {
      io = newio;
      iohandle_s();
      newio_s = true;
    }
  }

  if (serverConnected)
    clearvar();
}

void Iotbundle::pmMeter()
{
  // calculate
  uint16_t pm1 = var_sum[0] / var_index;
  uint16_t pm2 = var_sum[1] / var_index;
  uint16_t pm10 = var_sum[2] / var_index;

  // create string
  String url = this->_server + "/api/";
  url += String(_project_id[0]);
  url += "/update.php";
  url += "?user_id=" + String(_user_id);
  url += "&esp_id=" + _esp_id;
  if (var_index)
  { // validate
    if (pm1 >= 0 && pm1 <= 1999 && !isnan(pm1))
      url += "&pm1=" + String(pm1);
    if (pm2 >= 0 && pm2 <= 1999 && !isnan(pm2))
      url += "&pm2=" + String(pm2);
    if (pm10 >= 0 && pm10 <= 1999 && !isnan(pm10))
      url += "&pm10=" + String(pm10);
  }
  if (newio_c)
    url += "&io_c=" + String(io);
  else if (newio_s)
    url += "&io_s=" + String(io);

  String payload = getData(url);

  if (payload != "")
  {
    int16_t newio = Stringparse(payload);
    if (newio == 32767) // io from server updated
    {
      newio_s = false;
    }
    else if (newio == 32766) // io from client updated
    {
      newio_s = false;
      newio_c = false;
    }
    else if (newio >= 0)
    {
      io = newio;
      iohandle_s();
      newio_s = true;
    }
  }

  if (serverConnected)
    clearvar();
}

void Iotbundle::DHT()
{
  // calculate
  float humid = var_sum[0] / var_index;
  float temp = var_sum[1] / var_index;

  // create string
  String url = this->_server + "/api/";
  url += String(_project_id[0]);
  url += "/update.php";
  url += "?user_id=" + String(_user_id);
  url += "&esp_id=" + _esp_id;
  if (var_index)
  { // validate
    if (humid >= 0 && humid <= 100 && !isnan(humid))
      url += "&humid=" + String(humid, 1);
    if (temp >= -40 && temp <= 80 && !isnan(temp))
      url += "&temp=" + String(temp, 1);
  }
  if (newio_c)
    url += "&io_c=" + String(io);
  else if (newio_s)
    url += "&io_s=" + String(io);

  String payload = getData(url);

  if (payload != "")
  {
    int16_t newio = Stringparse(payload);
    if (newio == 32767) // io from server updated
    {
      newio_s = false;
    }
    else if (newio == 32766) // io from client updated
    {
      newio_s = false;
      newio_c = false;
    }
    else if (newio >= 0)
    {
      io = newio;
      iohandle_s();
      newio_s = true;
    }
  }

  if (serverConnected)
    clearvar();
}

void Iotbundle::smartFarmSolar()
{
  // calculate
  float humid = var_sum[0] / var_index;
  float temp = var_sum[1] / var_index;
  uint16_t vbatt = var_sum[2] / var_index;

  // create string
  String url = this->_server + "/api/";
  url += String(_project_id[0]);
  url += "/update.php";
  url += "?user_id=" + String(_user_id);
  url += "&esp_id=" + _esp_id;
  if (var_index)
  { // validate
    if (humid > 0 && humid <= 100 && !isnan(humid))
      url += "&humid=" + String(humid, 1);
    if (temp > 0 && temp <= 80 && !isnan(temp))
      url += "&temp=" + String(temp, 1);
    if (vbatt >= 2000 && vbatt <= 8000 && !isnan(vbatt))
      url += "&vbatt=" + String(vbatt);
  }
  url += "&valve=";
  url += (digitalRead(D1)) ? "1" : "0";
  if (newio_c)
    url += "&io_c=" + String(io);
  else if (newio_s)
    url += "&io_s=" + String(io);

  String payload = getData(url);

  if (payload != "")
  {
    int16_t newio = Stringparse(payload);
    if (newio == 32767) // io from server updated
    {
      newio_s = false;
    }
    else if (newio == 32766) // io from client updated
    {
      newio_s = false;
      newio_c = false;
    }
    else if (newio >= 0)
    {
      io = newio;
      iohandle_s();
      newio_s = true;
    }
  }

  if (serverConnected)
    clearvar();
}