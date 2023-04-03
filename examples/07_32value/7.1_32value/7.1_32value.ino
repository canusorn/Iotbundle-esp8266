
#include <ESP8266WiFi.h>
#include <iotbundle32.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(project)
#define PROJECT "32_VALUE"
Iotbundle32 iot(PROJECT);

// 1.1.ใส่ข้อมูลไวไฟ
const char *ssid = "wifi_ssid";
const char *password = "wifi_pass";

// 1.2.ใส่ข้อมูล user ที่สมัครกับเว็บ iotkiddie.com
String email = "test@iotkiddie.com";
String pass = "12345678";

unsigned long previousMillis = 0;

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    // 2 เริ่มเชื่อมต่อ หลังจากต่อไวไฟได้
    iot.begin(email, pass,"http://192.168.0.100");
}

void loop()
{
    // 3 คอยจัดการ และส่งค่าให้เอง
    iot.handle();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000)
    { // run every 1 second
        previousMillis = currentMillis;

       int variable1 = random(100);
       float variable2 = random(100)/10;

        /*  4 เมื่อได้ค่าใหม่ ให้อัพเดทตามลำดับตามที่ตกลงกับทางเว็บไว้แล้ว
        ตัวไลบรารี่รวบรวมและหาค่าเฉลี่ยส่งขึ้นเว็บให้เอง */
        iot.update32value(variable1, variable2);

        //------Serial display------
        Serial.print("variable1: ");
        Serial.print(variable1);
        Serial.print("\tvariable2: ");
        Serial.println(variable2);
    }
}