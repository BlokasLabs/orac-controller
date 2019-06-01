#include "input.h"
#include <fifo.h>

#include <pins_arduino.h>
#include <arduino.h>

enum { MAX_EVENTS = 8 };
enum { INPUT_DEBOUNCE_MS = 50 };
enum { INPUT_REPEAT_DELAY_MS = 300 };

static TFifo<InputEvent, uint8_t, MAX_EVENTS> g_eventQueue;
static uint8_t g_state;
static uint8_t g_repeating;
static unsigned long g_lastUpdated[BUTTON_COUNT];
static uint16_t g_inputRepeatMs = 300;

void input_set_repeat_ms(uint16_t ms)
{
	g_inputRepeatMs = ms;
}

static uint8_t readRawInput()
{
	// Read the state of the button pins directly and map them to the order of Button enum.
	return PINC & 0x3f;
}

static void updateState()
{
	// Read the state of the button pins directly and map them to the order of Button enum.
	uint8_t state = readRawInput();

	uint8_t diff = state ^ g_state;

	if (diff)
	{
		unsigned long ms = millis();
		for (uint8_t i=0; i<BUTTON_COUNT; ++i)
		{
			uint8_t bit = 1 << i;
			if ((diff & bit) && (ms - g_lastUpdated[i]) >= INPUT_DEBOUNCE_MS)
			{
				InputEvent e;
				e.m_button = (Button)i;
				e.m_event = (state & bit) ? EVENT_UP : EVENT_DOWN;
				g_eventQueue.push(e);
				g_lastUpdated[i] = ms;
				if (e.m_event == EVENT_UP)
				{
					g_state |= bit;
					g_repeating &= ~bit;
				}
				else
				{
					g_state &= ~bit;
				}
			}
		}
	}
}

void input_init()
{
	g_state = readRawInput();
	g_repeating = 0;
	const int pins[] = { PIN_BTN_A, PIN_BTN_B, PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_LEFT, PIN_BTN_RIGHT };

	for (int p : pins)
	{
		pinMode(p, INPUT_PULLUP);
	}
	for (int i=0; i<BUTTON_COUNT; ++i)
	{
		g_lastUpdated[i] = millis();
	}
}

void input_update()
{
	updateState();
	unsigned long ms = millis();
	for (int i=0; i<BUTTON_COUNT; ++i)
	{
		uint8_t bit = 1 << i;
		if ((g_state & bit) == 0)
		{
			if (!(g_repeating & bit))
			{
				if ((ms - g_lastUpdated[i]) >= INPUT_REPEAT_DELAY_MS)
				{
					g_repeating |= bit;
					g_lastUpdated[i] = ms;
				}
			}
			else
			{
				if ((ms - g_lastUpdated[i]) >= g_inputRepeatMs)
				{
					InputEvent e;
					e.m_button = (Button)i;
					e.m_event = EVENT_DOWN;
					g_eventQueue.push(e);
					g_lastUpdated[i] = ms;
				}
			}
		}
	}
}

bool input_get_event(InputEvent &result)
{
	return g_eventQueue.pop(result);
}
