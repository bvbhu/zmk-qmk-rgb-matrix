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
 * rgb_matrix_driver.c — LED 驱动实现（ZMK led_strip）+ 设置持久化（Zephyr settings）
 *
 * 1. LED 驱动：
 *    用 ZMK led_strip API 实现 rgb_matrix_driver_t，对应 QMK 的
 *    rgb_matrix_drivers.c。pixel buffer 与 device 句柄在此文件内私有。
 *
 * 2. 设置持久化：
 *    通过 Zephyr settings 子系统保存/恢复灯光参数（非 QMK eeconfig）。
 *    CONFIG_RGB_MATRIX_PERSISTENCE=n 或 CONFIG_SETTINGS=n 时整体裁剪。
 */

#include "rgb_matrix_driver.h"
#include "rgb_matrix.h"
#include "utils.h"

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ================================================================
 * 1. LED 驱动（基于 ZMK led_strip API）
 * ================================================================
 * pixel buffer 显式零初始化为全黑（熄灭），跨帧保留数据，
 * raindrops 等灯效依赖此行为。 */

static struct led_rgb       led_strip_pixels[RGB_MATRIX_LED_COUNT] = {0};
static const struct device *led_strip_dev;

static void driver_init(void)
{
	led_strip_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_underglow));
	if(!device_is_ready(led_strip_dev))
	{
		LOG_ERR("LED strip device not ready");
		led_strip_dev = NULL;
	}
}

static void driver_set_color(int index, uint8_t r, uint8_t g, uint8_t b)
{
	led_strip_pixels[index].r = r;
	led_strip_pixels[index].g = g;
	led_strip_pixels[index].b = b;
}

static void driver_set_color_all(uint8_t r, uint8_t g, uint8_t b)
{
	for(uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++)
	{
		led_strip_pixels[i].r = r;
		led_strip_pixels[i].g = g;
		led_strip_pixels[i].b = b;
	}
}

static void driver_flush(void)
{
	if(led_strip_dev)
	{
		led_strip_update_rgb(led_strip_dev, led_strip_pixels, RGB_MATRIX_LED_COUNT);
	}
}

const rgb_matrix_driver_t rgb_matrix_driver = {
	.init          = driver_init,
	.set_color     = driver_set_color,
	.set_color_all = driver_set_color_all,
	.flush         = driver_flush,
};

/* ================================================================
 * 2. 设置持久化（Zephyr settings 子系统，非 QMK eeconfig）
 * ================================================================
 * 门控条件与 Kconfig 帮助文本一致：RGB_MATRIX_PERSISTENCE=n（总开关）
 * 或 CONFIG_SETTINGS=n（Zephyr 未启用 settings 栈）时，持久化代码整体
 * 裁剪，出厂默认值直接生效。 */

#define RGB_MATRIX_SETTINGS_KEY "rgb_matrix/state"

#if IS_ENABLED(CONFIG_RGB_MATRIX_PERSISTENCE) && IS_ENABLED(CONFIG_SETTINGS)

static bool settings_loaded = false;
static struct k_work_delayable rgb_matrix_save_work;

static void rgb_matrix_save_handler(struct k_work* work)
{
	(void)work;
	uint64_t raw = rgb_matrix_config.raw;
	settings_save_one(RGB_MATRIX_SETTINGS_KEY, &raw, sizeof(raw));
}

/* 防抖保存：RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE 默认跟随 ZMK 全局
 * CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE，也可在键盘仓 .conf 覆盖。
 *  - 负值：禁用持久化，不启动调度，不写 Flash
 *  - 0：立即写入
 *  - 正值：尾缘防抖，停止按键后经过此时间才写一次 Flash */
void rgb_matrix_settings_save(void)
{
	if(RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE < 0)
	{
		return;
	}
	k_work_reschedule(&rgb_matrix_save_work, K_MSEC(RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE));
}

static int rgb_matrix_settings_set(const char* name, size_t len, settings_read_cb read_cb, void* cb_arg)
{
	(void)len; /* NVS 后端只传 1 字节哨兵值，不代表真实数据长度 */

	if(strcmp(name, "state") != 0)
	{
		return -ENOENT;
	}

	uint64_t raw;
	ssize_t read_len = read_cb(cb_arg, &raw, sizeof(raw));
	if(read_len != sizeof(raw))
	{
		return -EINVAL;
	}

	rgb_matrix_config.raw = raw;
	settings_loaded = true;
	return 0;
}

static int rgb_matrix_settings_export(int (*cb)(const char* name,
												const void* value,
												size_t val_len))
{
	uint64_t raw = rgb_matrix_config.raw;
	return cb("state", &raw, sizeof(raw));
}

static int rgb_matrix_settings_commit(void)
{
	/* settings_load() 完成后调用，若未加载到有效配置则写入默认值 */
	if(!settings_loaded || !rgb_matrix_config.mode || rgb_matrix_config.mode >= RGB_MATRIX_EFFECT_MAX)
	{
		eeconfig_update_rgb_matrix_default();
	}
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(rgb_matrix, "rgb_matrix",
							   NULL,						/* h_get */
							   rgb_matrix_settings_set,		/* h_set */
							   rgb_matrix_settings_commit,	/* h_commit */
							   rgb_matrix_settings_export); /* h_export */

/* 初始化 settings 延迟保存工作项 */
void rgb_matrix_settings_init(void)
{
	k_work_init_delayable(&rgb_matrix_save_work, rgb_matrix_save_handler);
}

#else /* !(CONFIG_RGB_MATRIX_PERSISTENCE && CONFIG_SETTINGS)：持久化关闭 */

void rgb_matrix_settings_save(void)
{
}

void rgb_matrix_settings_init(void)
{
	/* 无持久化时直接采用出厂默认值 */
	eeconfig_update_rgb_matrix_default();
}

#endif /* IS_ENABLED(CONFIG_RGB_MATRIX_PERSISTENCE) && IS_ENABLED(CONFIG_SETTINGS) */
