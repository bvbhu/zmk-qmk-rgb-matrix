/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * RGB Matrix — ZMK Module Public API
 *
 * 键盘仓 keymap.c 通过此头文件获取 RGB Matrix 类型定义与函数声明。
 * 键盘仓 Kconfig 值通过 #include "rgb_matrix_generated_config.h"
 * 被 rgb_matrix_types.h 引入（由 snippet 从 .conf 自动生成），
 * 必须定义 MATRIX_ROWS、MATRIX_COLS、RGB_MATRIX_LED_COUNT 等硬件参数
 * 以及 CONFIG_RGB_MATRIX_EFFECT_* 灯效选项。
 */

#pragma once

#include "controller/rgb_matrix.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===== 键盘仓提供的符号（由键盘仓 keymap.c 定义）=====
 * 在键盘仓 config/keymap.c 中定义以下符号：
 *   led_config_t g_led_config;
 *   bool rgb_matrix_indicators_advanced_user(uint8_t, uint8_t);
 */

#ifdef __cplusplus
}
#endif
