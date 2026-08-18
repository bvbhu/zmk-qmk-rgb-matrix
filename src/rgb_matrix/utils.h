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

uint8_t qadd8(uint8_t i, uint8_t j);
uint8_t qadd8lim(uint8_t i, uint8_t j, uint8_t lim);
uint8_t qsub8(uint8_t i, uint8_t j);
int8_t abs8(int8_t i);
uint8_t sqrt16(uint16_t x);
uint8_t scale8(uint8_t i, uint8_t scale);
uint16_t scale16by8(uint16_t i, uint8_t scale);
uint8_t random8(void);
uint16_t random16(void);
uint8_t random8_max(uint8_t lim);
uint8_t random8_min_max(uint8_t min, uint8_t lim);
uint8_t sin8(uint8_t theta);
uint8_t cos8(uint8_t theta);
uint8_t atan2_8(int16_t dy, int16_t dx);
rgb_t hsv_to_rgb(hsv_t hsv);

typedef uint16_t fast_timer_t;

uint16_t timer_read(void);
uint16_t timer_elapsed(uint16_t last);
fast_timer_t timer_read_fast(void);
fast_timer_t timer_elapsed_fast(fast_timer_t last);
uint32_t sync_timer_read32(void);
uint32_t sync_timer_elapsed32(uint32_t last);