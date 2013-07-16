#include <Arduino.h>
#include <avr/eeprom.h>

#define FREQ 60
int  char2pin[] = {9, 8, 7, 10, 11, 12, 13, 6};
byte digits  [] = {0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6};

void displayDigit(int d)
{
	byte code = digits[d%10];
	for (int c = 7; c >= 0; c--)
	{
		digitalWrite(char2pin[c], code%2 ? HIGH : LOW);
		code >>= 1;
	}
}

void clearDigit()
{
	for (int c = 7; c >= 0; c--)
		digitalWrite(char2pin[c], LOW);
}

void setup()
{
	for (int i = 5; i <= 13; i++)
		pinMode(i, OUTPUT);
	pinMode(3, OUTPUT);
}

void loop()
{
	unsigned char d = eeprom_read_byte(0);
	//static int v = 0;
	//analogWrite(3, v++);
	for (int i = 0; i < FREQ; i++)
	{
		digitalWrite(5, LOW);
		displayDigit(d/10);
		delay(500/FREQ);

		digitalWrite(5, HIGH);
		displayDigit(d%10);
		delay(500/FREQ);
	}
	eeprom_write_byte(0, d+1);
}
