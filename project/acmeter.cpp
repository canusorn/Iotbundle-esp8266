#include "acmeter.h"

Acmeter::Acmeter(String server, uint16_t user_id, uint8_t project_id, String esp_id)
{
    _server = server;
    _user_id = user_id;
    _project_id = project_id;
    _esp_id = esp_id;
}

bool Acmeter::update(float v, float i, float p, float e, float f, float pf)
{
    this->voltage = v;
    this->current = i;
    this->power = p;
    this->energy = e;
    this->frequency = f;
    this->pf = pf;

    return sentData();
}

bool Acmeter::sentData()
{

    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    HTTPClient https;

    const char *headerNames[] = {"Location"};
    https.collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));

    DEBUG("[HTTP] begin...\n");
    String url = this->_server + "api/";
    url += String(_project_id);
    url += "/update.php";
    url += "?user_id=" + String(_user_id);
    url += "&esp_id=" + _esp_id;
    if (!isnan(voltage))
    {
        url += "&voltage=" + String(voltage, 1);
        url += "&current=" + String(current, 3);
        url += "&power=" + String(power, 1);
        url += "&energy=" + String(energy, 3);
        url += "&frequency=" + String(frequency, 1);
        url += "&pf=" + String(pf, 2);
    }
    // if (newio_c)
    //     url += "&io_c=" + String(io);
    // else if (newio_s)
    //     url += "&io_s=" + String(io);

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
                DEBUGLN("[HTTPS] GET... code: " + (String)httpCode);

                // file found at server
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    String payload = https.getString();
                    DEBUGLN(payload);
                    // jsonParse(payload);
                    // if (payload.toInt() > 0)
                    // {
                    //     user_id = payload.toInt();
                    //     DEBUGLN("get user_id : " + String(user_id));
                    // }
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

    return serverConnected;
}
