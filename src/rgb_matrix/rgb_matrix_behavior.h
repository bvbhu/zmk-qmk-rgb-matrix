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
 * rgb_matrix_behavior.h — rgb_matrix_behavior.c 的私有头文件
 *
 * 为 Key Reactive / Framebuffer 灯效提供 position → (row, col) 编译期映射表：
 *   RGB_MATRIX_POS_TO_RC_LEN  — 映射表长度（= zmk_matrix_transform map 长度）
 *   RGB_MATRIX_POS_TO_RC_MAP  — 映射表初始化器（{row, col} 数组）
 *
 * 仅当对应灯效启用且存在 zmk_matrix_transform 节点时才定义这两个宏；
 * rgb_matrix_behavior.c 以 #if defined(RGB_MATRIX_POS_TO_RC_LEN) 决定
 * 是否编译按键事件监听器，因此本头文件是"是否启用监听"的唯一门控。
 */

#pragma once

#include <zephyr/sys/util.h>

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

#if defined(RGB_MATRIX_KEYREACTIVE_ENABLED) || \
	(defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS) && defined(ENABLE_RGB_MATRIX_TYPING_HEATMAP))

/* 保存 RC 原始定义（modifiers.h 的 RC(keycode)），避免与 matrix_transform.h 的 RC(row,col) 冲突 */
#pragma push_macro("RC")
#undef RC

#	include <dt-bindings/zmk/matrix_transform.h>
#	include <zephyr/devicetree.h>

#	if DT_HAS_CHOSEN(zmk_matrix_transform)
#		define ZMK_RGB_MT_NODE DT_CHOSEN(zmk_matrix_transform)
#		define ZMK_RGB_MT_LEN DT_PROP_LEN(ZMK_RGB_MT_NODE, map)
#		define ZMK_RGB_POS_RC_ENTRY(idx, _)                              \
			{ (uint8_t)KT_ROW(DT_PROP_BY_IDX(ZMK_RGB_MT_NODE, map, idx)), \
			  (uint8_t)KT_COL(DT_PROP_BY_IDX(ZMK_RGB_MT_NODE, map, idx)) }
#		define RGB_MATRIX_POS_TO_RC_LEN ZMK_RGB_MT_LEN
#		define RGB_MATRIX_POS_TO_RC_MAP \
			{ LISTIFY(ZMK_RGB_MT_LEN, ZMK_RGB_POS_RC_ENTRY, (, ), 0) }
/* ZMK_RGB_POS_RC_ENTRY 是 LISTIFY 内部宏，用完即弃 */
#		undef ZMK_RGB_POS_RC_ENTRY
#	endif /* DT_HAS_CHOSEN(zmk_matrix_transform) */

/* 恢复 RC 原始定义（modifiers.h 的 RC(keycode)） */
#pragma pop_macro("RC")

#endif /* key-reactive / framebuffer 灯效启用 */
