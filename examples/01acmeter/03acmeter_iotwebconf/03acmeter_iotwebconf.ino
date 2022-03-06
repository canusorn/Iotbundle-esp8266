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
#include <EEPROM.h>
#include <iotbundle.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(project)
#define PROJECT "AC_METER"
Iotbundle iot(PROJECT);

#define PIN_RESET -1
#define DC_JUMPER 0

const char thingName[] = "acmeter";
const char wifiInitialApPassword[] = "iotbundle";

#define STRING_LEN 128
#define NUMBER_LEN 32

#define CONFIG_VERSION "0.0.2"

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
char serverParamValue[STRING_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- You can also use namespace formats e.g.: iotwebconf::TextParameter
IotWebConfParameterGroup login = IotWebConfParameterGroup("login", "ล็อกอิน(สมัครที่เว็บก่อนนะครับ)");

IotWebConfTextParameter emailParam = IotWebConfTextParameter("อีเมลล์", "emailParam", emailParamValue, STRING_LEN);
IotWebConfPasswordParameter passParam = IotWebConfPasswordParameter("รหัสผ่าน", "passParam", passParamValue, STRING_LEN);
IotWebConfTextParameter serverParam = IotWebConfTextParameter("เซิฟเวอร์", "serverParam", serverParamValue, STRING_LEN, "https://iotkiddie.com");

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
iotwebconf::NetworkState prev_state = iotwebconf::Boot;
uint8_t displaytime;
String noti;

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

  // for clear eeprom jump D5 to GND
  pinMode(D5, INPUT_PULLUP);
  if (digitalRead(D5) == false)
  {
    delay(1000);
    if (digitalRead(D5) == false)
    {
      oled.clear(PAGE);
      oled.setCursor(0, 0);
      oled.print("Clear All data\n rebooting");
      oled.display();
      delay(1000);
      clearEEPROM();
    }
  }

  login.addItem(&emailParam);
  login.addItem(&passParam);
  login.addItem(&serverParam);

  //  iotWebConf.setStatusPin(STATUS_PIN);
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

  // display status
  iotwebconf::NetworkState curr_state = iotWebConf.getState();
  if (curr_state == iotwebconf::Boot)
  {
    prev_state = curr_state;
  }
  else if (curr_state == iotwebconf::NotConfigured)
  {
    if (prev_state == iotwebconf::Boot)
    {
      displaytime = 5;
      prev_state = curr_state;
      noti = "-State-\n\nno config\nstay in\nAP Mode";
    }
  }
  else if (curr_state == iotwebconf::ApMode)
  {
    if (prev_state == iotwebconf::Boot)
    {
      displaytime = 5;
      prev_state = curr_state;
      noti = "-State-\n\nAP Mode\nfor 30 sec";
    }
    else if (prev_state == iotwebconf::Connecting)
    {
      displaytime = 5;
      prev_state = curr_state;
      noti = "-State-\n\nX  can't\nconnect\nwifi\ngo AP Mode";
    }
    else if (prev_state == iotwebconf::OnLine)
    {
      displaytime = 10;
      prev_state = curr_state;
      noti = "-State-\n\nX  wifi\ndisconnect\ngo AP Mode";
    }
  }
  else if (curr_state == iotwebconf::Connecting)
  {
    if (prev_state == iotwebconf::ApMode)
    {
      displaytime = 5;
      prev_state = curr_state;
      noti = "-State-\n\nwifi\nconnecting";
    }
    else if (prev_state == iotwebconf::OnLine)
    {
      displaytime = 10;
      prev_state = curr_state;
      noti = "-State-\n\nX  wifi\ndisconnect\nreconnecting";
    }
  }
  else if (curr_state == iotwebconf::OnLine)
  {
    if (prev_state == iotwebconf::Connecting)
    {
      displaytime = 5;
      prev_state = curr_state;
      noti = "-State-\n\nwifi\nconnect\nsuccess\n" + String(WiFi.RSSI()) + " dBm";
    }
  }

  if (displaytime)
  {
    displaytime--;
    oled.clear(PAGE);
    oled.setCursor(0, 0);
    oled.print(noti);
    Serial.println(noti);
  }

  // display state
  if (curr_state == iotwebconf::NotConfigured || curr_state == iotwebconf::ApMode)
    oled.drawIcon(55, 0, 9, 8, wifi_ap, sizeof(wifi_ap), true);
  else if (curr_state == iotwebconf::Connecting)
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
  else if (curr_state == iotwebconf::OnLine)
  {
    if (iot.serverConnected)
      oled.drawIcon(56, 0, 8, 8, wifi_on, sizeof(wifi_on), true);
    else
      oled.drawIcon(56, 0, 8, 8, wifi_nointernet, sizeof(wifi_nointernet), true);
  }
  else if (curr_state == iotwebconf::OffLine)
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
  s += "<title>Iotkiddie AC Powermeter config</title>";
  if (iotWebConf.getState() == iotwebconf::NotConfigured)
    s += "<script>\nlocation.href='/config';\n</script>";
  s += "</head><body>IoTkiddie config data";
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