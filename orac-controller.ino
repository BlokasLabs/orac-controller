#include <usbmidi.h>
#include "SH1106.h"

#include "font.h"

uint8_t g_buttonState;

void setup()
{
	sh1106_init(SS, PIN_LCD_DC, PIN_LCD_RESET);
	// Enable pull-ups on button pins.
	PORTC = 0x3f;
	g_buttonState = PINC & 0x3f;

	print(0, "Waiting for OscDisplayBridge...", 31, false);
}

void print(int8_t line, const char *text, uint8_t n, bool inverted)
{
	sh1106_set_position(0, 7-(line&7));
	int16_t space = 128 - n*(FONT_WIDTH+1);
	while (n--)
	{
		const uint8_t *p = &MICRO_FONT[(*text++ - ' ') * FONT_WIDTH];
		sh1106_draw_progmem_bitmap(p, FONT_WIDTH, inverted);
		sh1106_draw_space(1, inverted);
	}
	if (space > 0)
	{
		sh1106_draw_space(space, inverted);
	}
}

char g_messageBuffer[128 / 4];

class CommandHandler
{
public:
	CommandHandler()
		:m_state(STATE_WAITING_FOR_COMMAND)
		,m_textLine(-1)
	{
	}

	void process(uint8_t b)
	{
		if (b == 0xf7)
		{
			executeCommand();
			m_state = STATE_WAITING_FOR_COMMAND;
			return;
		}

		switch (m_state)
		{
		case STATE_WAITING_FOR_COMMAND:
			if (b == 0xf0)
			{
				m_state = STATE_SYSEX_START;
			}
			return;
		case STATE_SYSEX_START:
			if (b == 0x00)
			{
				m_state = STATE_RECEIVING_TEXT;
				m_textLine = -1;
				m_counter = 0;
			}
			else if (b == 0x01)
			{
				m_state = STATE_RECEIVING_INVERTED_TEXT;
				m_textLine = -1;
				m_counter = 0;
			}
			else m_state = STATE_WAITING_FOR_COMMAND;
			return;
		case STATE_RECEIVING_TEXT:
		case STATE_RECEIVING_INVERTED_TEXT:
			if (m_textLine == -1)
			{
				m_textLine = b;
			}
			else
			{
				if (m_counter < sizeof(g_messageBuffer))
					g_messageBuffer[m_counter++] = b;
			}
			return;
		}
	}

private:
	void executeCommand()
	{
		if (m_state == STATE_RECEIVING_TEXT || m_state == STATE_RECEIVING_INVERTED_TEXT)
		{
			print(m_textLine, g_messageBuffer, m_counter, m_state == STATE_RECEIVING_INVERTED_TEXT);
		}
	}

	enum State
	{
		STATE_WAITING_FOR_COMMAND,
		STATE_SYSEX_START,
		STATE_RECEIVING_TEXT,
		STATE_RECEIVING_INVERTED_TEXT,
		STATE_SYSEX_END,
	};

	State m_state;
	int8_t m_textLine;
	uint8_t m_counter;
};

CommandHandler g_handler;

void loop()
{
	while (USBMIDI.available())
	{
		g_handler.process(USBMIDI.read());
	}

	uint8_t newButtonState = PINC & 0x3f;
	uint8_t change = newButtonState ^ g_buttonState;

	if (change)
	{
		for (int8_t i=0; i<6; ++i)
		{
			int8_t bit = 1 << i;
			if (change & bit)
			{
				bool on = !(newButtonState & bit);

				USBMIDI.write(on ? 0x90 : 0x80);
				USBMIDI.write(i);
				USBMIDI.write(on ? 100 : 0);
			}
		}
		g_buttonState = newButtonState;
	}

	USBMIDI.poll();
}
