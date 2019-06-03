/*
 * orac-controller - Midiboy sketch for controlling Orac (https://github.com/TheTechnobear/Orac)
 * Copyright (C) 2019  Vilniaus Blokas UAB, https://blokas.io/
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

#ifndef FONT_H
#define FONT_H

#include <avr/pgmspace.h>

// 3x5 font.
enum { MICRO_FONT_WIDTH = 3 };
extern const PROGMEM unsigned char MICRO_FONT[288];

enum { FONT_5X7_WIDTH = 5 };
extern const PROGMEM unsigned char FONT_5X7[475];

#endif // FONT_H
