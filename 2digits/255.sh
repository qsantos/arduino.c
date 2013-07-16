#!/bin/sh
(echo ":01000000FF00"; echo ":00000001FF") | avrdude -D -p atmega32u4 -c avr109 -P /dev/ttyACM0 -U eeprom:w:-:i
