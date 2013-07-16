/*\
 *  Library for pure-C programming for Arduino
 *  Copyright (C) 2012  Quentin SANTOS
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
\*/

#ifndef USB_H
#define USB_H

#undef CDC_ENABLED
#undef HID_ENABLED

typedef char           bool;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#include <Arduino.h>
#include <USBCore.h>
#include <USBDesc.h>

// from USBAPI.h
#define TRANSFER_PGM     0x80
#define TRANSFER_RELEASE 0x40
#define TRANSFER_ZERO    0x20

// setup token
typedef struct
{
	u8  bmRequestType;
	u8  bRequest;
	u8  wValueL;
	u8  wValueH;
	u16 wIndex;
	u16 wLength;
} Setup;

int  USB_SendControl(u8 flags, const void* d, int len);
int  SendInterfaces ();
u8   USB_Available  (u8 ep);
void USB_Flush      (u8 ep);

u8  USBGetConfiguration(void);
int USB_Recv           (u8 ep, void* d, int len);
u8  USB_SendSpace      (u8 ep);
int USB_Send           (u8 ep, const void* d, int len);
int USB_RecvControl    (void* d, int len);
u8  USBConnected       ();

void USB_attach();

#ifdef HID_ENABLED
int  HID_GetInterface (u8* interfaceNum);
int  HID_GetDescriptor(int i);
void HID_SendReport   (u8 id, const void* data, int len);
bool HID_Setup        (Setup* setup);
#endif

#ifdef CDC_ENABLED
int    CDC_GetInterface(u8* interfaceNum);
bool   CDC_Setup       (Setup* setup);

void   Serial_accept   (void);
int    Serial_available(void);
int    Serial_peek     (void);
int    Serial_read     (void);
void   Serial_flush    (void);
size_t Serial_write    (uint8_t c);
#endif

#endif
