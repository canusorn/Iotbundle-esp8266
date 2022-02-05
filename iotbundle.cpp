#include "iotbundle.h"

Iotbundle::Iotbundle(String server, String project)
{

  //set server
  if (server == "IOTKID")
    this->_server = "https://iotkid.space/";
  else if (server == "IOTKIDDIE")
    this->_server = "https://iotkiddie.com/";

  //set project id
  if (project == "AC_METER")
  {
    this->_project_id = 1;
    AllowIO = 0b111100001;
  } // pin4,3=pzem  2,1=i2c
  else if (project == "PM_METER")
    this->_project_id = 2;
  else if (project == "DC_METER")
    this->_project_id = 3;

  //get this esp id
  this->_esp_id = String(ESP.getChipId());
}

void Iotbundle::begin(String email, String pass)
{

  this->_email = email;
  this->_pass = pass;

  String url = this->_server + "api/connect.php";
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
}

void Iotbundle::handle()
{
  uint32_t currentMillis = millis();
  if (currentMillis - _previousMillis >= sendtime * 1000)
  {
    _previousMillis = currentMillis;
    if (_user_id > 0)
    {
      if (var_index)
      {
        if (_project_id == 1)
        {

          DEBUGLN("sending data to server");
          acMeter();
        }
      }
    }
    else
    {
      _get_userid++;
      if (_get_userid >= retryget_userid)
      {
        _get_userid = 0;

        DEBUGLN("retry login");
        begin(this->_email, this->_pass);
      }
    }
  }
}

void Iotbundle::update(float var1, float var2, float var3, float var4, float var5, float var6, float var7, float var8, float var9, float var10)
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

  const char *headerNames[] = {"Location"};
  https.collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));

  DEBUG("[HTTPS] begin...\n");

  DEBUGLN(url);

  while (!(url == ""))
  {

    if (https.begin(*client, url))
    { // HTTP
      url = "";
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
        if (https.hasHeader("Location"))
        { // if has redirect code
          url = https.header("Location");
          DEBUGLN(url);
        }
      }
      else
      {
        DEBUGLN("[HTTPS] GET... failed, error: " + https.errorToString(httpCode));
        serverConnected = false;
      }

      https.end();
    }
    else
    {
      DEBUGLN("[HTTPS} Unable to connect");
      serverConnected = false;
    }
  }

  return payload;
}

bool Iotbundle::status()
{
  return serverConnected;
}

void Iotbundle::iohandle_s()
{ //handle io from server
  DEBUGLN("io:" + String(io, BIN));
  uint8_t wemosGPIO[] = {16, 5, 4, 0, 2, 14, 12, 13, 15}; // GPIO from d0 d1 d2 ... d8
  uint16_t useio = io & AllowIO;
  for (int i = 0; i < 9; i++)
  {
    if (bitRead(useio, i))
    { // use only allow pin
      if (bitRead(io, i))
      {
        pinMode(wemosGPIO[i], OUTPUT);
        digitalWrite(wemosGPIO[i], HIGH);
      }
      else
      {
        pinMode(wemosGPIO[i], OUTPUT);
        digitalWrite(wemosGPIO[i], LOW);
      }
    }
  }
}

void Iotbundle::acMeter()
{
  String url = this->_server + "api/";
  url += String(_project_id);
  url += "/update.php";
  url += "?user_id=" + String(_user_id);
  url += "&esp_id=" + _esp_id;
  if (!isnan(var_sum[0]))
    url += "&voltage=" + String(var_sum[0] / var_index, 1);
  if (!isnan(var_sum[1]))
    url += "&current=" + String(var_sum[1] / var_index, 3);
  if (!isnan(var_sum[2]))
    url += "&power=" + String(var_sum[2] / var_index, 1);
  if (!isnan(var_sum[3]))
    url += "&energy=" + String(var_sum[3] / var_index, 3);
  if (!isnan(var_sum[4]))
    url += "&frequency=" + String(var_sum[4] / var_index, 1);
  if (!isnan(var_sum[5]))
    url += "&pf=" + String(var_sum[5] / var_index, 2);
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
    else if (newio > 0)
    {
      io = newio;
      iohandle_s();
      newio_s = true;
    }
  }

  if (serverConnected)
    clearvar();
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