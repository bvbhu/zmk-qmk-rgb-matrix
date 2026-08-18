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

#include "qmk_compat.h"

#include <zephyr/kernel.h>

bool is_keyboard_master(void)
{
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
	return IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL);
#else
	return true;
#endif
}

bool is_keyboard_left(void)
{
#if defined(RGB_MATRIX_IS_LEFT)
	return RGB_MATRIX_IS_LEFT;
#elif IS_ENABLED(CONFIG_ZMK_SPLIT)
	/* ZMK 惯例：central = 左半, peripheral = 右半 */
	return IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL);
#else
	return true;
#endif
}

#ifdef CONFIG_ZMK_HID_INDICATORS
led_t host_keyboard_led_state(void)
{
	uint8_t raw = (uint8_t)zmk_hid_indicators_get_current_profile();
	led_t state = {
		.num_lock = (raw >> 0) & 1,
		.caps_lock = (raw >> 1) & 1,
		.scroll_lock = (raw >> 2) & 1,
	};
	return state;
}
#endif /* CONFIG_ZMK_HID_INDICATORS */

uint8_t get_mods(void)
{
	return (uint8_t)zmk_hid_get_explicit_mods();
}
