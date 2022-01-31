
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
  }
  else if (project == "PM_METER")
    this->_project_id = 2;
  else if (project == "DC_METER")
    this->_project_id = 3;

  //get this esp id
  this->_esp_id = String(ESP.getChipId());
}

void Iotbundle::init(String email, String pass)
{

  this->_email = email;
  this->_pass = pass;

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  const char *headerNames[] = {"Location"};
  https.collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));

  DEBUG("[HTTPS] begin...\n");
  String url = this->_server + "api/connect.php";
  url += "?email=" + this->_email;
  url += "&pass=" + this->_pass;
  url += "&esp_id=" + this->_esp_id;
  url += "&project_id=" + this->_project_id;

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
          String payload = https.getString();
          DEBUGLN(payload);
          if (payload.toInt() > 0)
          {
            _user_id = payload.toInt();
            DEBUGLN("get user_id : " + String(_user_id));
          }
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
}

void Iotbundle::handle()
{
  uint32_t currentMillis = millis();
  if (currentMillis - _previousMillis >= sendtime * 1000)
  {
    _previousMillis = currentMillis;
    if (_user_id > 0)
    {
      if (_project_id == 1)
      {
        Acmeter meter(_server, _user_id, _project_id, _esp_id);
        if (var_index)
        {
        //   DEBUGLN("sending data to server");
          // bool senddata = 
          meter.update(var_sum[0] / var_index,
                                         var_sum[1] / var_index,
                                         var_sum[2] / var_index,
                                         var_sum[3] / var_index,
                                         var_sum[4] / var_index,
                                         var_sum[5] / var_index);
        //   if (senddata)
        //   {
        //     clearvar();
        //   }
        //   else
        //   {
        //     DEBUGLN("send fail");
        //   }
        // }
        // else
        // {
        //   DEBUGLN("no data to send");
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
        init(this->_email, this->_pass);
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