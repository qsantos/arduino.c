#include <Arduino.h>

char DS = 0;
char CP = 1;

void setup()
{
	pinMode(DS, OUTPUT);
	pinMode(CP, OUTPUT);
}

void pushBit(char bit)
{
	digitalWrite(DS, bit ? HIGH : LOW);
	digitalWrite(CP, HIGH);
	digitalWrite(CP, LOW);
}

void pushBits(int word, int len)
{
	for (; len; len--, word >>= 1)
		pushBit(word%2);
}

void loop()
{
	for (int i = 0; ; i=(i+1)%255)
	{
		pushBits(i, 8);
		delay(500);
	}
}
