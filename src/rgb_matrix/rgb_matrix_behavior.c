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
 * rgb_matrix_behavior.c — &rgb_ug 行为驱动 + 按键事件监听
 *
 * 1. 行为驱动：
 *    与 ZMK 内置 underglow 驱动使用相同的 DT_DRV_COMPAT 和 devicetree 节点
 *    (rgb_ug)，但仅在 CONFIG_ZMK_RGB_UNDERGLOW=n 时编译，避免冲突。
 *    binding_pressed 调用 rgb_matrix.c 中实现的 QMK 风格控制函数
 *    （rgb_matrix_toggle / rgb_matrix_increase_hue 等），返回 OPAQUE 消费键码。
 *    所有调节型命令经 Shift 反向：Shift 未按住 → 正向，Shift 按住 → 反向。
 *
 * 2. 按键事件监听：
 *    旁路监听 zmk_position_state_changed，经 rgb_matrix_behavior.h 提供的
 *    position → (row, col) 映射表转发给 rgb_matrix_handle_key_event，
 *    供 Key Reactive / Framebuffer 灯效使用。
 *
 * 设置持久化与 LED 驱动已移至 rgb_matrix_driver.c。
 */

#define DT_DRV_COMPAT zmk_behavior_underglow

#include "rgb_matrix.h"
#include "rgb_matrix_behavior.h"

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>
#include <dt-bindings/zmk/rgb.h>
#include <zephyr/devicetree.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* ================================================================
 * &rgb_ug 行为驱动
 * ================================================================
 * binding_pressed 将 RGB 命令分发到 rgb_matrix.c 中的 QMK 风格控制函数。
 * 持久化（防抖保存）由各控制函数内部调用 rgb_matrix_settings_save() 完成。
 * binding_released 只消费（不穿透），不做任何处理。 */

static inline bool rgb_matrix_shift_not_held(void)
{
	return !(zmk_hid_get_explicit_mods() & (MOD_LSFT | MOD_RSFT));
}

static int on_rgb_ug_binding_pressed(struct zmk_behavior_binding* binding,
									 struct zmk_behavior_binding_event event)
{
	(void)event;

	switch(binding->param1)
	{
		case RGB_TOG_CMD:
			rgb_matrix_toggle();
			break;
		case RGB_ON_CMD:
			rgb_matrix_enable();
			break;
		case RGB_OFF_CMD:
			rgb_matrix_disable();
			break;
		case RGB_HUI_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_increase_hue();
			else rgb_matrix_decrease_hue();
			break;
		case RGB_HUD_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_decrease_hue();
			else rgb_matrix_increase_hue();
			break;
		case RGB_SAI_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_increase_sat();
			else rgb_matrix_decrease_sat();
			break;
		case RGB_SAD_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_decrease_sat();
			else rgb_matrix_increase_sat();
			break;
		case RGB_BRI_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_increase_val();
			else rgb_matrix_decrease_val();
			break;
		case RGB_BRD_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_decrease_val();
			else rgb_matrix_increase_val();
			break;
		case RGB_SPI_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_increase_speed();
			else rgb_matrix_decrease_speed();
			break;
		case RGB_SPD_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_decrease_speed();
			else rgb_matrix_increase_speed();
			break;
		case RGB_EFF_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_step();
			else rgb_matrix_step_reverse();
			break;
		case RGB_EFR_CMD:
			if(rgb_matrix_shift_not_held()) rgb_matrix_step_reverse();
			else rgb_matrix_step();
			break;
#ifdef RGB_EFS_CMD
		case RGB_EFS_CMD:
			rgb_matrix_mode((uint8_t)binding->param2);
			break;
#endif
#ifdef RGB_COLOR_HSB_CMD
		case RGB_COLOR_HSB_CMD:
		{
			uint32_t hsb = (uint32_t)binding->param2;
			rgb_matrix_sethsv((uint16_t)((hsb >> 16) & 0xFF), (uint8_t)((hsb >> 8) & 0xFF), (uint8_t)(hsb & 0xFF));
		}
			break;
#endif
		default:
			break;
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
 * 按键事件监听器 (Key Reactive / Framebuffer 灯效)
 * ================================================================
 * 监听所有按键的按下/释放，将 ZMK position 经 rgb_matrix_behavior.h 的
 * 映射表转换为物理 (row, col) 后转发给 rgb_matrix_handle_key_event。
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
