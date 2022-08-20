
#include <PZEM004Tv30.h>

PZEM004Tv30 pzem(D3, D4);

/*******************************************
   Set your address over here..
   The address can be between 0x01 and 0xF7
 *******************************************/
#define SET_ADDRESS 0x02

void setup()
{
    Serial.begin(115200);

    // Print out current custom address
    Serial.print("Previous address:   0x");
    Serial.println(pzem.readAddress(), HEX);

    static uint8_t addr = SET_ADDRESS;
    Serial.print("Setting address to: 0x");
    Serial.println(addr, HEX);

    if (!pzem.setAddress(addr))
    {
        // Setting custom address failed. Probably no PZEM connected
        Serial.println("Error setting address.");
    }
    else
    {
        // Print out the new custom address
        Serial.print("Current address:    0x");
        Serial.println(pzem.readAddress(), HEX);
        Serial.println();
    }
}

void loop()
{
    delay(5000);
}