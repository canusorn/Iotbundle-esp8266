#include <ESP8266WiFi.h>
#include <DHT.h>
#include <iotbundle.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(project)
#define PROJECT "DHT"
Iotbundle iot(PROJECT);

// 1.1.ใส่ข้อมูลไวไฟ
const char *ssid = "G6PD_2.4G";
const char *password = "570610193";

// 1.2.ใส่ข้อมูล user ที่สมัครกับเว็บ iotkiddie.com
String email = "test@iotkiddie.com";
String pass = "12345678";

#define DHTPIN D7
// Uncomment whatever type you're using!
#define DHTTYPE DHT11 // DHT 11
// #define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
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
    iot.begin(email, pass, "http://192.168.2.50");
}

void update_started()
{
    Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished()
{
    Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total)
{
    Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err)
{
    Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
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
        delay(100);
        iot.otaUpdate(String(DHTTYPE), "https://iotkiddie.com/ota/4.3DHT11_iotwebconf.ino.d1_mini.bin");  // addition version (DHT11, DHT22, DHT21)  ,  custom url

        // WiFiClientSecure client;
        // client.setInsecure();
        // t_httpUpdate_return ret = ESPhttpUpdate.update(client, "https://iotkiddie.com/ota/4.3DHT11_iotwebconf.ino.d1_mini.bin");
        // switch (ret)
        // {
        // case HTTP_UPDATE_FAILED:
        //     Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        //     break;

        // case HTTP_UPDATE_NO_UPDATES:
        //     Serial.println("HTTP_UPDATE_NO_UPDATES");
        //     break;

        // case HTTP_UPDATE_OK:
        //     Serial.println("HTTP_UPDATE_OK");
        //     break;
        // }
    }
}