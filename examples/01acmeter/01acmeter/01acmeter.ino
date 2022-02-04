/*
   -PZEM004T-
   5V - 5V
   GND - GND
   D3 - TX
   D4 - RX
*/

#include <ESP8266WiFi.h>
#include <PZEM004Tv30.h>
#include <iotbundle.h>

// 1 สร้าง object ชื่อ iot และกำหนดค่า(server,project)
Iotbundle iot("IOTKID", "AC_METER");

const char *ssid = "G6PD_2.4G";
const char *password = "570610193";

String email = "anusorn1998@gmail.com";
String pass = "vo6liIN";

PZEM004Tv30 pzem(D3, D4);             //rx,tx pin

unsigned long previousMillis = 0;
float voltage, current, power, energy, frequency, pf;

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

  //  pzem.resetEnergy(); //reset energy

  // 2 เริ่มเชื่อมต่อ หลังจากต่อไวไฟได้
  iot.begin(email, pass);
}

void loop()
{
  // 3 คอยจัดการ และส่งค่าให้เอง
  iot.handle();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000)
  { //run every 1 second
    previousMillis = currentMillis;

   //------read data------
  voltage = pzem.voltage();
  current = pzem.current();
  power = pzem.power();
  energy = pzem.energy();
  frequency = pzem.frequency();
  pf = pzem.pf();

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

    // 4 เมื่อได้ค่าใหม่ ให้อัพเดท ตัวไลบรารี่รวบรวมและหาค่าเฉลี่ยส่งขึ้นเว็บให้เอง
    iot.update(voltage, current, power, energy, frequency, pf);
  }
}