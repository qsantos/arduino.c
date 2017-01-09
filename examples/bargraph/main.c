#include <Arduino.h>

uint8_t DS = 0;
uint8_t CP = 1;

void setup()
{
	pinMode(DS, OUTPUT);
	pinMode(CP, OUTPUT);
}

void pushBit(int bit)
{
	digitalWrite(DS, bit ? HIGH : LOW);
	digitalWrite(CP, HIGH);
	digitalWrite(CP, LOW);
}

void pushBits(int bits, int len)
{
	for (int i = 0; i < len; i += 1)
	{
		pushBit((bits >> i) & 1);
	}
}

void loop()
{
	for (int i = 0; ; i=(i+1)%255)
	{
		pushBits(i, 8);
		delay(500);
	}
}
