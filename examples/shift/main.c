#include <Arduino.h>

//         g  f  e  d DP  c  b  a
int map[]={1, 2, 3, 4, 0, 5, 6, 7};
byte digits[]={0xFC,0x60,0xDA,0xF2,0x66,0xB6,0xBE,0xE0,0xFE,0xF6,0x08};
void displayDigit(int d)
{
	byte code = digits[d%11];
	for (int c = 7; c >= 0; c--)
	{   
		int bit = (code >> map[c]) & 0x1;
		digitalWrite(12, LOW);
		digitalWrite(13, bit ? HIGH : LOW);
		digitalWrite(12, HIGH);
	}   
}
void setup()
{
	pinMode(13, OUTPUT);
	pinMode(12, OUTPUT);
}
void loop()
{
	displayDigit(4);
	delay(1000);
	displayDigit(2);
	delay(1000);
}
