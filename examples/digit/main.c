#include <Arduino.h>

uint8_t digits []={0xFC,0x60,0xDA,0xF2,0x66,0xB6,0xBE,0xE0,0xFE,0xF6};
uint8_t  led2pin[]={9, 8, 7,10,11,12,13, 6};
void displayDigit(int d)
{
	byte code = digits[d%10];
	for (int c = 7; c >= 0; c--)
	{   
		digitalWrite(led2pin[c], code%2 ? HIGH : LOW);
		code >>= 1;
	}   
}

void setup()
{
	for (uint8_t i = 6; i <= 13; i++)
		pinMode(i, OUTPUT);
}
void loop()
{
	displayDigit(4);
	delay(1000);
	displayDigit(2);
	delay(1000);
}
