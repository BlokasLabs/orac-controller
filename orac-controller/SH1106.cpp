/*
 * Midimon.
 * Copyright (C) 2018-2019  Vilniaus Blokas UAB, https://blokas.io/midiboy
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "SH1106.h"

#include <avr/pgmspace.h>

#include <SPI.h>
#include <stdint.h>

static uint8_t g_ss;
static uint8_t g_cd;
static uint8_t g_reset;
static uint8_t g_scroll;

static const SPISettings g_spiSettings(10000000, MSBFIRST, SPI_MODE0);

class Transaction
{
public:
	inline Transaction()
	{
		digitalWrite(g_ss, LOW);
		SPI.beginTransaction(g_spiSettings);
	}

	inline ~Transaction()
	{
		digitalWrite(g_ss, HIGH);
		SPI.endTransaction();
	}
};

void sh1106_init(int ss, int cd, int reset)
{
	g_ss = ss;
	g_cd = cd;
	g_reset = reset;
	g_scroll = 0;

	pinMode(g_ss, OUTPUT);
	pinMode(g_cd, OUTPUT);
	pinMode(g_reset, OUTPUT);

	SPI.begin();

	sh1106_reset();
}

enum Mode
{
	COMMAND,
	DATA
};

inline static void sh1106_mode(Mode mode)
{
	digitalWrite(g_cd, mode == DATA);
}

inline static void sh1106_send(uint8_t byte)
{
	SPI.transfer(byte);
}

inline static void sh1106_set_scroll_line(uint8_t line)
{
	sh1106_send(0x40 | (line & 0x3f));
}

inline static void sh1106_set_column_address(uint8_t address)
{
	sh1106_send(address & 0x0f);
	sh1106_send(0x10 | (address >> 4));
}

inline static void sh1106_set_page_address(uint8_t address)
{
	sh1106_send(0xb0  | (address & 0x07));
}

inline static void sh1106_set_contrast_reg(uint8_t volume) // A.K.A. PM 
{
	sh1106_send(0x81);
	sh1106_send(volume);
}

inline static void sh1106_set_all_pixels_on(bool on)
{
	sh1106_send(0xa4 | (on ? 1 : 0));
}

inline static void sh1106_set_inverse_display(bool on)
{
	sh1106_send(0xa6 | (on ? 1 : 0));
}

inline static void sh1106_set_display_enable(bool on)
{
	sh1106_send(0xae | (on ? 1 : 0));
}

inline static void sh1106_set_mirror_x(bool on)
{
	sh1106_send(0xa0 | (on ? 1 : 0));
}

inline static void sh1106_set_mirror_y(bool on)
{
	sh1106_send(0xc0 | (on ? 0x08 : 0));
}

inline static void sh1106_nop()
{
	sh1106_send(0xe3);
}

void sh1106_reset()
{
	digitalWrite(g_reset, LOW);
	delay(1);
	digitalWrite(g_reset, HIGH);
	delay(5);

	g_scroll = 0;

	{
		Transaction t;

		sh1106_mode(COMMAND);
		sh1106_set_display_enable(false);
		sh1106_set_scroll_line(0);
		sh1106_set_mirror_x(true);
		sh1106_set_mirror_y(false);
		sh1106_set_contrast_reg(0xff);
	}

	sh1106_clear();

	{
		Transaction t;
		sh1106_mode(COMMAND);
		sh1106_set_display_enable(true);
	}
}

void sh1106_clear()
{
	sh1106_mode(DATA);
	Transaction t;

	for (int j=0; j<8; ++j)
	{
		sh1106_mode(COMMAND);
		sh1106_set_column_address(2);
		sh1106_set_page_address(j);

		sh1106_mode(DATA);
		for (int i=0; i<128; ++i)
		{
			sh1106_send(0);
		}
	}
}

void sh1106_set_scroll(uint8_t line)
{
	g_scroll = line & 0x3f;
	sh1106_mode(COMMAND);
	Transaction t;
	sh1106_set_scroll_line(g_scroll);
}

void sh1106_add_scroll(int8_t delta)
{
	sh1106_set_scroll(g_scroll + delta);
}

void sh1106_set_position(uint8_t x, uint8_t y)
{
	sh1106_mode(COMMAND);
	Transaction t;
	sh1106_set_column_address(x+2);
	sh1106_set_page_address((g_scroll >> 3) + y);
}

void sh1106_draw_space(uint8_t n, bool inverse)
{
	sh1106_mode(DATA);
	Transaction t;
	uint8_t byte = !inverse ? 0x00 : 0xff;
	while (n-- != 0)
		sh1106_send(byte);
}

void sh1106_draw_bitmap(const void *data, uint8_t n, bool inverse)
{
	sh1106_mode(DATA);
	Transaction t;
	const uint8_t *p = (const uint8_t*)data;
	if (!inverse)
	{
		while (n-- != 0)
			sh1106_send(*p++);
	}
	else
	{
		while (n-- != 0)
			sh1106_send(~(*p++));
	}
}

void sh1106_draw_progmem_bitmap(const void *data, uint8_t n, bool inverse)
{
	sh1106_mode(DATA);
	Transaction t;
	const uint8_t *p = (const uint8_t *)data;
	if (!inverse)
	{
		while (n-- != 0)
		{
			uint8_t byte = pgm_read_byte(p++);
			sh1106_send(byte);
		}
	}
	else
	{
		while (n-- != 0)
		{
			uint8_t byte = pgm_read_byte(p++);
			sh1106_send(~byte);
		}
	}
}

void sh1106_set_contrast(uint8_t contrast)
{
	sh1106_mode(COMMAND);
	Transaction t;
	sh1106_set_contrast_reg(contrast);
}
