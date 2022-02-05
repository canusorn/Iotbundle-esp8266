/*
  -PZEM004T-
  5V - 5V
  GND - GND
  D3 - TX
  D4 - RX

  state from iotwebconf
  0 Boot,
  1 NotConfigured,
  2 ApMode,
  3 Connecting,
  4 OnLine,
  5 OffLine

*/

#include <ESP8266WiFi.h>
#include <PZEM004Tv30.h>
#include <Wire.h>          // Include Wire if you're using I2C
#include <SFE_MicroOLED.h> // Include the SFE_MicroOLED library
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <iotbundle.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(server,project)
Iotbundle iot("IOTKID", "AC_METER");

#define PIN_RESET -1
#define DC_JUMPER 0

const char thingName[] = "acmeter";
const char wifiInitialApPassword[] = "iotbundle";

#define STRING_LEN 128
#define NUMBER_LEN 32

#define CONFIG_VERSION "0.01"
#define CONFIG_PIN D5
//#define IOTWEBCONF_CONFIG_USE_MDNS 80
//#define STATUS_PIN LED_BUILTIN

// -- Method declarations.
void handleRoot();
// -- Callback methods.
void wifiConnected();
void configSaved();
bool formValidator(iotwebconf::WebRequestWrapper *webRequestWrapper);

DNSServer dnsServer;
WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

char emailParamValue[STRING_LEN];
char passParamValue[STRING_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- You can also use namespace formats e.g.: iotwebconf::TextParameter
IotWebConfParameterGroup login = IotWebConfParameterGroup("login", "ล็อกอิน(สมัครที่เว็บก่อนนะครับ)");

IotWebConfTextParameter emailParam = IotWebConfTextParameter("อีเมลล์", "emailParam", emailParamValue, STRING_LEN);
IotWebConfPasswordParameter passParam = IotWebConfPasswordParameter("รหัสผ่าน", "passParam", passParamValue, STRING_LEN);

MicroOLED oled(PIN_RESET, DC_JUMPER); // Example I2C declaration, uncomment if using I2C
PZEM004Tv30 pzem(D3, D4);             // rx,tx pin

unsigned long previousMillis = 0;
float voltage, current, power, energy, frequency, pf;

uint8_t logo_bmp[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xC0, 0xF0, 0xE0, 0x78, 0x38, 0x78, 0x3C, 0x1C, 0x3C, 0x1C, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1C, 0x3C, 0x1C, 0x3C, 0x78, 0x38, 0xF0, 0xE0, 0xF0, 0xC0, 0xC0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x01, 0x00, 0x00, 0xF0, 0xF8, 0x70, 0x3C, 0x3C, 0x1C, 0x1E, 0x1E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0E, 0x0E, 0x1E, 0x1E, 0x1E, 0x3C, 0x1C, 0x7C, 0x70, 0xF0, 0x70, 0x20, 0x01, 0x01, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x1C, 0x3E, 0x1E, 0x0F, 0x0F, 0x07, 0x87, 0x87, 0x07, 0x0F, 0x0F, 0x1E, 0x3E, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1F, 0x1F, 0x3F, 0x3F, 0x1F, 0x1F, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t wifi_on[] = {0x08, 0x04, 0x12, 0xCA, 0xCA, 0x12, 0x04, 0x08};
uint8_t wifi_off[] = {0x88, 0x44, 0x32, 0xDA, 0xCA, 0x16, 0x06, 0x09};
uint8_t wifi_ap[] = {0x3E, 0x41, 0x1C, 0x00, 0xF8, 0x00, 0x1C, 0x41, 0x3E};
uint8_t wifi_nointernet[] = {0x04, 0x12, 0xCA, 0xCA, 0x12, 0x04, 0x5F, 0xDF};
uint8_t t_connecting;

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  //------Display LOGO at start------
  oled.begin();
  oled.clear(PAGE);
  oled.clear(ALL);
  oled.drawBitmap(logo_bmp); // call the drawBitmap function and pass it the array from above
  oled.setFontType(0);
  oled.setCursor(0, 36);
  oled.print(" IoTbundle");
  oled.display();

  login.addItem(&emailParam);
  login.addItem(&passParam);

  //  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
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
  server.onNotFound([]()
                    { iotWebConf.handleNotFound(); });

  Serial.println("Ready.");

  //  pzem.resetEnergy(); //reset energy
  // Serial.println(ESP.getChipId());
}

void loop()
{
  iotWebConf.doLoop();
  server.handleClient();
  MDNS.update();

  // 3 คอยจัดการ และส่งค่าให้เอง
  iot.handle();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000)
  { // run every 1 second
    previousMillis = currentMillis;
    displayValue(); // update OLED

    /*  4 เมื่อได้ค่าใหม่ ให้อัพเดทตามลำดับตามตัวอย่าง
    ตัวไลบรารี่รวบรวมและหาค่าเฉลี่ยส่งขึ้นเว็บให้เอง
    ถ้าค่าไหนไม่ต้องการส่งค่า ให้กำหนดค่าเป็น NAN
    เช่น ต้องการส่งแค่ voltage current power
    iot.update(voltage, current, power, NAN, NAN, NAN);    */
    iot.update(voltage, current, power, energy, frequency, pf);
  }
}

void displayValue()
{
  //------read data------
  voltage = pzem.voltage();
  if (!isnan(voltage))
  { // ถ้าอ่านค่าได้
    current = pzem.current();
    power = pzem.power();
    energy = pzem.energy();
    frequency = pzem.frequency();
    pf = pzem.pf();
  }
  else
  { // ถ้าอ่านค่าไม่ได้ให้ใส่ค่า NAN(not a number)
    current = NAN;
    power = NAN;
    energy = NAN;
    frequency = NAN;
    pf = NAN;
  }

  //------Update OLED display------
  oled.clear(PAGE);
  oled.setFontType(0);

  // display voltage
  oled.setCursor(3, 0);
  oled.print(voltage, 1);
  oled.setCursor(42, 0);
  oled.println("V");

  // display current
  if (current < 10)
    oled.setCursor(9, 12);
  else
    oled.setCursor(3, 12);
  oled.print(current, 2);
  oled.setCursor(42, 12);
  oled.println("A");

  // display power
  if (power < 10)
    oled.setCursor(26, 24);
  else if (power < 100)
    oled.setCursor(20, 24);
  else if (power < 1000)
    oled.setCursor(14, 24);
  else if (power < 10000)
    oled.setCursor(8, 24);
  else
    oled.setCursor(2, 24);
  oled.print(power, 0);
  oled.setCursor(42, 24);
  oled.println("W");

  // display energy
  oled.setCursor(3, 36);
  if (energy < 10)
    oled.print(energy, 3);
  else if (energy < 100)
    oled.print(energy, 2);
  else if (energy < 1000)
    oled.print(energy, 1);
  else
  {
    oled.setCursor(8, 36);
    oled.print(energy, 0);
  }
  oled.setCursor(42, 36);
  oled.println("kWh");

  // on error
  if (isnan(voltage))
  {
    oled.clear(PAGE);
    oled.setCursor(0, 0);
    oled.printf("Please\n\nConnect\n\nPZEM004T");
  }

  // display state
  if (iotWebConf.getState() == 1 || iotWebConf.getState() == 2)
    oled.drawIcon(55, 0, 9, 8, wifi_ap, sizeof(wifi_ap), true);
  else if (iotWebConf.getState() == 3)
  {
    if (t_connecting == 1)
    {
      oled.drawIcon(56, 0, 8, 8, wifi_on, sizeof(wifi_on), true);
      t_connecting = 0;
    }
    else
    {
      t_connecting = 1;
    }
  }
  else if (iotWebConf.getState() == 4)
  {
    if (iot.serverConnected)
      oled.drawIcon(56, 0, 8, 8, wifi_on, sizeof(wifi_on), true);
    else
      oled.drawIcon(56, 0, 8, 8, wifi_nointernet, sizeof(wifi_nointernet), true);
  }
  else if (iotWebConf.getState() == 5)
    oled.drawIcon(56, 0, 8, 8, wifi_off, sizeof(wifi_off), true);

  oled.display();

  //------Serial display------
  if (!isnan(voltage))
  {
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");
    Serial.print("Energy: ");
    Serial.print(energy, 3);
    Serial.println("kWh");
    Serial.print("Frequency: ");
    Serial.print(frequency, 1);
    Serial.println("Hz");
    Serial.print("PF: ");
    Serial.println(pf);
  }
  else
  {
    Serial.println("No sensor detect");
  }
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
  s += "<title>Iotbundle AC Powermeter config</title></head><body>สวัสดีชาวโลก";
  s += "<ul>";
  s += "<li>อีเมลล์ : ";
  s += emailParamValue;
  s += "<li>ESP ID : ";
  s += ESP.getChipId();
  s += "</ul>";
  s += "Go to <a href='config'>configure page</a> to change values.";
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