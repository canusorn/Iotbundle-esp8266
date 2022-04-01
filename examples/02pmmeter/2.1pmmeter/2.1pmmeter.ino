/*
   -PMS7003-
   5V - VCC
   GND - GND
   D4  - TX
   D3  - RX(not use in this code)

*/

#include <ESP8266WiFi.h>
#include <PMS.h>
#include <SoftwareSerial.h>
#include <iotbundle.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(project)
#define PROJECT "PM_METER"
Iotbundle iot(PROJECT);

// 1.1.ใส่ข้อมูลไวไฟ
const char *ssid = "wifi_ssid";
const char *password = "wifi_pass";

// 1.2.ใส่ข้อมูล user ที่สมัครกับเว็บ iotkiddie.com
String email = "test@iotkiddie.com";
String pass = "12345678";

SoftwareSerial pmsSerial(D4, D3); // RX,TX
PMS pms(pmsSerial);
PMS::DATA data;

void setup()
{
    Serial.begin(115200);
    pmsSerial.begin(9600);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // 2 เริ่มเชื่อมต่อ หลังจากต่อไวไฟได้
    iot.begin(email, pass);
}

void loop()
{
    // 3 คอยจัดการ และส่งค่าให้เอง
    iot.handle();

    //------get data from PMS7003------
    if (pms.read(data))
    {
        /*  4 เมื่อได้ค่าใหม่ ให้อัพเดทตามลำดับตามตัวอย่าง
            ตัวไลบรารี่รวบรวมและหาค่าเฉลี่ยส่งขึ้นเว็บให้เอง
            ถ้าค่าไหนไม่ต้องการส่งค่า ให้กำหนดค่าเป็น NAN   */
        iot.update(data.PM_AE_UG_1_0, data.PM_AE_UG_2_5, data.PM_AE_UG_10_0);

        //------print on serial moniter------
        Serial.print("PM 1.0 (ug/m3): ");
        Serial.println(data.PM_AE_UG_1_0);
        Serial.print("PM 2.5 (ug/m3): ");
        Serial.println(data.PM_AE_UG_2_5);
        Serial.print("PM 10.0 (ug/m3): ");
        Serial.println(data.PM_AE_UG_10_0);
    }
}