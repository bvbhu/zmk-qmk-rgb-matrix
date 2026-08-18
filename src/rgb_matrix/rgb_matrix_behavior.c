/*
 * Copyright (c) 2026 bvbhu
 * SPDX-License-Identifier: MIT
 *
 * rgb_matrix_behavior.c — &rgb_ug 行为驱动 + 设置持久化 + 按键事件监听
 *
 * 替换原 rgb_matrix_settings.c/.h。
 *
 * 行为驱动：
 *   与 ZMK 内置 underglow 驱动使用相同的 DT_DRV_COMPAT 和 devicetree 节点
 *   (rgb_ug)，但仅在 CONFIG_ZMK_RGB_UNDERGLOW=n 时编译，避免冲突。
 *   binding_pressed 直接处理 RGB 命令（原监听器内容），返回 OPAQUE 消费键码，
 *   确保不穿透到其他层。
 *
 * 设置持久化：
 *   通过 Zephyr settings 子系统保存/恢复灯光参数。
 *   RGB_MATRIX_PERSISTENCE=n 或 CONFIG_SETTINGS=n 时整体裁剪。
 *
 * 按键事件监听：
 *   旁路监听 zmk_position_state_changed，转发给 rgb_matrix_handle_position_event，
 *   供 Key Reactive / Framebuffer 灯效使用。
 */

#define DT_DRV_COMPAT zmk_behavior_underglow

#include "lib8tion.h"
#include "rgb_matrix.h"

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <drivers/behavior.h>
#include <dt-bindings/zmk/rgb.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ================================================================
 * 构建时检查：ZMK_RGB_UNDERGLOW 必须关闭
 * ================================================================
 * 本驱动与 ZMK 内置 underglow 驱动争用同一个 rgb_ug 节点。
 * 两者同时编译会导致 device 重复注册链接错误。
 * 模块强制默认 CONFIG_ZMK_RGB_UNDERGLOW=n，如有键盘仓设为 y 则明确报错。 */
#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
#	error "CONFIG_ZMK_RGB_UNDERGLOW=y conflicts with ZMK_RGB_MATRIX. " \
       "Set CONFIG_ZMK_RGB_UNDERGLOW=n in your keyboard .conf (this is the default)."
#endif

/* ================================================================
 * Settings 持久化
 * ================================================================
 * CONFIG_SETTINGS 未启用时（RGB_MATRIX_PERSISTENCE=n 或用户显式关闭），
 * 持久化代码整体裁剪，出厂默认值直接生效。 */

#define RGB_MATRIX_SETTINGS_KEY "rgb_matrix/state"

#if IS_ENABLED(CONFIG_SETTINGS)

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

#else /* !CONFIG_SETTINGS：持久化关闭 */

void rgb_matrix_settings_save(void)
{
}

void rgb_matrix_settings_init(void)
{
	/* 无持久化时直接采用出厂默认值 */
	eeconfig_update_rgb_matrix_default();
}

#endif /* IS_ENABLED(CONFIG_SETTINGS) */

/* ================================================================
 * Shift 检测辅助宏
 * ================================================================ */
#define shift_not_held() (!(zmk_hid_get_explicit_mods() & (MOD_LSFT | MOD_RSFT)))

/* ================================================================
 * &rgb_ug 行为处理
 * ================================================================
 * 在 binding_pressed 中直接处理 RGB 命令，无需 zmk_position_state_changed
 * 监听器拦截。返回 ZMK_BEHAVIOR_OPAQUE 消费键码，不穿透到其他层。
 *
 * binding_released 只消费（不穿透），不做任何处理。 */

static int on_rgb_ug_binding_pressed(struct zmk_behavior_binding* binding,
									 struct zmk_behavior_binding_event event)
{
	(void)event;

	switch(binding->param1)
	{
		case RGB_TOG_CMD:
			rgb_matrix_config.enable ^= 1;
			rgb_task_state = STARTING;
			break;
		case RGB_ON_CMD:
			if(!rgb_matrix_config.enable) rgb_task_state = STARTING;
			rgb_matrix_config.enable = 1;
			break;
		case RGB_OFF_CMD:
			if(rgb_matrix_config.enable) rgb_task_state = STARTING;
			rgb_matrix_config.enable = 0;
			break;
		case RGB_HUI_CMD:
			if(rgb_matrix_config.enable)
			{
				if(shift_not_held())
					rgb_matrix_config.hsv.h += RGB_MATRIX_HUE_STEP;
				else
					rgb_matrix_config.hsv.h -= RGB_MATRIX_HUE_STEP;
			}
			break;
		case RGB_HUD_CMD:
			if(rgb_matrix_config.enable)
			{
				if(shift_not_held())
					rgb_matrix_config.hsv.h -= RGB_MATRIX_HUE_STEP;
				else
					rgb_matrix_config.hsv.h += RGB_MATRIX_HUE_STEP;
			}
			break;
		case RGB_SAI_CMD:
			if(rgb_matrix_config.enable)
			{
				if(shift_not_held())
					rgb_matrix_config.hsv.s = qadd8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP);
				else
					rgb_matrix_config.hsv.s = qsub8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP);
			}
			break;
		case RGB_SAD_CMD:
			if(rgb_matrix_config.enable)
			{
				if(shift_not_held())
					rgb_matrix_config.hsv.s = qsub8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP);
				else
					rgb_matrix_config.hsv.s = qadd8(rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP);
			}
			break;
		case RGB_BRI_CMD:
			if(rgb_matrix_config.enable)
			{
				if(shift_not_held())
					rgb_matrix_config.hsv.v = qadd8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
				else
					rgb_matrix_config.hsv.v = qsub8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
				if(rgb_matrix_config.hsv.v > RGB_MATRIX_MAXIMUM_BRIGHTNESS)
					rgb_matrix_config.hsv.v = RGB_MATRIX_MAXIMUM_BRIGHTNESS;
			}
			break;
		case RGB_BRD_CMD:
			if(rgb_matrix_config.enable)
			{
				if(shift_not_held())
					rgb_matrix_config.hsv.v = qsub8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
				else
					rgb_matrix_config.hsv.v = qadd8(rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP);
				if(rgb_matrix_config.hsv.v > RGB_MATRIX_MAXIMUM_BRIGHTNESS)
					rgb_matrix_config.hsv.v = RGB_MATRIX_MAXIMUM_BRIGHTNESS;
			}
			break;
		case RGB_SPI_CMD:
			if(rgb_matrix_config.enable)
			{
				if(shift_not_held())
					rgb_matrix_config.speed = qadd8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP);
				else
					rgb_matrix_config.speed = qsub8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP);
			}
			break;
		case RGB_SPD_CMD:
			if(rgb_matrix_config.enable)
			{
				if(shift_not_held())
					rgb_matrix_config.speed = qsub8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP);
				else
					rgb_matrix_config.speed = qadd8(rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP);
			}
			break;
		case RGB_EFF_CMD:
			if(rgb_matrix_config.enable)
			{
				uint8_t mode;
				if(shift_not_held())
				{
					mode = rgb_matrix_config.mode + 1;
					if(mode >= RGB_MATRIX_EFFECT_MAX) mode = 1;
				}
				else
				{
					mode = rgb_matrix_config.mode - 1;
					if(mode < 1) mode = RGB_MATRIX_EFFECT_MAX - 1;
				}
				rgb_matrix_config.mode = mode;
				rgb_task_state = STARTING;
			}
			break;
		case RGB_EFR_CMD:
			if(rgb_matrix_config.enable)
			{
				uint8_t mode;
				if(shift_not_held())
				{
					mode = rgb_matrix_config.mode - 1;
					if(mode < 1) mode = RGB_MATRIX_EFFECT_MAX - 1;
				}
				else
				{
					mode = rgb_matrix_config.mode + 1;
					if(mode >= RGB_MATRIX_EFFECT_MAX) mode = 1;
				}
				rgb_matrix_config.mode = mode;
				rgb_task_state = STARTING;
			}
			break;
#ifdef RGB_EFS_CMD
		case RGB_EFS_CMD:
			/* 直接切换到 param2 指定的灯效枚举值（从 1 开始，
			 * 按 rgb_matrix_effects.inc 中已启用灯效的顺序编号），
			 * 无效值或与当前相同时忽略。
			 * 依赖较新 ZMK 的定义 RGB_EFS_CMD，缺失时该 case 不编译。 */
			if(rgb_matrix_config.enable)
			{
				uint8_t mode = (uint8_t)binding->param2;
				if(mode >= 1 && mode < RGB_MATRIX_EFFECT_MAX && mode != rgb_matrix_config.mode)
				{
					rgb_matrix_config.mode = mode;
					rgb_task_state = STARTING;
				}
			}
			break;
#endif
#ifdef RGB_COLOR_HSB_CMD
		case RGB_COLOR_HSB_CMD:
			/* 直接设置颜色，param2 编码 (h << 16) | (s << 8) | v，均为 0-255。
			 * 依赖较新 ZMK 支持的 RGB_COLOR_HSB_CMD，未定义时该 case 不编译。 */
			if(rgb_matrix_config.enable)
			{
				uint32_t hsb = (uint32_t)binding->param2;
				rgb_matrix_config.hsv.h = (uint8_t)((hsb >> 16) & 0xFF);
				rgb_matrix_config.hsv.s = (uint8_t)((hsb >> 8) & 0xFF);
				rgb_matrix_config.hsv.v = (uint8_t)(hsb & 0xFF);
				if(rgb_matrix_config.hsv.v > RGB_MATRIX_MAXIMUM_BRIGHTNESS)
					rgb_matrix_config.hsv.v = RGB_MATRIX_MAXIMUM_BRIGHTNESS;
			}
			break;
#endif
		default:
			break;
	}

	rgb_matrix_settings_save();
	return ZMK_BEHAVIOR_OPAQUE;
}

static int on_rgb_ug_binding_released(struct zmk_behavior_binding* binding,
									  struct zmk_behavior_binding_event event)
{
	(void)binding;
	(void)event;
	return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api rgb_ug_driver_api = {
	.binding_pressed = on_rgb_ug_binding_pressed,
	.binding_released = on_rgb_ug_binding_released,
};

/* ===== 设备注册 =====
 * 与 ZMK 内置 underglow 驱动使用相同的 DT_DRV_COMPAT 和 rgb_ug 节点。
 * 内置驱动不编译时本驱动接管，内置驱动编译时本文件因 #if IS_ENABLED 报错。 */
BEHAVIOR_DT_DEFINE(DT_NODELABEL(rgb_ug), NULL, NULL, NULL, NULL, POST_KERNEL,
				   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &rgb_ug_driver_api);

/* ================================================================
 * 按键事件监听器 (Key Reactive / Framebuffer 灯效)
 * ================================================================
 * 监听所有按键的按下/释放，将 ZMK position 转换为物理 (row, col)
 * 后转发给 rgb_matrix_handle_key_event。
 * 不捕获事件 (返回 BUBBLE)，仅旁路记录命中点，不影响其他监听器。 */
#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED) || \
	(defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP))

/* ===== 按键事件 position → (row, col) 映射表，由 post_config.h 生成 */
static const struct
{
	uint8_t row;
	uint8_t col;
} zmk_rgb_pos_to_rc[RGB_MATRIX_POS_TO_RC_LEN] = RGB_MATRIX_POS_TO_RC_MAP;

static int rgb_matrix_key_event_listener(const zmk_event_t* eh)
{
	const struct zmk_position_state_changed* ev = as_zmk_position_state_changed(eh);
	if(ev == NULL) return ZMK_EV_EVENT_BUBBLE;
	
	uint8_t row, column;
	if(ev->position >= RGB_MATRIX_POS_TO_RC_LEN) return ZMK_EV_EVENT_BUBBLE;
	row = zmk_rgb_pos_to_rc[ev->position].row;
	column = zmk_rgb_pos_to_rc[ev->position].col;
	rgb_matrix_handle_key_event(row, column, ev->state);
	return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(rgb_matrix_key_event, rgb_matrix_key_event_listener);
ZMK_SUBSCRIPTION(rgb_matrix_key_event, zmk_position_state_changed);
#endif