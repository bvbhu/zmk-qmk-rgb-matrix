/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: MIT
 *
 * qmk_compat.h — QMK API 兼容
 * 为 keymap.c 等 ZMK 上层代码提供 QMK 风格的函数。
 * 时间相关基于 Zephyr k_uptime_get_32。
 */

#pragma once

#include <stdint.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

/* HID 指示灯（Caps/Num/Scroll Lock 状态）：
 * 默认启用（模块 Kconfig 已设 ZMK_HID_INDICATORS default y）。
 * 未启用时 host_keyboard_led_state() 不定义，keymap.c 中的
 * 指示灯回调不可使用该函数（编译报错提示开启配置）。 */
#ifdef CONFIG_ZMK_HID_INDICATORS
#	include <zmk/hid_indicators.h>
#endif

/* MOD_LCTL / MOD_RCTL / MOD_LSFT / MOD_RSFT /
 * MOD_LALT / MOD_RALT / MOD_LGUI / MOD_RGUI
 * 由 <zmk/hid.h> 定义（HID Usage Page 7 标准值），无需重复定义。 */

#define PACKED __attribute__((packed))

typedef struct
{
	uint8_t num_lock : 1;
	uint8_t caps_lock : 1;
	uint8_t scroll_lock : 1;
} led_t;

#ifdef CONFIG_ZMK_HID_INDICATORS
led_t host_keyboard_led_state(void);
#endif

/* ===== Modifier masks ===== */
#define MOD_MASK_CTRL (MOD_LCTL | MOD_RCTL)
#define MOD_MASK_SHIFT (MOD_LSFT | MOD_RSFT)
#define MOD_MASK_ALT (MOD_LALT | MOD_RALT)
#define MOD_MASK_GUI (MOD_LGUI | MOD_RGUI)

/* ===== get_highest_layer(layer_state) ===== */
#define layer_state NULL
#define get_highest_layer(...) ((uint8_t)zmk_keymap_highest_layer_active())

/* ===== get_mods ===== */
uint8_t get_mods(void);

typedef uint16_t fast_timer_t;

uint16_t timer_read(void);
uint16_t timer_elapsed(uint16_t last);
fast_timer_t timer_read_fast(void);
fast_timer_t timer_elapsed_fast(fast_timer_t last);
uint32_t sync_timer_read32(void);
uint32_t sync_timer_elapsed32(uint32_t last);

/* ===== color.h ===== */
/* RGB 颜色（0-255 各通道） */
typedef struct PACKED rgb_t
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb_t;
typedef rgb_t RGB;
typedef rgb_t rgb_led_t;

/* HSV 颜色（全部 0-255，包括 hue） */
typedef struct PACKED hsv_t
{
	uint8_t h;
	uint8_t s;
	uint8_t v;
} hsv_t;
typedef hsv_t HSV;

bool is_keyboard_master(void);
bool is_keyboard_left(void);