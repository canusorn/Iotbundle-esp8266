#include <ESP8266WiFi.h>
#include <DHT.h>
#include <iotbundle.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(project)
#define PROJECT "DHT"
Iotbundle iot(PROJECT);

// 1.1.ใส่ข้อมูลไวไฟ
const char *ssid = "wifi_ssid";
const char *password = "wifi_pass";

// 1.2.ใส่ข้อมูล user ที่สมัครกับเว็บ iotkiddie.com
String email = "test@iotkiddie.com";
String pass = "12345678";

#define DHTPIN D7
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;

void setup()
{
    Serial.begin(115200);
    dht.begin();

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

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 2000)
    { // run every 2 second
        previousMillis = currentMillis;
        //------get data from DHT------
        float humid = dht.readHumidity();
        float temp = dht.readTemperature();

        // display data in serialmonitor
        Serial.println("Humidity: " + String(humid) + "%  Temperature: " + String(temp) + "°C ");

        /*  4 เมื่อได้ค่าใหม่ ให้อัพเดทตามลำดับตามตัวอย่าง
            ตัวไลบรารี่รวบรวมและหาค่าเฉลี่ยส่งขึ้นเว็บให้เอง
            ถ้าค่าไหนไม่ต้องการส่งค่า ให้กำหนดค่าเป็น NAN   */
        iot.update(humid, temp);
    }
}