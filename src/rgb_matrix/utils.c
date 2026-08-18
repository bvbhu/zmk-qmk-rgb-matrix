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

#include "utils.h"

#include <stdlib.h>

uint8_t qadd8(uint8_t i, uint8_t j)
{
	return (i < 255 - j) ? (i + j) : 255;
}

uint8_t qadd8lim(uint8_t i, uint8_t j, uint8_t lim)
{
	return (i < lim - j) ? (i + j) : lim;
}

uint8_t qsub8(uint8_t i, uint8_t j)
{
	return (i >= j) ? (i - j) : 0;
}

int8_t abs8(int8_t i)
{
	return (i < 0) ? (int8_t)(-i) : i;
}

uint8_t sqrt16(uint16_t x)
{
	uint16_t rem = 0;
	uint16_t root = 0;
	for(uint8_t i = 0; i < 8; i++)
	{
		rem = (rem << 2) | ((uint16_t)(x >> 14) & 0x03);
		x <<= 2;
		root <<= 1;
		uint16_t try = (root << 1) | 1;
		if(rem >= try)
		{
			rem -= try;
			root |= 1;
		}
	}
	return (uint8_t)root;
}

/* ===== 定标运算 ===== */

uint8_t scale8(uint8_t i, uint8_t scale)
{
	return (uint8_t)(((uint16_t)i * (uint16_t)scale) >> 8);
}

uint16_t scale16by8(uint16_t i, uint8_t scale)
{
	return (uint16_t)(((uint32_t)i * (uint32_t)scale) >> 8);
}

uint8_t random8(void)
{
	return (uint8_t)(rand() & 0xFF);
}

uint16_t random16(void)
{
	return (uint16_t)(rand() & 0xFFFF);
}

uint8_t random8_max(uint8_t lim)
{
	uint8_t r = random8();
	return (uint8_t)(((uint16_t)r * lim) >> 8);
}

uint8_t random8_min_max(uint8_t min, uint8_t lim)
{
	uint8_t delta = (uint8_t)(lim - min);
	return (uint8_t)(random8_max(delta) + min);
}

uint8_t sin8(uint8_t theta)
{
	uint8_t x = theta & 0x7F;
	if(x > 64)
	{
		x = 128 - x;
	}
	uint16_t a = (uint16_t)x * (uint16_t)(128 - x);
	uint8_t v = (uint8_t)((a * 127U + 2048U) >> 12);

	return (theta & 0x80) ? (uint8_t)(128 - v) : (uint8_t)(128 + v);
}

uint8_t cos8(uint8_t theta)
{
	return sin8((uint8_t)(theta + 64));
}

uint8_t atan2_8(int16_t dy, int16_t dx)
{
	static const uint8_t atan_table[129] = {
		0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 10,
		10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 17, 17,
		17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23,
		23, 23, 24, 24, 24, 24, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 28, 28, 28, 28,
		28, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 32, 32, 32, 32
	};
	if(dx == 0 && dy == 0) return 0;
	uint32_t ax = (dx >= 0) ? (uint32_t)dx : (uint32_t)(-dx);
	uint32_t ay = (dy >= 0) ? (uint32_t)dy : (uint32_t)(-dy);
	uint8_t base = (ax >= ay) ? atan_table[(ay << 7) / ax] : (64 - atan_table[(ax << 7) / ay]);
	if(dx >= 0)
		return (dy >= 0) ? base : (uint8_t)(256 - base);
	else
		return (dy >= 0) ? (uint8_t)(128 - base) : (uint8_t)(128 + base);
}

rgb_t hsv_to_rgb(hsv_t hsv)
{
	rgb_t rgb;
	uint8_t h = hsv.h, s = hsv.s, v = hsv.v;
	if(hsv.s == 0)
	{
		rgb.r = v, rgb.g = v, rgb.b = v;
		return rgb;
	}

	uint16_t hue = h * 6;
	uint8_t f = hue & 0xFF;
	uint8_t sf = scale8(s, f), st = scale8(s, 255 - f);
	uint8_t p = v - scale8(s, v), q = v - scale8(sf, v), t = v - scale8(st, v);
	switch(hue >> 8)
	{

		case 1:
			rgb.r = q, rgb.g = v, rgb.b = p;
			break;
		case 2:
			rgb.r = p, rgb.g = v, rgb.b = t;
			break;
		case 3:
			rgb.r = p, rgb.g = q, rgb.b = v;
			break;
		case 4:
			rgb.r = t, rgb.g = p, rgb.b = v;
			break;
		case 5:
			rgb.r = v, rgb.g = p, rgb.b = q;
			break;
		default:
			rgb.r = v, rgb.g = t, rgb.b = p;
			break;
	}
	return rgb;
}

uint16_t timer_read(void)
{
	return (uint16_t)k_uptime_get_32();
}

uint16_t timer_elapsed(uint16_t last)
{
	return (uint16_t)(k_uptime_get_32() - last);
}

uint16_t timer_read_fast(void)
{
	return (uint16_t)k_uptime_get_32();
}

uint16_t timer_elapsed_fast(uint16_t last)
{
	return (uint16_t)(k_uptime_get_32() - last);
}

uint32_t sync_timer_read32(void)
{
	return k_uptime_get_32();
}

uint32_t sync_timer_elapsed32(uint32_t last)
{
	return k_uptime_get_32() - last;
}
