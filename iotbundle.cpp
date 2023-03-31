#include "iotbundle.h"

Iotbundle::Iotbundle(String project)
{
  // set project id
  setProjectID(project, 0);

  // get this esp id
  this->_esp_id = String(ESP.getChipId());
}

void Iotbundle::setProjectID(String project, uint8_t array_project)
{
  // set project id
  this->_project_id[array_project] = getProjectID(project);

  if (this->_project_id[array_project] == 0)
    _AllowIO &= 0b111111111;
  else if (this->_project_id[array_project] == 1)
    _AllowIO &= 0b111100001;
  else if (this->_project_id[array_project] == 2)
    _AllowIO &= 0b111100001;
  else if (this->_project_id[array_project] == 3)
    _AllowIO &= 0b100011000;
  else if (this->_project_id[array_project] == 4)
    _AllowIO &= 0b101111001;
  else if (this->_project_id[array_project] == 5)
    _AllowIO &= 0b101100110;
  else if (this->_project_id[array_project] == 6)
    _AllowIO &= 0b111100001;
}

void Iotbundle::begin(String email, String pass, String server)
{
  Serial.println("Iotkiddie v." + version);
  Serial.println("more infomation at https://iotkiddie.com");

  // set server
  this->_server = server;
  if (this->_server == "")
    this->_server = "https://iotkiddie.com";

  // set login url
  this->_login_url = this->_server + "/api/v10/connect.php";
  this->_update_url = this->_server + "/api/v10/update.php";

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

  login();
}

void Iotbundle::login()
{
  DEBUGLN("Begin -> email:" + this->_email + " server:" + this->_server);

  DEBUGLN("No of Project : " + String(projectCount()));

  // get string project
  String project = "";
  for (uint8_t i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) >= 0)
    {
      if (i != 0)
      {
        project += ",";
      }
      project += String(_project_id[i]);
    }
  }
  DEBUGLN("Project String : " + project);

  // String url = this->_server + "/api/connect.php";
  // url += "?email=" + this->_email;
  // url += "&pass=" + this->_pass;
  // url += "&esp_id=" + this->_esp_id;
  // url += "&project_id=" + String(this->_project_id[0]);

  String data = "{\"email\":\"" + this->_email;
  data += "\",\"pass\":\"" + this->_pass;
  data += "\",\"esp_id\":" + this->_esp_id;
  data += ",\"project_id\":\"" + project + "\",";

  // add in v0.0.5
  // loop for delete '.' ex. 0.0.5 => 5
  String v_int = "";
  uint8_t count = version.length();
  for (int i = 0; i < count; i++)
  {
    if (version[i] != '.')
    {
      v_int += version[i];
    }
  }
  data += "\"version\":" + String(v_int.toInt()) + "}";

  DEBUGLN(data);

  // String payload = getData(url);
  String payload = postData(data, _login_url);

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

  projectSort();

  // show all project
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) >= 0)
      DEBUGLN("project " + String(i) + " : " + String(_project_id[i]));
  }
}

void Iotbundle::projectSort()
{
  for (int i = 1; i < sizeof(this->_project_id); ++i)
  {
    if (_project_id[i] >= 0)
    {
      int j = _project_id[i];
      int k;
      for (k = i - 1; (k >= 0) && (j < _project_id[k]); k--)
      {
        _project_id[k + 1] = _project_id[k];
      }
      _project_id[k + 1] = j;
    }
  }
}

uint8_t Iotbundle::getProjectID(String project)
{
  if (project == "CUSTOM")
  {
    return 0;
  }
  else if (project == "AC_METER")
  {
    return 1;
  }
  else if (project == "PM_METER")
  {
    return 2;
  }
  else if (project == "DC_METER")
  {
    return 3;
  }
  else if (project == "DHT")
  {
    return 4;
  }
  else if (project == "smartfarm_solar")
  {
    return 5;
  }
  else if (project == "AC_METER_3P")
  {
    return 6;
  }
  return 0;
}

void Iotbundle::handle()
{
  uint32_t currentMillis = millis();
  if (currentMillis - _previousMillis >= sendtime * 1000)
  {
    _previousMillis = currentMillis;

    DEBUGLN("TodayTimestamp: " + String(daytimestamp));
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
          login();
        }
      }
    }

    TimerHandle();
  }
}

void Iotbundle::TimerHandle()
{
  // uint8_t wemosGPIO[] = {16, 5, 4, 0, 2, 14, 12, 13, 15}; // GPIO from d0 d1 d2 ... d8
  uint8_t k = 0;
  uint8_t prev_active_pin = 9;
  while (timer_interval[k])
  {
    // DEBUGLN("[timer loop] pin:D" + String(timer_pin[k]) + " on,\ttime left " + String(timer_start[k] + timer_interval[k] - daytimestamp) + " sec");
    if (bitRead(_AllowIO, timer_pin[k]))
    {
      if ((daytimestamp >= timer_start[k]) && (daytimestamp < (timer_start[k] + timer_interval[k])))
      {
        if (!timer_state[k])
        {
          digitalWrite(wemosGPIO(timer_pin[k]), (timer_active[k] ? HIGH : LOW));
          DEBUGLN("[Timer] pin:D" + String(timer_pin[k]) + " on,\ttime left " + String(timer_start[k] + timer_interval[k] - daytimestamp) + " sec");
          value_pin[timer_pin[k]] = timer_active[k];
          prev_active_pin = timer_pin[k];
          timer_state[k] = true;
          pin_change = true;
        }
      }
      else if (prev_active_pin != timer_pin[k])
      {
        if (timer_state[k])
        {
          digitalWrite(wemosGPIO(timer_pin[k]), (timer_active[k] ? LOW : HIGH));
          DEBUGLN("[Timer] pin:D" + String(timer_pin[k]) + " off");
          value_pin[timer_pin[k]] = !timer_active[k];
          timer_state[k] = false;
          pin_change = true;
        }
      }
      pinMode(wemosGPIO(timer_pin[k]), OUTPUT);
    }
    k++;
  }
}

void Iotbundle::updateProject()
{

  if (_json_update == "")
  {
    _json_update = "{\"esp_id\":" + _esp_id + ",";
    _json_update += "\"user_id\":" + String(_user_id) + ",";
    _json_update += "\"data\":[";
  }

  uint8_t count = projectCount();
  for (uint8_t i = 0; i < count; i++)
  {

    if (i != 0)
    {
      _json_update += ',';
    }

    if (_project_id[i] == 0)
    {
      custom(i);
    }
    else if (_project_id[i] == 1)
    {
      acMeter(i);
    }
    else if (_project_id[i] == 2)
    {
      pmMeter(i);
    }
    else if (_project_id[i] == 3)
    {
      dcMeter(i);
    }
    else if (_project_id[i] == 4)
    {
      DHT(i);
    }
    else if (_project_id[i] == 5)
    {
      smartFarmSolar(i);
    }
    else if (_project_id[i] == 6)
    {
      acMeter_3p(i);
    }
  }

  _json_update += ']';

  // if (!newio_s)
  readio();

  // if (newio_c)
  //   _json_update += ",\"io_c\":" + String(io);
  // else if (newio_s)
  //   _json_update += ",\"io_s\":" + String(io);

  if (pin_s) // request pin from server
    _json_update += ",\"pin_s\":1";
  else if (pin_c) // pin from server updated
  {
    _json_update += ",\"pin_c\":1";
    pin_c = false;
  }

  if (pin_change)
  {
    _json_update += ",\"pin_change\":" + String(pin_change_checksum);
  }

  if (timer_s) // request timer from server
  {
    _json_update += ",\"timer_s\":1";
    // timer_s = false;
  }
  else if (timer_c) // timer from server updated
  {
    _json_update += ",\"timer_c\":1";
    timer_c = false;
  }

  if (daytimestamp_s) // request today timestamp from server
  {
    _json_update += ",\"daytimestamp_s\":1";
  }

  _json_update += "}";

  DEBUGLN("sending data to server");

  String payload = postData(_json_update, _update_url);
  _json_update = "";
  if (serverConnected)
  {
    _noConnect = 0;
    if (payload != "")
    {
      Stringparse(payload);
    }
  }
  else
  {
    _noConnect++;
    if (payload != "" && _noConnect >= 6) // if can't connect about 30 sec
      this->noti = payload;               // display no oled
  }

  clearvar();
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

void Iotbundle::forceUpdate(bool settolowall)
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

      updateProject();
    }
  }
}

void Iotbundle::setProject(String projectname)
{
  // get project id
  uint8_t project_id = getProjectID(projectname);

  // find project array index
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) == project_id)
      activeProject = i;
  }
}

void Iotbundle::update(float var1, float var2, float var3, float var4, float var5, float var6, float var7, float var8, float var9, float var10)
{
  if (var_index[activeProject] >= 200) // if almost overflow
    clearvar();

  if (!isnan(var1) || !isnan(var2) || !isnan(var3) || !isnan(var4) || !isnan(var5) || !isnan(var6) || !isnan(var7) || !isnan(var8) || !isnan(var9) || !isnan(var10)) // not update if all nan
  {

    var_index[activeProject]++;
    var_sum[0][activeProject] += var1;
    var_sum[1][activeProject] += var2;
    var_sum[2][activeProject] += var3;
    var_sum[3][activeProject] += var4;
    var_sum[4][activeProject] += var5;
    var_sum[5][activeProject] += var6;
    var_sum[6][activeProject] += var7;
    var_sum[7][activeProject] += var8;
    var_sum[8][activeProject] += var9;
    var_sum[9][activeProject] += var10;

    DEBUG("updated data " + String(var_index[activeProject]) + " -> ");
    for (int i = 0; i < 10; i++)
    {
      DEBUG(String(var_sum[i][activeProject]) + ", ");
    }
    DEBUGLN();
    DEBUGLN("FreeHeap : " + String(ESP.getFreeHeap()));
  }
}

void Iotbundle::update(float v[3], float a[3], float p[3], float e[3], float f[3], float pf[3])
{
  for (uint8_t i = 0; i < 3; i++)
  {

    if (var_index_3p[i] >= 200) // if almost overflow
      clearvar();

    if (!isnan(v[i]) || !isnan(a[i]) || !isnan(p[i]) || !isnan(e[i]) || !isnan(f[i]) || !isnan(pf[i]))
    {
      var_index_3p[i]++;
      var_sum_3p[0][i] += v[i];
      var_sum_3p[1][i] += a[i];
      var_sum_3p[2][i] += p[i];
      var_sum_3p[3][i] += e[i];
      var_sum_3p[4][i] += f[i];
      var_sum_3p[5][i] += pf[i];

      DEBUG("updated data " + String(var_index_3p[i]) + " -> ");
      for (uint8_t j = 0; j < 6; j++)
      {
        DEBUG(String(var_sum_3p[j][i]) + ", ");
      }
      DEBUGLN();
    }
  }
  DEBUGLN("FreeHeap : " + String(ESP.getFreeHeap()));
}

void Iotbundle::clearvar()
{
  for (uint8_t i = 0; i < 10; i++)
  {
    for (uint8_t i2 = 0; i2 < 5; i2++)
    {
      var_sum[i][i2] = 0;
    }
  }
  for (uint8_t j = 0; j < 5; j++)
  {
    var_index[j] = 0;
  }

  for (uint8_t i = 0; i < 6; i++)
  {
    for (uint8_t i2 = 0; i2 < 3; i2++)
    {
      var_sum_3p[i][i2] = 0;
    }
  }
  for (uint8_t j = 0; j < 3; j++)
  {
    var_index_3p[j] = 0;
  }
}

String Iotbundle::getData(String data)
{
  String payload;
  if (_server[4] == 's')
    return getHttps(data);
  else
    return getHttp(data);
}

String Iotbundle::postData(String data, String url)
{
  String payload;
  if (_server[4] == 's')
    return postHttps(data, url);
  else
    return postHttp(data, url);
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
      payload = http.errorToString(httpCode);
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
    payload = "Unable\nto\nconnect";
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
      payload = https.errorToString(httpCode);
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
    payload = "Unable\nto\nconnect";
  }
  // }

  return payload;
}

String Iotbundle::postHttp(String data, String url)
{
  String payload;
  WiFiClient client;
  HTTPClient http;

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
      payload = http.errorToString(httpCode);
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
    payload = "Unable\nto\nconnect";
  }
  return payload;
}

String Iotbundle::postHttps(String data, String url)
{
  String payload;

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

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
      payload = https.errorToString(httpCode);
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
    payload = "Unable\nto\nconnect";
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
  // DEBUGLN("io:" + String(io, BIN));
  uint8_t wemosGPIO[] = {16, 5, 4, 0, 2, 14, 12, 13, 15}; // GPIO from d0 d1 d2 ... d8
  uint16_t useio = io ^ previo;                           // change only difference io
  DEBUGLN("newio:\t" + String(io, BIN));
  DEBUGLN("previo:\t" + String(previo, BIN));
  DEBUGLN("useio:\t" + String(useio, BIN));
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
  for (int i = 0; i <= 8; i++)
  {
    // DEBUGLN("[pinread before] pin D" + String(i) + "\tmode:" + String(pin_mode[i]) + "\tvalue:" + String(value_pin[i]));

    if (pin_mode[i] == 1 && bitRead(_AllowIO, i)) // input pin and must allow pin
    {
      if (digitalRead(wemosGPIO(i)) != value_pin[i])
      {
        value_pin[i] = digitalRead(wemosGPIO(i));
        pin_change = true;
        // DEBUGLN("input mode condition");
      }
      bitWrite(pin_change_checksum, i, value_pin[i]);
    }
    else if (pin_mode[i] == 2 && bitRead(_AllowIO, i)) // out pin and must allow pin
    {
      if (digitalRead(wemosGPIO(i)) != value_pin[i])
      {
        value_pin[i] = digitalRead(wemosGPIO(i));
        pin_change = true;
        // DEBUGLN("output mode condition");
      }
      bitWrite(pin_change_checksum, i, value_pin[i]);
    }
    // else if (pin_mode[i] == 3 && bitRead(_AllowIO, i)) // pwm pin and must allow pin
    // {
    //   if (digitalRead(wemosGPIO(i)) != value_pin[i])
    //   {
    //     value_pin[i] = digitalRead(wemosGPIO(i));
    //     pin_change = true;
    //   }
    // }
    else
    {
      // DEBUGLN("else condition");
      bitWrite(pin_change_checksum, i, 0);
    }
    // DEBUGLN("[pinread after] pin D" + String(i) + "\tmode:" + String(pin_mode[i]) + "\tvalue:" + String(value_pin[i]));
  }
  DEBUGLN("[pinread after] binary : " + String(pin_change_checksum, BIN));
}

void Iotbundle::setAllowIO(uint16_t allowio)
{
  this->_AllowIO = allowio;
  // init_io();
}

uint8_t Iotbundle::wemosGPIO(uint8_t pin)
{
  uint8_t wemosGPIO[] = {16, 5, 4, 0, 2, 14, 12, 13, 15};
  return wemosGPIO[pin];
}

void Iotbundle::Stringparse(String payload)
{
  int str_len = payload.length() + 1;
  char buff[str_len];
  payload.toCharArray(buff, str_len);

  // response string
  String res_data[10];
  int8_t res_index = -1;

  // split String with '&'
  for (int i = 0; i < str_len; i++)
  {
    if (buff[i] == '&')
    {
      res_index++;
      i++;
    }

    res_data[res_index] += buff[i];
  }

  // code response
  res_index = 0;
  int j = 0;
  String res_code, res_value;

  // split key=value with '='
  while (res_data[res_index] != "")
  {
    // DEBUGLN("res_data[" + String(res_index) + "] = " + res_data[res_index]);
    for (int i = 0; i < res_data[res_index].length() + 1; i++)
    {
      if (res_data[res_index][i] == '=')
      {
        j++;
        i++;
      }

      // DEBUGLN(res_data[res_index][i]);
      if (j == 0)
        res_code += res_data[res_index][i];
      else if (j == 1)
        res_value += res_data[res_index][i];
    }

    DEBUGLN("res_code : " + res_code + " = " + res_value);

    if (res_code.toInt() == 0) // have error
    {
      Serial.println("!error:" + res_value);
    }
    else if (res_code.toInt() == 1) // new io form server
    {
      // io = res_value.toInt();
      pinhandle_s(res_value);
      // newio_s = true;
    }
    else if (res_code.toInt() == 2) // io from client updated
    {
      if (res_value.toInt() == pin_change_checksum)
      {
        pin_change = false;
      }
    }
    else if (res_code.toInt() == 32767) // io from server updated
    {
      newio_s = false;
    }
    else if (res_code.toInt() == 32766) // io from client updated
    {
      newio_s = false;
      newio_c = false;
    }
    else if (res_code.toInt() == 32765) // check ota update
    {
      need_ota = true;
    }
    else if (res_code.toInt() == 32764) // Timer io update from server
    {
      Timerparse(res_value);
    }
    else if (res_code.toInt() == 32763) // today timestamp
    {
      daytimestamp_s = false;
      daytimestamp = res_value.toInt();
    }

    res_code = "";
    res_value = "";
    res_index++;
    j = 0;
  }
}

void Iotbundle::pinhandle_s(String pindata)
{
  pin_c = true;
  pin_s = false;

  if (pindata != "0")
    pin_change = true;

  int str_len = pindata.length() + 1;
  char buff[str_len];
  pindata.toCharArray(buff, str_len);

  // response string
  String res_data[9];
  uint8_t res_index;

  // split String with ','
  for (int i = 0; i < str_len; i++)
  {
    if (buff[i] == ',')
    {
      res_index++;
      i++;
    }

    res_data[res_index] += buff[i];
  }

  // split key=value with '='
  res_index = 0;
  while (res_data[res_index] != "")
  {
    DEBUGLN("[Pindata] " + String(res_index) + " : " + (String)res_data[res_index]);

    // split string
    uint8_t pin = (res_data[res_index].substring(0, res_data[res_index].indexOf(':'))).toInt();

    char mode_c = res_data[res_index][res_data[res_index].indexOf(':') + 1];
    uint8_t mode = 0;
    if (mode_c == 'I')
      mode = 1;
    else if (mode_c == 'O')
      mode = 2;
    else if (mode_c == 'P')
      mode = 3;

    uint16_t value = (res_data[res_index].substring(res_data[res_index].lastIndexOf(':') + 1, res_data[res_index].length())).toInt();

    DEBUGLN("Pin:" + String(pin) + "\tMode:" + String(mode) + "\tValue:" + String(value));

    if (pin_mode[pin] != mode && bitRead(_AllowIO, pin)) // if pinmode not same and must allow pin
    {
      pin_mode[pin] = mode; // change pin mode
    }
    if (bitRead(_AllowIO, pin)) // cheange pin value
    {
      if (pin_mode[pin] == 1) // input
      {
        pinMode(wemosGPIO(pin), INPUT);
        DEBUGLN("Pin:D" + String(pin) + " set as Input");
      }
      else if (pin_mode[pin] == 2) // output
      {
        pinMode(wemosGPIO(pin), OUTPUT);
        digitalWrite(wemosGPIO(pin), value ? HIGH : LOW);
        value_pin[pin] = value;
        // prev_value_pin[pin]=value;
        DEBUGLN("Pin:D" + String(pin) + " set as Output\tvalue:" + (value ? "HIGH" : "LOW"));
      }
      else if (pin_mode[pin] == 3) // PWM
      {
        pinMode(wemosGPIO(pin), OUTPUT);
        analogWrite(wemosGPIO(pin), value);
        value_pin[pin] = value;
        // prev_value_pin[pin]=value;
        DEBUGLN("Pin:D" + String(pin) + " set as PWM\tvalue:" + String(value));
      }
    }

    res_index++;
  }
}

void Iotbundle::Timerparse(String timer)
{
  //  format {pin}:{start}:{interval}:{active h-l},{pin}:{start}:{interval}:{active h-l}
  timer_c = true;
  timer_s = false;
  int8_t j = 0, timer_index = 0;
  String buff_timer = "";

  // clear old timer
  timer_interval[0] = 0;
  timer_interval[1] = 0;
  timer_interval[2] = 0;
  timer_interval[3] = 0;
  timer_interval[4] = 0;
  timer_interval[5] = 0;
  timer_interval[6] = 0;
  timer_interval[7] = 0;
  timer_interval[8] = 0;
  timer_interval[9] = 0;
  timer_interval[10] = 0;

  if (timer)
  {
    for (int i = 0; i < timer.length() + 1; i++)
    {
      if (timer[i] == ',')
      {
        i++;
        if (j == 3)
        {
          timer_active[timer_index] = buff_timer.toInt();
        }
        j = 0;
        timer_index++;
        buff_timer = "";
      }
      else if (timer[i] == ':')
      {
        if (j == 0)
        {
          timer_pin[timer_index] = buff_timer.toInt();
        }
        else if (j == 1)
        {
          timer_start[timer_index] = buff_timer.toInt();
        }
        else if (j == 2)
        {
          timer_interval[timer_index] = buff_timer.toInt();
        }
        else if (j == 3)
        {
          timer_active[timer_index] = buff_timer.toInt();
        }

        buff_timer = "";
        j++;
        i++;
      }
      else if (i == timer.length())
      {
        if (j == 3)
        {
          timer_active[timer_index] = buff_timer.toInt();
        }
      }

      buff_timer += timer[i];

      // DEBUGLN(timer[i]);
      // if (j == 0)
      //   res_code += timer[i];
      // else if (j == 1)
      // res_value += timer[i];
    }

    uint8_t k = 0;

    while (timer_interval[k])
    {
      DEBUGLN("pin:" + (String)timer_pin[k] + ", start:" + (String)timer_start[k] + ", interval:" + (String)timer_interval[k] + ", active:" + (String)timer_active[k]);
      k++;
    }
  }
  else
  {
    DEBUGLN("No timer");
  }
}

void Iotbundle::otaUpdate(String optional_version, String url)
{
  // WiFiClient client;

  ESPhttpUpdate.onStart([]()
                        { Serial.println("CALLBACK:  HTTP update process started"); });

  ESPhttpUpdate.onEnd([]()
                      { Serial.println("CALLBACK:  HTTP update process finished"); });

  ESPhttpUpdate.onProgress([](int cur, int total)
                           { Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total); });

  ESPhttpUpdate.onError([](int err)
                        { Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err); });

  // get project id string
  String project = "";
  for (uint8_t i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) >= 0)
    {
      if (i != 0)
      {
        project += ",";
      }
      project += String(_project_id[i]);
    }
  }

  // get version int
  String v_int = "";
  uint8_t count = this->version.length();
  for (int i = 0; i < count; i++)
  {
    if (this->version[i] != '.')
    {
      v_int += this->version[i];
    }
  }

  t_httpUpdate_return ret;
  if (url.length() == 0) // use default url
  {

    url = _server + "/ota/esp8266.php" + "?p_id=" + project;
    if (optional_version != "")
      url += "&optional_version=" + optional_version;

    if (_server[4] == 's')
    {
      WiFiClientSecure client;
      client.setInsecure();
      ret = ESPhttpUpdate.update(client, url, String(v_int.toInt()));
    }
    else
    {
      WiFiClient client;
      ret = ESPhttpUpdate.update(client, url, String(v_int.toInt()));
    }
  }
  else // use custom url
  {
    url = url + "?p_id=" + project;
    if (optional_version != "")
      url += "&optional_version=" + optional_version;

    if (url[4] == 's')
    {
      WiFiClientSecure client;
      client.setInsecure();
      ret = ESPhttpUpdate.update(client, url, String(v_int.toInt()));
    }
    else
    {
      WiFiClient client;
      ret = ESPhttpUpdate.update(client, url, String(v_int.toInt()));
    }
    // Serial.println("custom url:" + url);
  }

  // Serial.println("url:" + url);
  // t_httpUpdate_return ret = ESPhttpUpdate.update(client, url, version_payload);
  // t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);

  // t_httpUpdate_return ret = ESPhttpUpdate.update(client, "192.168.2.50", 80, "/ota/esp8266.php", version);
  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.println("[update] Update failed.");
    break;
  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("[update] Update no Update.");
    break;
  case HTTP_UPDATE_OK:
    Serial.println("[update] Update ok."); // may not be called since we reboot the ESP
    break;
  }
}

void Iotbundle::interrupt1sec()
{
  // today timestamp update
  daytimestamp++;

  if (daytimestamp >= 86400)
  {
    daytimestamp = daytimestamp % 86400;
  }

  if (daytimestamp % 600 == 450) // update time every 10 min
  {
    timer_s = true;
  }
}

uint32_t Iotbundle::getTodayTimestamp()
{
  return daytimestamp;
}

void Iotbundle::custom(uint8_t id)
{
  // get project id
  uint8_t project_id = getProjectID("CUSTOM");

  // find project array index
  uint8_t array;
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) == project_id)
      array = i;
  }

  // calculate
  float c0 = var_sum[0][array] / var_index[array];
  float c1 = var_sum[1][array] / var_index[array];
  float c2 = var_sum[2][array] / var_index[array];
  float c3 = var_sum[3][array] / var_index[array];
  float c4 = var_sum[4][array] / var_index[array];
  float c5 = var_sum[5][array] / var_index[array];
  float c6 = var_sum[6][array] / var_index[array];
  float c7 = var_sum[7][array] / var_index[array];
  float c8 = var_sum[8][array] / var_index[array];
  float c9 = var_sum[9][array] / var_index[array];

  _json_update += "{\"project_id\":" + String(_project_id[id]);

  if (var_index[array])
  { // validate
    if (!isnan(c0))
      _json_update += ",\"c0\":" + String(c0, 1);
    if (!isnan(c1))
      _json_update += ",\"c1\":" + String(c1, 1);
    if (!isnan(c2))
      _json_update += ",\"c2\":" + String(c2, 1);
    if (!isnan(c3))
      _json_update += ",\"c3\":" + String(c3, 1);
    if (!isnan(c4))
      _json_update += ",\"c4\":" + String(c4, 1);
    if (!isnan(c5))
      _json_update += ",\"c5\":" + String(c5, 1);
    if (!isnan(c6))
      _json_update += ",\"c6\":" + String(c6, 1);
    if (!isnan(c7))
      _json_update += ",\"c7\":" + String(c7, 1);
    if (!isnan(c8))
      _json_update += ",\"c8\":" + String(c8, 1);
    if (!isnan(c9))
      _json_update += ",\"c9\":" + String(c9, 1);
  }
  _json_update += "}";
}

void Iotbundle::acMeter(uint8_t id)
{
  // get project id
  uint8_t project_id = getProjectID("AC_METER");

  // find project array index
  uint8_t array;
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) == project_id)
      array = i;
  }

  // calculate
  float v = var_sum[0][array] / var_index[array];
  float i = var_sum[1][array] / var_index[array];
  float p = var_sum[2][array] / var_index[array];
  float e = var_sum[3][array] / var_index[array];
  float f = var_sum[4][array] / var_index[array];
  float pf = var_sum[5][array] / var_index[array];

  _json_update += "{\"project_id\":" + String(_project_id[id]);

  if (var_index[array])
  { // validate
    if (v >= 50 && v <= 270 && !isnan(v))
      _json_update += ",\"voltage\":" + String(v, 1);
    if (i >= 0 && i <= 120 && !isnan(i))
      _json_update += ",\"current\":" + String(i, 3);
    if (p >= 0 && p <= 25000 && !isnan(p))
      _json_update += ",\"power\":" + String(p, 1);
    if (e >= 0 && e <= 99999 && !isnan(e))
      _json_update += ",\"energy\":" + String(e, 3);
    if (f >= 40 && f <= 70 && !isnan(f))
      _json_update += ",\"frequency\":" + String(f, 1);
    if (pf >= 0 && pf <= 1 && !isnan(pf))
      _json_update += ",\"pf\":" + String(pf, 2);
  }
  _json_update += "}";
}

void Iotbundle::pmMeter(uint8_t id)
{
  // get project id
  uint8_t project_id = getProjectID("PM_METER");

  // find project array index
  uint8_t array;
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) == project_id)
      array = i;
  }

  // calculate
  uint16_t pm1 = var_sum[0][array] / var_index[array];
  uint16_t pm2 = var_sum[1][array] / var_index[array];
  uint16_t pm10 = var_sum[2][array] / var_index[array];

  _json_update += "{\"project_id\":" + String(_project_id[id]);

  if (var_index[array])
  { // validate
    if (pm1 >= 0 && pm1 <= 1999 && !isnan(pm1))
      _json_update += ",\"pm1\":" + String(pm1);
    if (pm2 >= 0 && pm2 <= 1999 && !isnan(pm2))
      _json_update += ",\"pm2\":" + String(pm2);
    if (pm10 >= 0 && pm10 <= 1999 && !isnan(pm10))
      _json_update += ",\"pm10\":" + String(pm10);
  }
  _json_update += "}";
}

void Iotbundle::dcMeter(uint8_t id)
{
  // get project id
  uint8_t project_id = getProjectID("DC_METER");

  // find project array index
  uint8_t array;
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) == project_id)
      array = i;
  }

  // calculate
  float v = var_sum[0][array] / var_index[array];
  float i = var_sum[1][array] / var_index[array];
  float p = var_sum[2][array] / var_index[array];
  float e = var_sum[3][array] / var_index[array];

  _json_update += "{\"project_id\":" + String(_project_id[id]);

  if (var_index[array])
  { // validate
    if (v >= 0 && v <= 300 && !isnan(v))
      _json_update += ",\"voltage\":" + String(v, 2);
    if (i >= 0 && i <= 300 && !isnan(i))
      _json_update += ",\"current\":" + String(i, 2);
    if (p >= 0 && p <= 90000 && !isnan(p))
      _json_update += ",\"power\":" + String(p, 1);
    if (e >= 0 && e <= 10000 && !isnan(e))
      _json_update += ",\"energy\":" + String(e, 3);
  }
  _json_update += "}";
}

void Iotbundle::DHT(uint8_t id)
{
  // get project id
  uint8_t project_id = getProjectID("DHT");

  // find project array index
  uint8_t array;
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) == project_id)
      array = i;
  }

  // calculate
  float humid = var_sum[0][array] / var_index[array];
  float temp = var_sum[1][array] / var_index[array];

  _json_update += "{\"project_id\":" + String(_project_id[id]);

  if (var_index[array])
  { // validate
    if (humid >= 10 && humid <= 100 && !isnan(humid))
      _json_update += ",\"humid\":" + String(humid, 1);
    if (temp <= 70 && !isnan(temp))
      _json_update += ",\"temp\":" + String(temp, 1);
  }

  _json_update += "}";
}

void Iotbundle::smartFarmSolar(uint8_t id)
{
  // get project id
  uint8_t project_id = getProjectID("smartfarm_solar");

  // find project array index
  uint8_t array;
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) == project_id)
      array = i;
  }

  // calculate
  float humid = var_sum[0][array] / var_index[array];
  float temp = var_sum[1][array] / var_index[array];
  uint16_t vbatt = var_sum[2][array] / var_index[array];

  if (var_index[array])
  { // validate
    if (humid > 0 && humid <= 100 && !isnan(humid))
      _json_update += ",\"humid\":" + String(humid, 1);
    if (temp > 0 && temp <= 80 && !isnan(temp))
      _json_update += ",\"temp\":" + String(temp, 1);
    if (vbatt >= 2000 && vbatt <= 8000 && !isnan(vbatt))
      _json_update += ",\"vbatt\":" + String(vbatt);
  }
  _json_update += ",\"valve\":";
  _json_update += (digitalRead(D1)) ? "1" : "0";

  _json_update += "}";
}

void Iotbundle::acMeter_3p(uint8_t id)
{
  // get project id
  uint8_t project_id = getProjectID("AC_METER_3P");

  // find project array index
  uint8_t array;
  for (byte i = 0; i < sizeof(this->_project_id); i++)
  {
    if ((_project_id[i]) == project_id)
      array = i;
  }

  _json_update += "{\"project_id\":" + String(_project_id[id]);

  for (uint8_t i = 0; i < 3; i++)
  {
    // calculate
    float v = var_sum_3p[0][i] / var_index_3p[i];
    float a = var_sum_3p[1][i] / var_index_3p[i];
    float p = var_sum_3p[2][i] / var_index_3p[i];
    float e = var_sum_3p[3][i] / var_index_3p[i];
    float f = var_sum_3p[4][i] / var_index_3p[i];
    float pf = var_sum_3p[5][i] / var_index_3p[i];

    if (var_index_3p[i])
    { // validate
      if (v >= 60 && v <= 260 && !isnan(v))
        _json_update += ",\"v" + String(i + 1) + "\":" + String(v, 1);
      if (a >= 0 && a <= 100 && !isnan(a))
        _json_update += ",\"i" + String(i + 1) + "\":" + String(a, 3);
      if (p >= 0 && p <= 24000 && !isnan(p))
        _json_update += ",\"p" + String(i + 1) + "\":" + String(p, 1);
      if (e >= 0 && e <= 10000 && !isnan(e))
        _json_update += ",\"e" + String(i + 1) + "\":" + String(e, 3);
      if (f >= 40 && f <= 70 && !isnan(f))
        _json_update += ",\"f" + String(i + 1) + "\":" + String(f, 1);
      if (pf >= 0 && pf <= 1 && !isnan(pf))
        _json_update += ",\"pf" + String(i + 1) + "\":" + String(pf, 2);
    }
  }

  _json_update += "}";
}