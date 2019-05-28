#include <usbmidi.h>
#include "SH1106.h"

void setup()
{
	sh1106_init(SS, PIN_LCD_DC, PIN_LCD_RESET);
}

void loop()
{
	while (USBMIDI.available())
	{
		if ((USBMIDI.read() & 0xf0) == 0x90)
		{
			static bool x = false;
			sh1106_set_position(10, 2);
			sh1106_draw_space(4, x);
			x = !x;
		}
	}

	USBMIDI.poll();
}
