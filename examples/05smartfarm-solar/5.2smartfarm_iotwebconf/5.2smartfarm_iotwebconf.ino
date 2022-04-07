#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <DHT.h>
#include <iotbundle.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(project)
#define PROJECT "smartfarm_solar"
Iotbundle iot(PROJECT);

const char thingName[] = "smartfarm-solar";
const char wifiInitialApPassword[] = "iotbundle";

#define STRING_LEN 128
#define NUMBER_LEN 32

#define CONFIG_VERSION "0.0.4"

// -- Method declarations.
void handleRoot();
// -- Callback methods.
void wifiConnected();
void configSaved();
bool formValidator(iotwebconf::WebRequestWrapper *webRequestWrapper);

#define DHTPIN D7
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

#define maxValveOn 20 // in minutes

unsigned long previousMillis = 0, valveOnProtect;
uint8_t dhtSample;
uint16_t valveOnTime;
bool valveOnFlag;

DNSServer dnsServer;
WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

char emailParamValue[STRING_LEN];
char passParamValue[STRING_LEN];
char serverParamValue[STRING_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- You can also use namespace formats e.g.: iotwebconf::TextParameter
IotWebConfParameterGroup login = IotWebConfParameterGroup("login", "ล็อกอิน(สมัครที่เว็บก่อนนะครับ)");

IotWebConfTextParameter emailParam = IotWebConfTextParameter("อีเมลล์", "emailParam", emailParamValue, STRING_LEN);
IotWebConfPasswordParameter passParam = IotWebConfPasswordParameter("รหัสผ่าน", "passParam", passParamValue, STRING_LEN);
IotWebConfTextParameter serverParam = IotWebConfTextParameter("เซิฟเวอร์", "serverParam", serverParamValue, STRING_LEN, "https://iotkiddie.com");

void setup()
{
    Serial.begin(115200);

    // set low on boot
    digitalWrite(D1, LOW);
    pinMode(D1, OUTPUT);

    // go to deep sleep if low battery
    uint16_t vbatt = analogRead(A0) * 6200 / 1024; // Rall=300k+220k+100k  ->   max=6200mV at adc=1024(10bit)
    if (vbatt <= 4000 && vbatt >= 1000)            // if <= 1000 is no battery
    // if (vbatt <= 3900)
    {
        vbatt = 0;
        for (int i = 0; i < 20; i++)
        {
            vbatt += analogRead(A0) * 6200 / 1024;
            delay(50);
        }
        if (vbatt /= 20 < 4000)
        {
            pinMode(D4, OUTPUT);
            digitalWrite(D4, LOW);
            delay(2000);
            digitalWrite(D4, HIGH);
            delay(200);
            digitalWrite(D4, LOW);
            delay(2000);
            ESP.deepSleep(10 * 60e6); // sleep for 10 minutes
        }
    }

    dht.begin();

    // for clear eeprom jump D5 to GND
    pinMode(D5, INPUT_PULLUP);
    if (digitalRead(D5) == false)
    {
        delay(1000);
        if (digitalRead(D5) == false)
        {
            delay(1000);
            clearEEPROM();
        }
    }

    login.addItem(&emailParam);
    login.addItem(&passParam);
    login.addItem(&serverParam);

    iotWebConf.setStatusPin(D4);
    // iotWebConf.setConfigPin(CONFIG_PIN);
    //  iotWebConf.addSystemParameter(&stringParam);
    iotWebConf.addParameterGroup(&login);
    iotWebConf.setConfigSavedCallback(&configSaved);
    iotWebConf.setFormValidator(&formValidator);
    iotWebConf.getApTimeoutParameter()->visible = false;
    iotWebConf.setWifiConnectionCallback(&wifiConnected);

    // -- Define how to handle updateServer calls.
    iotWebConf.setupUpdateServer(
        [](const char *updatePath)
        {
            httpUpdater.setup(&server, updatePath);
        },
        [](const char *userName, char *password)
        {
            httpUpdater.updateCredentials(userName, password);
        });

    // -- Initializing the configuration.
    iotWebConf.init();

    // -- Set up required URL handlers on the web server.
    server.on("/", handleRoot);
    server.on("/config", []
              { iotWebConf.handleConfig(); });
    server.on("/cleareeprom", clearEEPROM);
    server.on("/reboot", reboot);
    server.onNotFound([]()
                      { iotWebConf.handleNotFound(); });

    Serial.println("ESPID: " + String(ESP.getChipId()));
    Serial.println("Ready.");
}

void loop()
{
    // 3 คอยจัดการ และส่งค่าให้เอง
    iot.handle();

    iotWebConf.doLoop();
    server.handleClient();
    MDNS.update();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 2000)
    { // run every 2 second
        previousMillis = currentMillis;

        // protection Max time valve on
        if (digitalRead(D1) && !valveOnProtect)
        {
            // capture time when start on valve
            valveOnProtect = millis() + maxValveOn * 60 * 1000;
        }
        else if (!digitalRead(D1))
        {
            valveOnProtect = 0;
        }
        else if (millis() >= valveOnProtect)
        {
            valveOnProtect = 0;
            digitalWrite(D1, LOW);
        }

        uint16_t vbatt = analogRead(A0) * 6200 / 1024; // Rall=300k+220k+100k  ->   max=6200mV at adc=1024(10bit)

        //------get data from DHT------
        float humid = dht.readHumidity();
        float temp = dht.readTemperature();

        // display data in serialmonitor
        Serial.println("Humidity: " + String(humid) + "%  Temperature: " + String(temp) + "°C Vbatt: " + String(vbatt) + " mv");

        // go to deep sleep if low battery
        if (vbatt <= 3900 && vbatt >= 1000) // if <= 1000 is no battery
        // if (vbatt <= 3850)
        {
            vbatt = 0;
            for (int i = 0; i < 20; i++)
            {
                vbatt += analogRead(A0) * 6200 / 1024;
                delay(50);
            }
            vbatt /= 20;
            if ((!digitalRead(D1) && vbatt < 3900) || (digitalRead(D1) && vbatt < 3600))
            {
                pinMode(D4, OUTPUT);
                digitalWrite(D4, LOW);
                delay(2000);
                digitalWrite(D4, HIGH);
                delay(200);
                digitalWrite(D4, LOW);
                delay(2000);
                ESP.deepSleep(10 * 60e6); // sleep for 10 minutes
            }
        }
        /*  4 เมื่อได้ค่าใหม่ ให้อัพเดทตามลำดับตามตัวอย่าง
            ตัวไลบรารี่รวบรวมและหาค่าเฉลี่ยส่งขึ้นเว็บให้เอง
            ถ้าค่าไหนไม่ต้องการส่งค่า ให้กำหนดค่าเป็น NAN   */
        iot.update(humid, temp, vbatt);
    }
}

void updateDHT()
{
}

void handleRoot()
{
    // -- Let IotWebConf test and handle captive portal requests.
    if (iotWebConf.handleCaptivePortal())
    {
        // -- Captive portal request were already served.
        return;
    }
    String s = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
    s += "<title>Iotkiddie smartfarm solar config</title></head><body>IoTkiddie config data";
    s += "<ul>";
    s += "<li>Device name : ";
    s += String(iotWebConf.getThingName());
    s += "<li>อีเมลล์ : ";
    s += emailParamValue;
    s += "<li>WIFI SSID : ";
    s += String(iotWebConf.getSSID());
    s += "<li>RSSI : ";
    s += String(WiFi.RSSI()) + " dBm";
    s += "<li>ESP ID : ";
    s += ESP.getChipId();
    s += "<li>Server : ";
    s += serverParamValue;
    s += "</ul>";
    s += "<button style='margin-top: 10px;' type='button' onclick=\"location.href='/reboot';\" >รีบูทอุปกรณ์</button><br><br>";
    s += "<a href='config'>configure page</a> เพื่อแก้ไขข้อมูล wifi และ user";
    s += "</body></html>\n";

    server.send(200, "text/html", s);
}

void configSaved()
{
    Serial.println("Configuration was updated.");
}

void wifiConnected()
{

    Serial.println("WiFi was connected.");
    MDNS.begin(iotWebConf.getThingName());
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Ready! Open http://%s.local in your browser\n", String(iotWebConf.getThingName()));
    if ((String)emailParamValue != "" && (String)passParamValue != "")
    {
        Serial.println("login");

        // 2 เริ่มเชื่อมต่อ หลังจากต่อไวไฟได้
        if ((String)passParamValue != "")
            iot.begin((String)emailParamValue, (String)passParamValue, (String)serverParamValue);
        else // ถ้าไม่ได้ตั้งค่า server ให้ใช้ค่า default
            iot.begin((String)emailParamValue, (String)passParamValue);
    }
}

bool formValidator(iotwebconf::WebRequestWrapper *webRequestWrapper)
{
    Serial.println("Validating form.");
    bool valid = true;

    /*
      int l = webRequestWrapper->arg(stringParam.getId()).length();
      if (l < 3)
      {
        stringParam.errorMessage = "Please provide at least 3 characters for this test!";
        valid = false;
      }
    */
    return valid;
}

void clearEEPROM()
{
    EEPROM.begin(512);
    // write a 0 to all 512 bytes of the EEPROM
    for (int i = 0; i < 512; i++)
    {
        EEPROM.write(i, 0);
    }

    EEPROM.end();
    server.send(200, "text/plain", "Clear all data\nrebooting");
    delay(1000);
    ESP.restart();
}

void reboot()
{
    server.send(200, "text/plain", "rebooting");
    delay(1000);
    ESP.restart();
}