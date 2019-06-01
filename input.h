#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

enum Button
{
	BUTTON_A     = 1,
	BUTTON_B     = 0,
	BUTTON_UP    = 5,
	BUTTON_DOWN  = 3,
	BUTTON_LEFT  = 4,
	BUTTON_RIGHT = 2,

	BUTTON_COUNT = 6
};

enum EventType
{
	EVENT_DOWN,
	EVENT_UP,
};

struct InputEvent
{
	Button m_button;
	EventType m_event;
};

void input_init();
void input_update();
void input_set_repeat_ms(uint16_t ms);
bool input_get_event(InputEvent &result);

#endif // INPUT_H
