#!/bin/sh
(echo ":010000002AD5"; echo ":00000001FF") | avrdude -D -p atmega32u4 -c avr109 -P /dev/ttyACM0 -U eeprom:w:-:i
