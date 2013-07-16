/* Copyright (c) 2010, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "c_USB.h"

#if defined(USBCON)

#define EP_TYPE_CONTROL         0x00
#define EP_TYPE_BULK_IN         0x81
#define EP_TYPE_BULK_OUT        0x80
#define EP_TYPE_INTERRUPT_IN    0xC1
#define EP_TYPE_INTERRUPT_OUT   0xC0
#define EP_TYPE_ISOCHRONOUS_IN  0x41
#define EP_TYPE_ISOCHRONOUS_OUT 0x40

//==================================================================
//                     DEVICE DESCRIPTION
//==================================================================

static const u16 STRING_LANGUAGE[2] PROGMEM =
{
	(3<<8) | (2+2),
	0x0409	// English
};

const u16 STRING_IPRODUCT[17] PROGMEM =
{
	(3<<8) | (2+2*16),
#if USB_PID == 0x8036
	'A','r','d','u','i','n','o',' ','L','e','o','n','a','r','d','o'
#else
	'U','S','B',' ','I','O',' ','B','o','a','r','d',' ',' ',' ',' '
#endif
};

const u16 STRING_IMANUFACTURER[12] PROGMEM =
{
	(3<<8) | (2+2*11),
#if USB_VID == 0x2341
	'A','r','d','u','i','n','o',' ','L','L','C'
#else
	'U','n','k','n','o','w','n',' ',' ',' ',' '
#endif
};

#ifdef CDC_ENABLED
#define DEVICE_CLASS 0x02
#else
#define DEVICE_CLASS 0x00
#endif

// DEVICE DESCRIPTOR
static const DeviceDescriptor USB_DeviceDescriptor PROGMEM =
	D_DEVICE(0x00,        0x00,0x00,64,USB_VID,USB_PID,0x100,IMANUFACTURER,IPRODUCT,0,1);

// used for CDC
static const DeviceDescriptor USB_DeviceDescriptorA PROGMEM =
	D_DEVICE(DEVICE_CLASS,0x00,0x00,64,USB_VID,USB_PID,0x100,IMANUFACTURER,IPRODUCT,0,1);

//==================================================================
//                    BASIC USB UTILITIES
//==================================================================

// TX and TX LED pulse handling
#define TX_RX_LED_PULSE_MS 100
volatile u8 TxLEDPulse; // milliseconds remaining for TX LED pulse
volatile u8 RxLEDPulse; // milliseconds remaining for RX LED pulse

volatile u8 _curConf = 0;

static inline void WaitIN(void)
{
	while (!(UEINTX & (1<<TXINI)));
}

static inline void ClearIN(void)
{
	UEINTX = ~(1<<TXINI);
}

static inline void WaitOUT(void)
{
	while (!(UEINTX & (1<<RXOUTI)));
}

static inline u8 WaitForINOrOUT()
{
	while (!(UEINTX & ((1<<TXINI)|(1<<RXOUTI))));
	return (UEINTX & (1<<RXOUTI)) == 0;
}

static inline void ClearOUT(void)
{
	UEINTX = ~(1<<RXOUTI);
}

static inline void Recv(volatile u8* data, u8 count)
{
	while (count--)
		*data++ = UEDATX;

	// make the RX LED pulse
	RXLED1;
	RxLEDPulse = TX_RX_LED_PULSE_MS;
}

static inline u8 Recv8()
{
	// make the RX LED pulse
	RXLED1;
	RxLEDPulse = TX_RX_LED_PULSE_MS;

	return UEDATX;
}

static inline void Send8(u8 d)
{
	UEDATX = d;
}

static inline void SetEP(u8 ep)
{
	UENUM = ep;
}

static inline u8 FifoByteCount()
{
	return UEBCLX;
}

static inline u8 ReceivedSetupInt()
{
	return UEINTX & (1<<RXSTPI);
}

static inline void ClearSetupInt()
{
	UEINTX = ~((1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI));
}

static inline void Stall()
{
	UECONX = (1<<STALLRQ) | (1<<EPEN);
}

static inline u8 ReadWriteAllowed()
{
	return UEINTX & (1<<RWAL);
}

static inline u8 Stalled()
{
	return UEINTX & (1<<STALLEDI);
}

static inline u8 FifoFree()
{
	return UEINTX & (1<<FIFOCON);
}

static inline void ReleaseRX()
{
	UEINTX = 0x6B; // FIFOCON=0 NAKINI=1 RWAL=1 NAKOUTI=0 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=1
}

static inline void ReleaseTX()
{
	UEINTX = 0x3A; // FIFOCON=0 NAKINI=0 RWAL=1 NAKOUTI=1 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=0
}

static inline u8 FrameNumber()
{
	return UDFNUML;
}


//==================================================================
//                COMMUNICATION INTERRUPT HANDLING
//==================================================================

#define EP_SINGLE_64 0x32 // EP0
#define EP_DOUBLE_64 0x36 // Other endpoints

static inline void InitEP(u8 index, u8 type, u8 size)
{
	UENUM = index;
	UECONX = 1;
	UECFG0X = type;
	UECFG1X = size;
}

static const u8 _initEndpoints[] PROGMEM =
{
	0,

#ifdef CDC_ENABLED
	EP_TYPE_INTERRUPT_IN, // CDC_ENDPOINT_ACM
	EP_TYPE_BULK_OUT,     // CDC_ENDPOINT_OUT
	EP_TYPE_BULK_IN,      // CDC_ENDPOINT_IN
#endif

#ifdef HID_ENABLED
	EP_TYPE_INTERRUPT_IN,  // HID_ENDPOINT_INT
#endif
};
static inline void InitEndpoints()
{
	for (u8 i = 1; i < sizeof(_initEndpoints); i++)
	{
		UENUM = i;
		UECONX = 1;
		UECFG0X = pgm_read_byte(_initEndpoints+i);
		UECFG1X = EP_DOUBLE_64;
	}
	UERST = 0x7E;	// And reset them
	UERST = 0;
}

int _cmark;
int _cend;
static inline void InitControl(int end)
{
	SetEP(0);
	_cmark = 0;
	_cend = end;
}

static inline bool SendControl(u8 d)
{
	if (_cmark < _cend)
	{
		if (!WaitForINOrOUT())
			return false;
		Send8(d);
		if (!((_cmark + 1) & 0x3F))
			ClearIN();	// Fifo is full, release this packet
	}
	_cmark++;
	return true;
}

// Clipped by _cmark/_cend
int USB_SendControl(u8 flags, const void* d, int len)
{
	int sent = len;
	const u8* data = (const u8*)d;
	bool pgm = flags & TRANSFER_PGM;
	while (len--)
	{
		u8 c = pgm ? pgm_read_byte(data++) : *data++;
		if (!SendControl(c))
			return -1;
	}
	return sent;
}

int SendInterfaces()
{
	int total = 0;
	u8 interfaces = 0;

#ifdef CDC_ENABLED
	total = CDC_GetInterface(&interfaces);
#endif

#ifdef HID_ENABLED
	total += HID_GetInterface(&interfaces);
#endif

	return interfaces;
}

// Construct a dynamic configuration descriptor
// This really needs dynamic endpoint allocation etc
// TODO
static inline bool SendConfiguration(int maxlen)
{
	// Count and measure interfaces
	InitControl(0);
	int interfaces = SendInterfaces();
	ConfigDescriptor config = D_CONFIG(_cmark + sizeof(ConfigDescriptor),interfaces);

	// Now send them
	InitControl(maxlen);
	USB_SendControl(0,&config,sizeof(ConfigDescriptor));
	SendInterfaces();
	return true;
}

u8 _cdcComposite = 0;

static bool SendDescriptor(Setup* setup)
{
	u8 t = setup->wValueH;
	if (USB_CONFIGURATION_DESCRIPTOR_TYPE == t)
		return SendConfiguration(setup->wLength);

	InitControl(setup->wLength);
#ifdef HID_ENABLED
	if (HID_REPORT_DESCRIPTOR_TYPE == t)
		return HID_GetDescriptor(t);
#endif

	u8 desc_length = 0;
	const u8* desc_addr = 0;
	if (USB_DEVICE_DESCRIPTOR_TYPE == t)
	{
		if (setup->wLength == 8)
			_cdcComposite = 1;
		desc_addr = (uint8_t*) (_cdcComposite ? &USB_DeviceDescriptorA : &USB_DeviceDescriptor);
	}
	else if (USB_STRING_DESCRIPTOR_TYPE == t)
	{
		if (setup->wValueL == 0)
			desc_addr = (const u8*)&STRING_LANGUAGE;
		else if (setup->wValueL == IPRODUCT)
			desc_addr = (const u8*)&STRING_IPRODUCT;
		else if (setup->wValueL == IMANUFACTURER)
			desc_addr = (const u8*)&STRING_IMANUFACTURER;
		else
			return false;
	}

	if (desc_addr == 0)
		return false;
	if (desc_length == 0)
		desc_length = pgm_read_byte(desc_addr);

	USB_SendControl(TRANSFER_PGM,desc_addr,desc_length);
	return true;
}

// communication interrupt
ISR(USB_COM_vect)
{
	SetEP(0); // assume EndPoint 0 is used

	// wait and receive setup token
	if (!ReceivedSetupInt()) // did not get the receive token (interrupt flag)
		return;
	Setup setup;
	Recv((uint8_t*) &setup, sizeof(setup));
	ClearSetupInt();

	u8 requestType = setup.bmRequestType;
	switch (requestType & REQUEST_DIRECTION)
	{
	case REQUEST_DEVICETOHOST:
		WaitIN(); // waits until ready to send data
		break;
	case REQUEST_HOSTTODEVICE:
		ClearIN(); // ready to receive data
		break;
	}

	bool ok = true;
	switch (requestType & REQUEST_TYPE)
	{
	case REQUEST_STANDARD:
		switch (setup.bRequest)
		{
		case GET_STATUS:
			Send8(0);
			Send8(0);
			break;
		case SET_ADDRESS:
			WaitIN();
			UDADDR = setup.wValueL | (1<<ADDEN);
			break;
		case GET_DESCRIPTOR:
			ok = SendDescriptor(&setup);
			break;
		case SET_DESCRIPTOR:
			ok = false;
			break;
		case GET_CONFIGURATION:
			Send8(1);
			break;
		case SET_CONFIGURATION:
			switch (requestType & REQUEST_RECIPIENT)
			{
			case REQUEST_DEVICE:
				InitEndpoints();
				_curConf = setup.wValueL;
				break;
			default:
				ok = false; // should not occur
			}
			break;
		}
		break;
	case REQUEST_CLASS:
		InitControl(setup.wLength); // Max length of transfer
		switch (setup.wIndex)
		{
#ifdef CDC_ENABLED
		case CDC_ACM_INTERFACE:
			ok = CDC_Setup(&setup);
			break;
#endif
#ifdef HID_ENABLED
		case HID_INTERFACE:
			ok = HID_Setup(&setup);
			break;
#endif
		default:
			break;
		}
		break;
	default:
		ok = false; // should not occur
	}

	if (ok)
		ClearIN();
	else
		Stall();
}


//==================================================================
//                   GENERAL INTERRUPT HANDLING
//==================================================================

#define LOCKEP   u8 _sreg = SREG; cli(); SetEP(ep & 7);
#define UNLOCKEP SREG = _sreg;

// Number of bytes, assumes a rx endpoint
u8 USB_Available(u8 ep)
{
	LOCKEP;
	u8 r = FifoByteCount();
	UNLOCKEP;
	return r;
}

void USB_Flush(u8 ep)
{
	SetEP(ep);
	if (FifoByteCount())
		ReleaseTX();
}

// General interrupt
ISR(USB_GEN_vect)
{
	u8 udint = UDINT;
	UDINT = 0;

	if (udint & (1<<EORSTI)) // End of Reset
	{
		InitEP(0, EP_TYPE_CONTROL, EP_SINGLE_64); // init EP0
		_curConf = 0;                             // not configured yet
		UEIENX = 1 << RXSTPE;                     // Enable interrupts for ep0
	}

	// Start of Frame
	if (udint & (1<<SOFI))
	{
#ifdef CDC_ENABLED
		USB_Flush(CDC_TX);            // Send a tx frame if found
		while (USB_Available(CDC_RX)) // Handle received bytes (if any)
			Serial_accept();
#endif

		// happens every millisecond so we use it for TX and RX LED one-shot timing, too
		if (TxLEDPulse && !(--TxLEDPulse)) TXLED0;
		if (RxLEDPulse && !(--RxLEDPulse)) RXLED0;
	}
}


//==================================================================
//                         NON-CORE STUFF
//==================================================================

u8 USBGetConfiguration(void)
{
	return _curConf;
}

// Non Blocking receive
// Return number of bytes read
int USB_Recv(u8 ep, void* d, int len)
{
	if (!_curConf || len < 0)
		return -1;

	LOCKEP;
	u8 n = FifoByteCount();
	len = min(n,len);
	n = len;
	u8* dst = (u8*)d;
	while (n--)
		*dst++ = Recv8();
	if (len && !FifoByteCount())	// release empty buffer
		ReleaseRX();
	UNLOCKEP;

	return len;
}

// Space in send EP
u8 USB_SendSpace(u8 ep)
{
	LOCKEP;
	u8 r = ReadWriteAllowed() ? 64 - FifoByteCount() : 0;
	UNLOCKEP;
	return r;
}

// Blocking Send of data to an endpoint
int USB_Send(u8 ep, const void* d, int len)
{
	if (!_curConf)
		return -1;

	int r = len;
	const u8* data = (const u8*)d;
	u8 timeout = 250;		// 250ms timeout on send? TODO
	while (len)
	{
		u8 n = USB_SendSpace(ep);
		if (n == 0)
		{
			if (!(--timeout))
				return -1;
			delay(1);
			continue;
		}

		if (n > len)
			n = len;
		len -= n;
		{
			LOCKEP;
			if (ep & TRANSFER_ZERO)
			{
				while (n--)
					Send8(0);
			}
			else if (ep & TRANSFER_PGM)
			{
				while (n--)
					Send8(pgm_read_byte(data++));
			}
			else
			{
				while (n--)
					Send8(*data++);
			}
			if (!ReadWriteAllowed() || ((len == 0) && (ep & TRANSFER_RELEASE)))	// Release full buffer
				ReleaseTX();
			UNLOCKEP;
		}
	}
	TXLED1;					// light the TX LED
	TxLEDPulse = TX_RX_LED_PULSE_MS;
	return r;
}

// Does not timeout or cross fifo boundaries
// Will only work for transfers <= 64 bytes
// TODO
int USB_RecvControl(void* d, int len)
{
	WaitOUT();
	Recv((u8*)d,len);
	ClearOUT();
	return len;
}

// VBUS or counting frames
// Any frame counting?
u8 USBConnected()
{
	u8 f = UDFNUML;
	delay(3);
	return f != UDFNUML;
}

void USB_attach()
{
	_curConf = 0;
	UHWCON = 0x01;                  // power internal reg
	USBCON = (1<<USBE)|(1<<FRZCLK); // clock frozen, usb enabled
	PLLCSR = 0x12;                  // Need 16 MHz xtal
	while (!(PLLCSR & (1<<PLOCK))); // wait for lock pll

	delay(1); // quickfix for MacOsX 10.7.3 magic baud reset bug

	USBCON = ((1<<USBE)|(1<<OTGPADE)); // start USB clock
	UDIEN = (1<<EORSTE)|(1<<SOFE);     // Enable interrupts for EOR (End of Reset) and SOF (start of frame)
	UDCON = 0;                         // enable attach resistor

	TX_RX_LED_INIT;
}

#endif
