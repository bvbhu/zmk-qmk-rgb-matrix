/*
 * Copyright (c) 2026 bvbhu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <stdint.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

#ifdef CONFIG_ZMK_HID_INDICATORS
#	include <zmk/hid_indicators.h>
typedef struct
{
	uint8_t num_lock : 1;
	uint8_t caps_lock : 1;
	uint8_t scroll_lock : 1;
} led_t;
led_t host_keyboard_led_state(void);
#endif

#define MOD_MASK_CTRL (MOD_LCTL | MOD_RCTL)
#define MOD_MASK_SHIFT (MOD_LSFT | MOD_RSFT)
#define MOD_MASK_ALT (MOD_LALT | MOD_RALT)
#define MOD_MASK_GUI (MOD_LGUI | MOD_RGUI)

#define layer_state NULL
#define get_highest_layer(...) ((uint8_t)zmk_keymap_highest_layer_active())

uint8_t get_mods(void);

bool is_keyboard_master(void);
bool is_keyboard_left(void);