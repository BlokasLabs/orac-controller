#include <usbmidi.h>
#include "input.h"
#include "SH1106.h"

#include "font.h"

void setup()
{
	sh1106_init(SS, PIN_LCD_DC, PIN_LCD_RESET);

	input_init();

	print(0, "Waiting for OscDisplayBridge...", 31, 128, false);
}

void print(int8_t line, const char *text, uint8_t n, uint8_t maxWidth, bool inverted)
{
	sh1106_set_position(0, 7-(line&7));
	int16_t space = maxWidth - n*(FONT_WIDTH+1);
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

void clearScreen()
{
	sh1106_clear();
}

void printCtrl(uint8_t id, uint8_t v)
{
	const uint8_t line = 0x7e;
	const uint8_t narrowerLine = 0x3c;
	const uint8_t dot = 0x10;
	v = 20.0f / 127.0f * v;

	sh1106_set_position(127-20-1, 7-(id&7));
	sh1106_draw_bitmap(&line, 1, false);

	uint8_t i;
	for (i=0; i<v; ++i)
	{
		if (i%3 == 0) sh1106_draw_bitmap(&dot, 1, false);
		else sh1106_draw_space(1, false);
	}
	sh1106_draw_bitmap(&narrowerLine, 1, false);
	++i;
	for (; i<20; ++i)
	{
		if (i%3 == 0) sh1106_draw_bitmap(&dot, 1, false);
		else sh1106_draw_space(1, false);
	}
	sh1106_draw_bitmap(&line, 1, false);
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
			else if (b == 0x02)
			{
				m_state = STATE_RECEIVING_CTRL;
				m_textLine = -1;
				m_counter = 0;
			}
			else if (b == 0x03)
			{
				m_state = STATE_RECEIVING_CLEAR_SCREEN;
			}
			else if (b == 0x04)
			{
				m_state = STATE_RECEIVING_MAX_LINE_WIDTH;
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
		case STATE_RECEIVING_CTRL:
			if (m_textLine == -1)
			{
				m_textLine = b;
			}
			else
			{
				m_counter = b;
			}
			return;
		case STATE_RECEIVING_MAX_LINE_WIDTH:
			m_maxWidth = b == 0 ? 128 : b;
			return;
		}
	}

private:
	void executeCommand()
	{
		switch (m_state)
		{
		case STATE_RECEIVING_TEXT:
		case STATE_RECEIVING_INVERTED_TEXT:
			print(m_textLine, g_messageBuffer, m_counter, m_maxWidth, m_state == STATE_RECEIVING_INVERTED_TEXT);
			break;
		case STATE_RECEIVING_CLEAR_SCREEN:
			clearScreen();
			break;
		case STATE_RECEIVING_CTRL:
			printCtrl(m_textLine, m_counter);
			break;
		}
	}

	enum State
	{
		STATE_WAITING_FOR_COMMAND,
		STATE_SYSEX_START,
		STATE_RECEIVING_TEXT,
		STATE_RECEIVING_INVERTED_TEXT,
		STATE_RECEIVING_CTRL,
		STATE_RECEIVING_CLEAR_SCREEN,
		STATE_RECEIVING_MAX_LINE_WIDTH,
		STATE_SYSEX_END,
	};

	State m_state;
	int8_t m_textLine;
	uint8_t m_counter;
	uint8_t m_maxWidth;
};

CommandHandler g_handler;

void loop()
{
	while (USBMIDI.available())
	{
		g_handler.process(USBMIDI.read());
	}

	input_update();

	InputEvent event;
	while (input_get_event(event))
	{
		bool on = event.m_event == EVENT_DOWN;
		USBMIDI.write(on ? 0x90 : 0x80);
		USBMIDI.write(event.m_button);
		USBMIDI.write(on ? 100 : 0);
	}

	USBMIDI.poll();
}
