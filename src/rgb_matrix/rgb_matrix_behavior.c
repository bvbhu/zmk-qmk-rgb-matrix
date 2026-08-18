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
 * rgb_matrix_behavior.c — &rgb_ug 行为驱动 + 设置持久化 + 按键事件监听
 *
 * 替换原 rgb_matrix_settings.c/.h。三个职责在同一文件内分节实现：
 *
 * 1. 设置持久化：
 *    通过 Zephyr settings 子系统保存/恢复灯光参数。
 *    CONFIG_RGB_MATRIX_PERSISTENCE=n 或 CONFIG_SETTINGS=n 时整体裁剪，
 *    出厂默认值直接生效（与 Kconfig 帮助文本一致）。
 *
 * 2. 行为驱动：
 *    与 ZMK 内置 underglow 驱动使用相同的 DT_DRV_COMPAT 和 devicetree 节点
 *    (rgb_ug)，但仅在 CONFIG_ZMK_RGB_UNDERGLOW=n 时编译，避免冲突。
 *    binding_pressed 直接处理全部 RGB 命令（原监听器内容），返回 OPAQUE
 *    消费键码，确保不穿透到其他层。所有调节命令经 Shift 反向；仅当确实
 *    改动配置时才触发防抖保存，无操作命令不写 Flash。
 *
 * 3. 按键事件监听：
 *    旁路监听 zmk_position_state_changed，经 rgb_matrix_behavior.h 提供的
 *    position → (row, col) 编译期映射表转发给 rgb_matrix_handle_key_event，
 *    供 Key Reactive / Framebuffer 灯效使用。
 */

#define DT_DRV_COMPAT zmk_behavior_underglow

#include "rgb_matrix.h"
#include "rgb_matrix_behavior.h"
#include "utils.h"

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <drivers/behavior.h>
#include <dt-bindings/zmk/rgb.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ================================================================
 * 1. Settings 持久化
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

/* ================================================================
 * 2. &rgb_ug 行为驱动
 * ================================================================
 * 在 binding_pressed 中直接处理 RGB 命令，无需 zmk_position_state_changed
 * 监听器拦截。返回 ZMK_BEHAVIOR_OPAQUE 消费键码，不穿透到其他层。
 * binding_released 只消费（不穿透），不做任何处理。
 *
 * 所有"调节型"命令（H/S/V/SPD/EFF）共享同一套 Shift 反向语义：
 *   Shift 未按住 → 正向 (+1)；Shift 按住 → 反向 (-1)。
 * 调节仅在 RGB 启用时生效；辅助函数返回"是否实际改动"，仅在有改动时
 * 触发防抖保存，避免无操作命令（RGB 关闭、越界模式等）写 Flash。 */

static inline bool rgb_matrix_shift_not_held(void)
{
	return !(zmk_hid_get_explicit_mods() & (MOD_LSFT | MOD_RSFT));
}

/* 调节方向：Shift 未按住为 +1，按住为 -1 */
static inline int8_t rgb_matrix_adjust_dir(void)
{
	return rgb_matrix_shift_not_held() ? 1 : -1;
}

/* 通用饱和/限幅调节：正向 qadd8lim(限幅 limit)，反向 qsub8(夹到 0) */
static bool rgb_matrix_adjust_channel(uint8_t* channel, uint8_t step, uint8_t limit, int8_t dir)
{
	if(!rgb_matrix_config.enable)
	{
		return false;
	}
	*channel = (dir > 0) ? qadd8lim(*channel, step, limit) : qsub8(*channel, step);
	return true;
}

/* 色相：0-255 环形回绕，不需要限幅 */
static bool rgb_matrix_adjust_hue(int8_t dir)
{
	if(!rgb_matrix_config.enable)
	{
		return false;
	}
	rgb_matrix_config.hsv.h = (dir > 0) ? (uint8_t)(rgb_matrix_config.hsv.h + RGB_MATRIX_HUE_STEP)
										: (uint8_t)(rgb_matrix_config.hsv.h - RGB_MATRIX_HUE_STEP);
	return true;
}

static bool rgb_matrix_adjust_sat(int8_t dir)
{
	return rgb_matrix_adjust_channel(&rgb_matrix_config.hsv.s, RGB_MATRIX_SAT_STEP, 255, dir);
}

static bool rgb_matrix_adjust_val(int8_t dir)
{
	return rgb_matrix_adjust_channel(&rgb_matrix_config.hsv.v, RGB_MATRIX_VAL_STEP,
									 RGB_MATRIX_MAXIMUM_BRIGHTNESS, dir);
}

static bool rgb_matrix_adjust_speed(int8_t dir)
{
	return rgb_matrix_adjust_channel(&rgb_matrix_config.speed, RGB_MATRIX_SPD_STEP, 255, dir);
}

/* 直接设定模式：需启用、范围合法 (1 .. EFFECT_MAX-1) 且与当前不同 */
static bool rgb_matrix_set_mode(uint8_t mode)
{
	if(!rgb_matrix_config.enable || mode < 1 || mode >= RGB_MATRIX_EFFECT_MAX ||
	   mode == rgb_matrix_config.mode)
	{
		return false;
	}
	rgb_matrix_config.mode = mode;
	rgb_task_state = STARTING;
	return true;
}

/* 循环切换模式：+1 / -1，越界回绕到另一端点 */
static bool rgb_matrix_cycle_mode(int8_t dir)
{
	int16_t mode = (int16_t)rgb_matrix_config.mode + dir;
	if(mode < 1)
	{
		mode = RGB_MATRIX_EFFECT_MAX - 1;
	}
	else if(mode >= RGB_MATRIX_EFFECT_MAX)
	{
		mode = 1;
	}
	return rgb_matrix_set_mode((uint8_t)mode);
}

/* 直接设定 HSB 颜色（param2 打包为 0x00HHSSBB），亮度限幅到最大亮度 */
static bool rgb_matrix_set_hsb(uint32_t hsb)
{
	if(!rgb_matrix_config.enable)
	{
		return false;
	}
	rgb_matrix_config.hsv.h = (uint8_t)((hsb >> 16) & 0xFF);
	rgb_matrix_config.hsv.s = (uint8_t)((hsb >> 8) & 0xFF);
	rgb_matrix_config.hsv.v = qadd8lim((uint8_t)(hsb & 0xFF), 0, RGB_MATRIX_MAXIMUM_BRIGHTNESS);
	return true;
}

static int on_rgb_ug_binding_pressed(struct zmk_behavior_binding* binding,
									 struct zmk_behavior_binding_event event)
{
	(void)event;
	bool applied = false;

	switch(binding->param1)
	{
		case RGB_TOG_CMD:
			rgb_matrix_config.enable ^= 1;
			rgb_task_state = STARTING;
			applied = true;
			break;
		case RGB_ON_CMD:
			if(!rgb_matrix_config.enable) rgb_task_state = STARTING;
			rgb_matrix_config.enable = 1;
			applied = true;
			break;
		case RGB_OFF_CMD:
			if(rgb_matrix_config.enable) rgb_task_state = STARTING;
			rgb_matrix_config.enable = 0;
			applied = true;
			break;
		case RGB_HUI_CMD:
			applied |= rgb_matrix_adjust_hue(rgb_matrix_adjust_dir());
			break;
		case RGB_HUD_CMD:
			applied |= rgb_matrix_adjust_hue(-rgb_matrix_adjust_dir());
			break;
		case RGB_SAI_CMD:
			applied |= rgb_matrix_adjust_sat(rgb_matrix_adjust_dir());
			break;
		case RGB_SAD_CMD:
			applied |= rgb_matrix_adjust_sat(-rgb_matrix_adjust_dir());
			break;
		case RGB_BRI_CMD:
			applied |= rgb_matrix_adjust_val(rgb_matrix_adjust_dir());
			break;
		case RGB_BRD_CMD:
			applied |= rgb_matrix_adjust_val(-rgb_matrix_adjust_dir());
			break;
		case RGB_SPI_CMD:
			applied |= rgb_matrix_adjust_speed(rgb_matrix_adjust_dir());
			break;
		case RGB_SPD_CMD:
			applied |= rgb_matrix_adjust_speed(-rgb_matrix_adjust_dir());
			break;
		case RGB_EFF_CMD:
			applied |= rgb_matrix_cycle_mode(rgb_matrix_adjust_dir());
			break;
		case RGB_EFR_CMD:
			applied |= rgb_matrix_cycle_mode(-rgb_matrix_adjust_dir());
			break;
#ifdef RGB_EFS_CMD
		case RGB_EFS_CMD:
			applied |= rgb_matrix_set_mode((uint8_t)binding->param2);
			break;
#endif
#ifdef RGB_COLOR_HSB_CMD
		case RGB_COLOR_HSB_CMD:
			applied |= rgb_matrix_set_hsb((uint32_t)binding->param2);
			break;
#endif
		default:
			break;
	}

	if(applied)
	{
		rgb_matrix_settings_save();
	}
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
static int rgb_ug_init(const struct device* dev)
{
	(void)dev;
#if defined(RGB_MATRIX_POS_TO_RC_LEN)
	rgb_matrix_pos_to_rc_init();
#endif
	return 0;
}
BEHAVIOR_DT_DEFINE(DT_NODELABEL(rgb_ug), rgb_ug_init, NULL, NULL, NULL, POST_KERNEL,
				   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &rgb_ug_driver_api);

/* ================================================================
 * 3. 按键事件监听器 (Key Reactive / Framebuffer 灯效)
 * ================================================================
 * 监听所有按键的按下/释放，将 ZMK position 经 rgb_matrix_behavior.h 的
 * 编译期映射表转换为物理 (row, col) 后转发给 rgb_matrix_handle_key_event。
 * 映射宏不存在（灯效未启用或缺少 zmk_matrix_transform 节点）时本段整体
 * 裁剪；监听器不捕获事件 (返回 BUBBLE)，仅旁路记录命中点。 */
#if defined(RGB_MATRIX_POS_TO_RC_LEN)

/* 运行时初始化的 position → (row, col) 映射表。
 * 不使用 LISTIFY 编译期初始化，因为 Zephyr LISTIFY 最多支持 91 个元素，
 * 而 96 键/全尺寸键盘的矩阵变换常超过此限制。
 *
 * 使用 DT_PROP 获取完整数组再运行时索引，避免 DT_PROP_BY_IDX
 * 需要编译期常量的限制。 */
static const uint32_t zmk_rgb_map_data[RGB_MATRIX_POS_TO_RC_LEN] = DT_PROP(ZMK_RGB_MT_NODE, map);

static struct
{
	uint8_t row;
	uint8_t col;
} zmk_rgb_pos_to_rc[RGB_MATRIX_POS_TO_RC_LEN];

void rgb_matrix_pos_to_rc_init(void)
{
	for(int i = 0; i < RGB_MATRIX_POS_TO_RC_LEN; i++)
	{
		zmk_rgb_pos_to_rc[i].row = (uint8_t)KT_ROW(zmk_rgb_map_data[i]);
		zmk_rgb_pos_to_rc[i].col = (uint8_t)KT_COL(zmk_rgb_map_data[i]);
	}
}

static int rgb_matrix_key_event_listener(const zmk_event_t* eh)
{
	const struct zmk_position_state_changed* ev = as_zmk_position_state_changed(eh);
	if(ev == NULL || ev->position >= RGB_MATRIX_POS_TO_RC_LEN)
	{
		return ZMK_EV_EVENT_BUBBLE;
	}

	rgb_matrix_handle_key_event(zmk_rgb_pos_to_rc[ev->position].row,
								zmk_rgb_pos_to_rc[ev->position].col,
								ev->state);
	return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(rgb_matrix_key_event, rgb_matrix_key_event_listener);
ZMK_SUBSCRIPTION(rgb_matrix_key_event, zmk_position_state_changed);
#endif
