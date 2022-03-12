#include "iotbundle.h"

Iotbundle::Iotbundle(String project)
{
  // set project id
  if (project == "AC_METER")
  {
    this->_project_id = 1;
    _AllowIO = 0b111100001;
  } // pin4,3=pzem  2,1=i2c
  else if (project == "PM_METER")
  {
    this->_project_id = 2;
    _AllowIO = 0b111100001;
  }
  else if (project == "DC_METER")
  {
    this->_project_id = 3;
  }
  else if (project == "DHT")
  {
    this->_project_id = 4;
    _AllowIO = 0b101111001;
  }
  else if (project == "smartfarm_solar")
  {
    this->_project_id = 5;
    _AllowIO = 0b101100110;
  }

  // set all allow pin to low
  init_io();

  // get this esp id
  this->_esp_id = String(ESP.getChipId());
}

void Iotbundle::begin(String email, String pass, String server)
{

  // set server
  this->_server = server;
  if (this->_server == "")
    this->_server = "https://iotkiddie.com";
  this->_email = email;
  this->_pass = pass;

  DEBUGLN("Begin -> email:" + this->_email + " server:" + this->_server);

  String url = this->_server + "/api/connect.php";
  url += "?email=" + this->_email;
  url += "&pass=" + this->_pass;
  url += "&esp_id=" + this->_esp_id;
  url += "&project_id=" + String(this->_project_id);
  // DEBUGLN(url);

  String payload = getDataSSL(url);

  if (payload.toInt() > 0)
  {
    _user_id = payload.toInt();
    DEBUGLN("get user_id : " + String(_user_id));
  }
  else
    DEBUGLN("can't login");
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
        if (_project_id == 1)
        {
          DEBUGLN("sending data to server");
          if (!newio_s)
            readio();
          acMeter();
        }
        else if (_project_id == 2)
        {
          DEBUGLN("sending data to server");
          if (!newio_s)
            readio();
          pmMeter();
        }
        else if (_project_id == 4)
        {
          DEBUGLN("sending data to server");
          if (!newio_s)
            readio();
          DHT();
        }
        else if (_project_id == 5)
        {
          DEBUGLN("sending data to server");
          if (!newio_s)
            readio();
          smartFarmSolar();
        }
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

String Iotbundle::getDataSSL(String url)
{
  String payload;

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  // const char *headerNames[] = {"Location"};
  // https.collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));

  DEBUG("[HTTPS] begin...\n");

  DEBUGLN(url);

  // while (!(url == ""))
  // {

  if (https.begin(*client, url))
  { // HTTP
    // url = "";

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
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        payload = https.getString();
        DEBUGLN(payload);

        serverConnected = true;
      }
      else
      {
        serverConnected = false;
      }
      // if (https.hasHeader("Location"))
      // { // if has redirect code
      //   url = https.header("Location");
      //   DEBUGLN(url);
      // }
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
    DEBUGLN("[HTTPS} Unable to connect");
    serverConnected = false;
  }
  // }

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
  String url = this->_server + "/api/";
  url += String(_project_id);
  url += "/update.php";
  url += "?user_id=" + String(_user_id);
  url += "&esp_id=" + _esp_id;
  if (var_index)
  { // validate
    if (v >= 60 && v <= 260 && !isnan(v))
      url += "&voltage=" + String(v, 1);
    if (i >= 0 && i <= 100 && !isnan(i))
      url += "&current=" + String(i, 3);
    if (p >= 0 && p <= 24000 && !isnan(p))
      url += "&power=" + String(p, 1);
    if (e >= 0 && e <= 10000 && !isnan(e))
      url += "&energy=" + String(e, 3);
    if (f >= 40 && f <= 70 && !isnan(f))
      url += "&frequency=" + String(f, 1);
    if (pf >= 0 && pf <= 1 && !isnan(pf))
      url += "&pf=" + String(pf, 2);
  }
  if (newio_c)
    url += "&io_c=" + String(io);
  else if (newio_s)
    url += "&io_s=" + String(io);

  String payload = getDataSSL(url);

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
  url += String(_project_id);
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

  String payload = getDataSSL(url);

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
  url += String(_project_id);
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

  String payload = getDataSSL(url);

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
  url += String(_project_id);
  url += "/update.php";
  url += "?user_id=" + String(_user_id);
  url += "&esp_id=" + _esp_id;
  if (var_index)
  { // validate
    if (humid >= 0 && humid <= 100 && !isnan(humid))
      url += "&humid=" + String(humid, 1);
    if (temp >= -40 && temp <= 80 && !isnan(temp))
      url += "&temp=" + String(temp, 1);
    if (vbatt >= 0 && temp <= 9999 && !isnan(vbatt))
      url += "&vbatt=" + String(vbatt);
  }
  if (newio_c)
    url += "&io_c=" + String(io);
  else if (newio_s)
    url += "&io_s=" + String(io);

  String payload = getDataSSL(url);

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