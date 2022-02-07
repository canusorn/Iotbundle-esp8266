/*
   -PZEM004T-
   5V - 5V
   GND - GND
   D3 - TX
   D4 - RX
*/

#include <ESP8266WiFi.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <SFE_MicroOLED.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <iotbundle.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(server,project)
#define PROJECT "AC_METER"
#define SERVER "https://iotkiddie.com"
Iotbundle iot(PROJECT);

#define PIN_RESET -1
#define DC_JUMPER 0

const char *ssid = "G6PD_2.4G";
const char *password = "570610193";
const char *host = "powermeter-test";

String email = "anusorn1998@gmail.com";
String pass = "vo6liIN";

MicroOLED oled(PIN_RESET, DC_JUMPER); // Example I2C declaration, uncomment if using I2C
PZEM004Tv30 pzem(D3, D4);             // rx,tx pin

unsigned long previousMillis = 0;
float voltage, current, power, energy, frequency, pf;

uint8_t logo_bmp[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xC0, 0xF0, 0xE0, 0x78, 0x38, 0x78, 0x3C, 0x1C, 0x3C, 0x1C, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1C, 0x3C, 0x1C, 0x3C, 0x78, 0x38, 0xF0, 0xE0, 0xF0, 0xC0, 0xC0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x01, 0x00, 0x00, 0xF0, 0xF8, 0x70, 0x3C, 0x3C, 0x1C, 0x1E, 0x1E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0E, 0x0E, 0x1E, 0x1E, 0x1E, 0x3C, 0x1C, 0x7C, 0x70, 0xF0, 0x70, 0x20, 0x01, 0x01, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x1C, 0x3E, 0x1E, 0x0F, 0x0F, 0x07, 0x87, 0x87, 0x07, 0x0F, 0x0F, 0x1E, 0x3E, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1F, 0x1F, 0x3F, 0x3F, 0x1F, 0x1F, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

ESP8266WebServer server(80);
String serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form><p>ESP ID : " + String(ESP.getChipId()) + "</p>";

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

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.waitForConnectResult() == WL_CONNECTED)
  {
    MDNS.begin(host);
    server.on("/", HTTP_GET, []()
              {
                server.sendHeader("Connection", "close");
                server.send(200, "text/html", serverIndex); });
    server.on(
        "/update", HTTP_POST, []()
        {
          server.sendHeader("Connection", "close");
          server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
          ESP.restart(); },
        []()
        {
          HTTPUpload &upload = server.upload();
          if (upload.status == UPLOAD_FILE_START)
          {
            Serial.setDebugOutput(true);
            WiFiUDP::stopAll();
            Serial.printf("Update: %s\n", upload.filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace))
            { // start with max available size
              Update.printError(Serial);
            }
          }
          else if (upload.status == UPLOAD_FILE_WRITE)
          {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            {
              Update.printError(Serial);
            }
          }
          else if (upload.status == UPLOAD_FILE_END)
          {
            if (Update.end(true))
            { // true to set the size to the current progress
              Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            }
            else
            {
              Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
          }
          yield();
        });
    server.begin();
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  }
  else
  {
    Serial.println("WiFi Failed");
  }

  //  pzem.resetEnergy(); //reset energy

  // 2 เริ่มเชื่อมต่อ หลังจากต่อไวไฟได้
  iot.begin(email, pass, SERVER);
}

void loop()
{
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
  oled.setCursor(46, 0);
  oled.println("V");

  // display current
  if (current < 10)
    oled.setCursor(9, 12);
  else
    oled.setCursor(3, 12);
  oled.print(current, 2);
  oled.setCursor(46, 12);
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
  oled.setCursor(46, 24);
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
  oled.setCursor(46, 36);
  oled.println("kWh");

  // on error
  if (isnan(voltage))
  {
    oled.clear(PAGE);
    oled.setCursor(0, 0);
    oled.printf("Please\n\nConnect\n\nPZEM004T");
  }
  oled.display();

  //------Serial display------
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