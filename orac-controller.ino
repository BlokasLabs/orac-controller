#include <usbmidi.h>
#include "input.h"
#include "SH1106.h"
#include "font.h"

#define FONT_WIDTH FONT_5X7_WIDTH
#define FONT FONT_5X7

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
		const uint8_t *p = &FONT[(*text++ - ' ') * FONT_WIDTH];
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

void printCtrl(uint8_t id, uint8_t v, bool inverted)
{
	const uint8_t line = 0x7e;
	const uint8_t narrowerLine = 0x3c;
	const uint8_t dot = 0x10;
	v = 19.8f / 127.0f * v;

	sh1106_set_position(127-20-2, 7-(id&7));
	sh1106_draw_bitmap(&line, 1, inverted);

	uint8_t i;
	for (i=0; i<v; ++i)
	{
		if (i%3 == 0) sh1106_draw_bitmap(&dot, 1, inverted);
		else sh1106_draw_space(1, inverted);
	}
	sh1106_draw_bitmap(&narrowerLine, 1, inverted);
	++i;
	for (; i<20; ++i)
	{
		if (i%3 == 0) sh1106_draw_bitmap(&dot, 1, inverted);
		else sh1106_draw_space(1, inverted);
	}
	sh1106_draw_bitmap(&line, 1, inverted);
}

char g_messageBuffer[128 / 4];

class CommandHandler
{
public:
	CommandHandler()
		:m_state(STATE_WAITING_FOR_COMMAND)
		,m_textLine(-1)
		,m_inverted(false)
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
			switch (b&0x3f)
			{
			case 0x00:
				m_state = STATE_RECEIVING_TEXT;
				m_textLine = -1;
				m_counter = 0;
				m_inverted = b >= 0x40;
				break;
			case 0x01:
				m_state = STATE_RECEIVING_CTRL;
				m_textLine = -1;
				m_counter = 0;
				m_inverted = b >= 0x40;
				break;
			case 0x02:
				m_state = STATE_RECEIVING_CLEAR_SCREEN;
				break;
			case 0x03:
				m_state = STATE_RECEIVING_VIEW_MODE;
				break;
			default:
				m_state = STATE_WAITING_FOR_COMMAND;
				break;
			}
			return;
		case STATE_RECEIVING_TEXT:
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
		case STATE_RECEIVING_VIEW_MODE:
			m_counter = b;
			return;
		}
	}

private:
	void executeCommand()
	{
		switch (m_state)
		{
		case STATE_RECEIVING_TEXT:
			print(m_textLine, g_messageBuffer, m_counter, m_maxWidth, m_inverted);
			break;
		case STATE_RECEIVING_CLEAR_SCREEN:
			clearScreen();
			break;
		case STATE_RECEIVING_CTRL:
			printCtrl(m_textLine, m_counter, m_inverted);
			break;
		case STATE_RECEIVING_VIEW_MODE:
			m_mode = (ViewMode)m_counter;
			switch (m_mode)
			{
			case MODE_MENU:
				m_maxWidth = 128;
				input_set_repeat_ms(300);
				break;
			case MODE_PARAMS:
				m_maxWidth = 105;
				input_set_repeat_ms(100);
				break;
			}
			break;
		}
	}

	enum State
	{
		STATE_WAITING_FOR_COMMAND,
		STATE_SYSEX_START,
		STATE_RECEIVING_TEXT,
		STATE_RECEIVING_CTRL,
		STATE_RECEIVING_CLEAR_SCREEN,
		STATE_RECEIVING_VIEW_MODE,
		STATE_SYSEX_END,
	};

	enum ViewMode
	{
		MODE_UNKNOWN = 0,
		MODE_MENU    = 1,
		MODE_PARAMS  = 2,
	};

	State m_state;
	ViewMode m_mode;
	int8_t m_textLine;
	bool m_inverted;
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
