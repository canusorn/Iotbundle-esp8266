/*
    PZEM-017 DC Power Meter

    Note :  โปรดระมัดระวังอันตรายจากการต่อไฟฟ้า และอุปกรณ์ที่อาจเสียหายจากการต่อใช้งาน ทางเราไม่รับผิดชอบในความเสียหาย สูญเสีย ไม่ว่าทางตรงหรือทางอ้อม หรือค่าใช้จ่ายใดๆ
    Note :  โค๊ดต้นฉบับตัวอย่างจากเว็บนี้ : https://solarduino.com/pzem-017-dc-energy-meter-online-monitoring-with-blynk-app/
    Note :  และนำมาดัดแปลงโดย https://www.IoTbundle.com
-----------------------------------------------------------------------------------------------------------------------------------------------
    การใช้งาน
    โค๊ดนี้จะต่อใช้งานบอร์ด Wemos(ESP8266) กับ เซนเซอร์วัดไฟฟ้ากระแสตรง PZEM-017 โดยสื่อสารกับด้วยโมดูล UART TTL to RS485(MAX485) ชนิด 4 พิน (DI DE Re & RO)
    ------------------      --------------------------------------      ------------
    | wemos(ESP8266) |  ->  | UART TTL to RS485 converter module |  ->  | PZEM-017 |
    ------------------      --------------------------------------      ------------
-----------------------------------------------------------------------------------------------------------------------------------------------
    การต่อสาย

    VCC            5V
    GND            GND
     A             A
     B             B
-----------------------------------------------------------------------------------------------------------------------------------------------
    การแก้ไขโค๊ด
    - ติดตั้ง ESP8266 จาก Board manager
    - ติดตั้ง Library "ModbusMaster" จาก Library manager
    - ติดตั้ง Library "SparkFun Micro OLED Breakout" จาก Library manager
    - ตั้งค่า Shunt ให้ถูกรุ่นที่ตัวแปร "NewshuntAddr"
-----------------------------------------------------------------------------------------------------------------------------------------------
*/

/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/ ////////////*/
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <SFE_MicroOLED.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <iotbundle.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(project)
#define PROJECT "DC_METER"
Iotbundle iot(PROJECT);

SoftwareSerial PZEMSerial;
MicroOLED oled(-1, 0);

// ตั้งค่า pin สำหรับต่อกับ MAX485
#define MAX485_RO D4
#define MAX485_DI D3

// Address ของ PZEM-017 : 0x01-0xF7
static uint8_t pzemSlaveAddr = 0x01;

// ตั้งค่า shunt -->> 0x0000-100A, 0x0001-50A, 0x0002-200A, 0x0003-300A
static uint16_t NewshuntAddr = 0x0001;

const char thingName[] = "dcmeter";
const char wifiInitialApPassword[] = "iotbundle";

#define STRING_LEN 128
#define NUMBER_LEN 32

// timer interrupt
Ticker timestamp;

ModbusMaster node;

float PZEMVoltage, PZEMCurrent, PZEMPower, PZEMEnergy;

unsigned long startMillisPZEM;         // start counting time for LCD Display */
unsigned long currentMillisPZEM;       // current counting time for LCD Display */
const unsigned long periodPZEM = 1000; // refresh every X seconds (in seconds) in LED Display. Default 1000 = 1 second
unsigned long startMillis1;            // to count time during initial start up (PZEM Software got some error so need to have initial pending time)

DNSServer dnsServer;
WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

char emailParamValue[STRING_LEN];
char passParamValue[STRING_LEN];
char serverParamValue[STRING_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, VERSION); // version defind in iotbundle.h file
// -- You can also use namespace formats e.g.: iotwebconf::TextParameter
IotWebConfParameterGroup login = IotWebConfParameterGroup("login", "ล็อกอิน(สมัครที่เว็บก่อนนะครับ)");

IotWebConfTextParameter emailParam = IotWebConfTextParameter("อีเมลล์ (ระวังห้ามใส่เว้นวรรค)", "emailParam", emailParamValue, STRING_LEN);
IotWebConfPasswordParameter passParam = IotWebConfPasswordParameter("รหัสผ่าน", "passParam", passParamValue, STRING_LEN);
IotWebConfTextParameter serverParam = IotWebConfTextParameter("เซิฟเวอร์", "serverParam", serverParamValue, STRING_LEN, "https://iotkiddie.com");

// -- Method declarations.
void handleRoot();
// -- Callback methods.
void wifiConnected();
void configSaved();
bool formValidator(iotwebconf::WebRequestWrapper *webRequestWrapper);

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
uint16_t timer_nointernet;

// timer interrupt every 1 second
void time1sec()
{
    iot.interrupt1sec();

    // if can't connect to network
    if (iotWebConf.getState() == iotwebconf::OnLine)
    {
        if (iot.serverConnected)
        {
            timer_nointernet = 0;
        }
        else
        {
            timer_nointernet++;
            if (timer_nointernet > 30)
                Serial.println("No connection time : " + String(timer_nointernet));
        }
    }

    // reconnect wifi if can't connect server
    if (timer_nointernet == 60)
    {
        Serial.println("Can't connect to server -> Restart wifi");
        iotWebConf.goOffLine();
        timer_nointernet++;
    }
    else if (timer_nointernet >= 65)
    {
        timer_nointernet = 0;
        iotWebConf.goOnLine(false);
    }
    else if (timer_nointernet >= 61)
        timer_nointernet++;
}

void setup()
{
    startMillis1 = millis();
    Serial.begin(115200);

    // for clear eeprom jump D5 to GND
    // pinMode(D5, INPUT_PULLUP);
    // if (digitalRead(D5) == false)
    // {
    //     delay(1000);
    //     if (digitalRead(D5) == false)
    //     {
    //         oled.clear(PAGE);
    //         oled.setCursor(0, 0);
    //         oled.print("Clear All data\n rebooting");
    //         oled.display();
    //         delay(1000);
    //         clearEEPROM();
    //     }
    // }

    PZEMSerial.begin(9600, SWSERIAL_8N2, MAX485_RO, MAX485_DI); // software serial สำหรับติดต่อกับ MAX485

    // timer interrupt every 1 sec
    timestamp.attach(1, time1sec);

    startMillisPZEM = millis(); /* Start counting time for run code */

    //------Display LOGO at start------
    Wire.begin();
    oled.begin();
    oled.clear(PAGE);
    oled.clear(ALL);
    oled.drawBitmap(logo_bmp); // call the drawBitmap function and pass it the array from above
    oled.setFontType(0);
    oled.setCursor(0, 36);
    oled.print(" IoTbundle");
    oled.display();

    node.begin(pzemSlaveAddr, PZEMSerial);

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

    Serial.println("ESPID: " + String(ESP.getChipId()));
    Serial.println("Ready.");

    // รอครบ 5 วินาที แล้วตั้งค่า shunt และ address
    oled.clear(PAGE);
    oled.setCursor(0, 0);
    oled.print("Setting\nsensor");
    oled.display();
    Serial.print("Setting PZEM 017 ");
    while (millis() - startMillis1 < 5000)
    {
        delay(500);
        Serial.print(".");
        oled.print(".");
        oled.display();
    }
    setShunt(pzemSlaveAddr);            // ตั้งค่า shunt
    changeAddress(0xF8, pzemSlaveAddr); // ตั้งค่า address 0x01 ซื่งเป็นค่า default ของตัว PZEM-017
    // resetEnergy();                                   // รีเซ็ตค่า Energy[Wh] (หน่วยใช้ไฟสะสม)

        // set which pin can change
    iot.setAllowIO(0b111100001);
}

void loop()
{
    iotWebConf.doLoop();
    server.handleClient();
    MDNS.update();

    // 3 คอยจัดการ และส่งค่าให้เอง
    iot.handle();

    currentMillisPZEM = millis();
    // อ่านค่าจาก PZEM-017
    if (currentMillisPZEM - startMillisPZEM >= periodPZEM) /* for every x seconds, run the codes below*/
    {
        uint8_t result;                              /* Declare variable "result" as 8 bits */
        result = node.readInputRegisters(0x0000, 6); /* read the 9 registers (information) of the PZEM-014 / 016 starting 0x0000 (voltage information) kindly refer to manual)*/
        if (result == node.ku8MBSuccess)             /* If there is a response */
        {
            uint32_t tempdouble = 0x00000000;                     /* Declare variable "tempdouble" as 32 bits with initial value is 0 */
            PZEMVoltage = node.getResponseBuffer(0x0000) / 100.0; /* get the 16bit value for the voltage value, divide it by 100 (as per manual) */
            // 0x0000 to 0x0008 are the register address of the measurement value
            PZEMCurrent = node.getResponseBuffer(0x0001) / 100.0; /* get the 16bit value for the current value, divide it by 100 (as per manual) */

            tempdouble = (node.getResponseBuffer(0x0003) << 16) + node.getResponseBuffer(0x0002); /* get the power value. Power value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
            PZEMPower = tempdouble / 10.0;                                                        /* Divide the value by 10 to get actual power value (as per manual) */

            tempdouble = (node.getResponseBuffer(0x0005) << 16) + node.getResponseBuffer(0x0004); /* get the energy value. Energy value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
            PZEMEnergy = tempdouble;
            PZEMEnergy /= 1000; // to kWh

            /*  4 เมื่อได้ค่าใหม่ ให้อัพเดทตามลำดับตามตัวอย่าง
            ตัวไลบรารี่รวบรวมและหาค่าเฉลี่ยส่งขึ้นเว็บให้เอง*/
            iot.update(PZEMVoltage, PZEMCurrent, PZEMPower, PZEMEnergy);
        }
        else // ถ้าติดต่อ PZEM-017 ไม่ได้ ให้ใส่ค่า NAN(Not a Number)
        {
            PZEMVoltage = NAN;
            PZEMCurrent = NAN;
            PZEMPower = NAN;
            PZEMEnergy = NAN;
        }

        // แสดงค่าที่ได้จากบน Serial monitor
        Serial.print("Vdc : ");
        Serial.print(PZEMVoltage);
        Serial.println(" V ");
        Serial.print("Idc : ");
        Serial.print(PZEMCurrent);
        Serial.println(" A ");
        Serial.print("Power : ");
        Serial.print(PZEMPower);
        Serial.println(" W ");
        Serial.print("Energy : ");
        Serial.print(PZEMEnergy);
        Serial.println(" kWh ");

        // แสดงค่าบน OLED
        display_update();

        if (iot.need_ota)
            iot.otaUpdate(); // addition version (DHT11, DHT22, DHT21)  ,  custom url

        startMillisPZEM = currentMillisPZEM; /* Set the starting point again for next counting time */
    }
}

// ---- ฟังก์ชันแสดงผลฝ่านจอ OLED ----
void display_update()
{
    //------Update OLED------
    oled.clear(PAGE);
    oled.setFontType(0);
    oled.setCursor(0, 0);
    oled.print(PZEMVoltage);
    oled.println(" V");
    oled.setCursor(0, 12);
    oled.print(PZEMCurrent);
    oled.println(" A");
    oled.setCursor(0, 24);
    oled.print(PZEMPower);
    oled.println(" W");
    oled.setCursor(0, 36);

    if (PZEMEnergy < 9.999)
    {
        oled.print(PZEMEnergy, 3);
        oled.println(" kWh");
    }
    else if (PZEMEnergy < 99.999)
    {
        oled.print(PZEMEnergy, 2);
        oled.println(" kWh");
    }
    else if (PZEMEnergy < 999.999)
    {
        oled.print(PZEMEnergy, 1);
        oled.println(" kWh");
    }
    else if (PZEMEnergy < 9999.999)
    {
        oled.print(PZEMEnergy, 0);
        oled.println(" kWh");
    }
    else
    { // ifnan
        oled.print(PZEMEnergy, 0);
        oled.println(" Wh");
    }

    // on error
    //   if (isnan(PZEMVoltage))
    //   {
    //     oled.clear(PAGE);
    //     oled.setCursor(0, 0);
    //     oled.printf("-Sensor-\n\nno sensor\ndetect!");
    //   }

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

    if (iot.noti != "" && displaytime == 0)
    {
        displaytime = 3;
        noti = iot.noti;
        iot.noti = "";
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
        {
            oled.drawIcon(56, 0, 8, 8, wifi_on, sizeof(wifi_on), true);
        }
        else
        {
            oled.drawIcon(56, 0, 8, 8, wifi_nointernet, sizeof(wifi_nointernet), true);
        }
    }
    else if (curr_state == iotwebconf::OffLine)
        oled.drawIcon(56, 0, 8, 8, wifi_off, sizeof(wifi_off), true);

    oled.display();
}

void setShunt(uint8_t slaveAddr) // Change the slave address of a node
{

    /* 1- PZEM-017 DC Energy Meter */

    static uint8_t SlaveParameter = 0x06;     /* Write command code to PZEM */
    static uint16_t registerAddress = 0x0003; /* change shunt register address command code */

    uint16_t u16CRC = 0xFFFF;                 /* declare CRC check 16 bits*/
    u16CRC = crc16_update(u16CRC, slaveAddr); // Calculate the crc16 over the 6bytes to be send
    u16CRC = crc16_update(u16CRC, SlaveParameter);
    u16CRC = crc16_update(u16CRC, highByte(registerAddress));
    u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
    u16CRC = crc16_update(u16CRC, highByte(NewshuntAddr));
    u16CRC = crc16_update(u16CRC, lowByte(NewshuntAddr));
    PZEMSerial.write(slaveAddr); /* these whole process code sequence refer to manual*/
    PZEMSerial.write(SlaveParameter);
    PZEMSerial.write(highByte(registerAddress));
    PZEMSerial.write(lowByte(registerAddress));
    PZEMSerial.write(highByte(NewshuntAddr));
    PZEMSerial.write(lowByte(NewshuntAddr));
    PZEMSerial.write(lowByte(u16CRC));
    PZEMSerial.write(highByte(u16CRC));
    delay(100);
}

void resetEnergy() // reset energy for Meter 1
{
    uint16_t u16CRC = 0xFFFF;           /* declare CRC check 16 bits*/
    static uint8_t resetCommand = 0x42; /* reset command code*/
    uint8_t slaveAddr = pzemSlaveAddr;  // if you set different address, make sure this slaveAddr must change also
    u16CRC = crc16_update(u16CRC, slaveAddr);
    u16CRC = crc16_update(u16CRC, resetCommand);
    PZEMSerial.write(slaveAddr);        /* send device address in 8 bit*/
    PZEMSerial.write(resetCommand);     /* send reset command */
    PZEMSerial.write(lowByte(u16CRC));  /* send CRC check code low byte  (1st part) */
    PZEMSerial.write(highByte(u16CRC)); /* send CRC check code high byte (2nd part) */
    delay(100);
}

void changeAddress(uint8_t OldslaveAddr, uint8_t NewslaveAddr) // Change the slave address of a node
{

    /* 1- PZEM-017 DC Energy Meter */

    static uint8_t SlaveParameter = 0x06;        /* Write command code to PZEM */
    static uint16_t registerAddress = 0x0002;    /* Modbus RTU device address command code */
    uint16_t u16CRC = 0xFFFF;                    /* declare CRC check 16 bits*/
    u16CRC = crc16_update(u16CRC, OldslaveAddr); // Calculate the crc16 over the 6bytes to be send
    u16CRC = crc16_update(u16CRC, SlaveParameter);
    u16CRC = crc16_update(u16CRC, highByte(registerAddress));
    u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
    u16CRC = crc16_update(u16CRC, highByte(NewslaveAddr));
    u16CRC = crc16_update(u16CRC, lowByte(NewslaveAddr));
    PZEMSerial.write(OldslaveAddr); /* these whole process code sequence refer to manual*/
    PZEMSerial.write(SlaveParameter);
    PZEMSerial.write(highByte(registerAddress));
    PZEMSerial.write(lowByte(registerAddress));
    PZEMSerial.write(highByte(NewslaveAddr));
    PZEMSerial.write(lowByte(NewslaveAddr));
    PZEMSerial.write(lowByte(u16CRC));
    PZEMSerial.write(highByte(u16CRC));
    delay(100);
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
    s += "<li>Version : ";
    s += VERSION;
    s += "</ul>";
    s += "<button style='margin-top: 10px;' type='button' onclick=\"location.href='/reboot';\" >รีบูทอุปกรณ์</button><br><br>";
    s += "<a href='config'>configure page แก้ไขข้อมูล wifi และ user</a>";
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

    // if (l < 3)
    // {
    //   emailParam.errorMessage = "Please provide at least 3 characters for this test!";
    //   valid = false;
    // }

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