#include <Arduino.h>

#include "USB.h"

void setup();
void loop();

int main()
{
	init();

#if defined(USBCON)
	USB_attach();
#endif

	setup();
	while (1)
		loop();
	return 0;
}
