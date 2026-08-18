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
 * rgb_matrix_driver.h — LED 驱动抽象 + 设置持久化 API
 *
 * 对应 QMK 的 rgb_matrix_drivers.h:40-49（rgb_matrix_driver_t）。
 * 本模块使用 ZMK led_strip API 实现，不依赖 QMK 的 IS31/WS2812 驱动头文件。
 *
 * 设置持久化基于 Zephyr settings 子系统（非 QMK eeconfig），
 * 由 rgb_matrix_driver.c 实现，供 rgb_matrix.c 中的 QMK 风格控制函数调用。
 */

#pragma once

#include <stdint.h>

/* ===== LED 驱动抽象（与 QMK rgb_matrix_driver_t 结构一致）===== */
typedef struct {
    /* Perform any initialisation required for the other driver functions to work. */
    void (*init)(void);
    /* Set the colour of a single LED in the buffer. */
    void (*set_color)(int index, uint8_t r, uint8_t g, uint8_t b);
    /* Set the colour of all LEDS on the keyboard in the buffer. */
    void (*set_color_all)(uint8_t r, uint8_t g, uint8_t b);
    /* Flush any buffered changes to the hardware. */
    void (*flush)(void);
} rgb_matrix_driver_t;

extern const rgb_matrix_driver_t rgb_matrix_driver;

/* ===== 设置持久化 API（基于 Zephyr settings 子系统，非 QMK eeconfig）===== */
void rgb_matrix_settings_init(void);
void rgb_matrix_settings_save(void);
