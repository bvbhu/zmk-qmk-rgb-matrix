# Copyright (c) 2026 bvbhu
# SPDX-License-Identifier: GPL-2.0-or-later
#
# RGB Matrix 模块 CMakeLists.txt
#
# 模块源码、配置头文件生成、keymap.c 注入全部只在"当前构建确实是使用本模块的
# 键盘"时启用：即 ZMK_CONFIG 下存在匹配当前 BOARD 或 SHIELD 的 <名>.keymap.c。
# 这样 settings_reset 等诊断 shield 构建（无 keymap.c 匹配）不会编译模块源码，
# 也不会把别的键盘的 keymap.c 注入进去。
#
# 使用 zephyr_include_directories() / zephyr_sources() 宏，
# 这些宏将配置传播到 app 目标，不依赖 ZEPHYR_CURRENT_TARGET 是否为 INTERFACE。

# ================================================================
# 判定当前构建是否启用模块：查找匹配 BOARD / SHIELD 的 keymap.c
#   - silicon65: config/boards/shields/silicon65/silicon65.keymap.c (SHIELD=silicon65)
#   - TL96:      config/tl96.keymap.c                               (BOARD=tl96)
#   - settings_reset: 无匹配 → 模块整体惰性
# ================================================================
set(RGB_MATRIX_ACTIVE FALSE)
set(RGB_MATRIX_KEYMAP_SOURCES "")
foreach(_kb ${BOARD} ${SHIELD})
    file(GLOB_RECURSE _rgbm "${ZMK_CONFIG}/*${_kb}.keymap.c")
    if(_rgbm)
        set(RGB_MATRIX_ACTIVE TRUE)
        list(APPEND RGB_MATRIX_KEYMAP_SOURCES ${_rgbm})
    endif()
endforeach()

message(STATUS "rgb_matrix: active=${RGB_MATRIX_ACTIVE} board=${BOARD} shield=${SHIELD}")

if(RGB_MATRIX_ACTIVE)

zephyr_include_directories(src/rgb_matrix)
zephyr_include_directories(src/rgb_matrix/animations)
zephyr_include_directories(src/rgb_matrix/animations/runners)

# 确保 <zmk/...> 头文件可被 qmk_compat.h 等引用
zephyr_include_directories(${CMAKE_SOURCE_DIR}/include)

# keymap.c（定义 g_led_config 与 indicators）编译进 zephyr 目标。
# 通过 zephyr_sources() 保证在 shield/board 两种构建下都可靠执行。
zephyr_include_directories(${ZMK_CONFIG})
zephyr_sources(${RGB_MATRIX_KEYMAP_SOURCES})
message(STATUS "rgb_matrix: injected keymap.c sources: ${RGB_MATRIX_KEYMAP_SOURCES}")

zephyr_sources(
    src/rgb_matrix/rgb_matrix.c
    src/rgb_matrix/rgb_matrix_behavior.c
    src/rgb_matrix/qmk_compat.c
    src/rgb_matrix/utils.c
)

# ================================================================
# 生成 rgb_matrix_generated_config.h（从 Kconfig）
# 直接在模块 CMakeLists 中生成，确保始终产出，不依赖 snippet 激活。
#
# 注意：set() / string(APPEND) 每次只传一个字符串参数，
# 避免 CMake 将多参数视为列表（列表分隔符 ';' 会泄露到输出）。
# ================================================================
set(RGB_MATRIX_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${RGB_MATRIX_GEN_DIR})

set(RGB_MATRIX_GEN_H "")
string(APPEND RGB_MATRIX_GEN_H "#pragma once\n")
string(APPEND RGB_MATRIX_GEN_H "\n")
string(APPEND RGB_MATRIX_GEN_H "/* 由 rgb_matrix 模块 CMakeLists 从 Kconfig 自动生成，请勿手改 */\n")
string(APPEND RGB_MATRIX_GEN_H "\n")
string(APPEND RGB_MATRIX_GEN_H "/* ===== Matrix dimensions ===== */\n")
string(APPEND RGB_MATRIX_GEN_H "/* MATRIX_ROWS/COLS/LED_COUNT 从 autoconf.h 直接读取，不在 CMake 阶段生成 */\n")
string(APPEND RGB_MATRIX_GEN_H "#define MATRIX_ROWS CONFIG_RGB_MATRIX_ROWS\n")
string(APPEND RGB_MATRIX_GEN_H "#define MATRIX_COLS CONFIG_RGB_MATRIX_COLS\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_LED_COUNT CONFIG_RGB_MATRIX_LED_COUNT\n")
string(APPEND RGB_MATRIX_GEN_H "\n")
string(APPEND RGB_MATRIX_GEN_H "/* Sentinel check: ROWS/COLS/LED_COUNT default to -1; keyboards must override */\n")
string(APPEND RGB_MATRIX_GEN_H "#if MATRIX_ROWS <= 0 || MATRIX_COLS <= 0 || RGB_MATRIX_LED_COUNT <= 0\n")
string(APPEND RGB_MATRIX_GEN_H "#error \"RGB_MATRIX_ROWS / RGB_MATRIX_COLS / RGB_MATRIX_LED_COUNT must be set in keyboard .conf\"\n")
string(APPEND RGB_MATRIX_GEN_H "#endif\n")
string(APPEND RGB_MATRIX_GEN_H "/* ===== Rendering parameters ===== */\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_CENTER { CONFIG_RGB_MATRIX_CENTER_X, CONFIG_RGB_MATRIX_CENTER_Y }\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_LED_FLUSH_LIMIT CONFIG_RGB_MATRIX_LED_FLUSH_LIMIT\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_LED_PROCESS_LIMIT CONFIG_RGB_MATRIX_LED_PROCESS_LIMIT\n")

# RGB_WORKQ_STACK_SIZE 从 autoconf.h 直接读取。
# 若用户设 CONFIG_RGB_WORKQ_STACK_SIZE=0，编译时 RGB_WORKQ_STACK_SIZE > 0 为假，系统 workqueue 生效。
string(APPEND RGB_MATRIX_GEN_H "#define RGB_WORKQ_STACK_SIZE CONFIG_RGB_WORKQ_STACK_SIZE\n")

string(APPEND RGB_MATRIX_GEN_H "\n")
string(APPEND RGB_MATRIX_GEN_H "/* ===== Step values ===== */\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_HUE_STEP CONFIG_RGB_MATRIX_HUE_STEP\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_SAT_STEP CONFIG_RGB_MATRIX_SAT_STEP\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_VAL_STEP CONFIG_RGB_MATRIX_VAL_STEP\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_SPD_STEP CONFIG_RGB_MATRIX_SPD_STEP\n")

string(APPEND RGB_MATRIX_GEN_H "\n")
string(APPEND RGB_MATRIX_GEN_H "/* ===== Limits and defaults ===== */\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_MAXIMUM_BRIGHTNESS CONFIG_RGB_MATRIX_MAXIMUM_BRIGHTNESS\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE CONFIG_RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_DEFAULT_FLAGS LED_FLAG_ALL\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_DEFAULT_HUE CONFIG_RGB_MATRIX_DEFAULT_HUE\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_DEFAULT_SAT CONFIG_RGB_MATRIX_DEFAULT_SAT\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_DEFAULT_VAL CONFIG_RGB_MATRIX_DEFAULT_VAL\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_DEFAULT_SPD CONFIG_RGB_MATRIX_DEFAULT_SPD\n")
string(APPEND RGB_MATRIX_GEN_H "#ifdef CONFIG_RGB_MATRIX_DEFAULT_ON\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_DEFAULT_ON CONFIG_RGB_MATRIX_DEFAULT_ON\n")
string(APPEND RGB_MATRIX_GEN_H "#else\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_DEFAULT_ON 0\n")
string(APPEND RGB_MATRIX_GEN_H "#endif\n")

string(APPEND RGB_MATRIX_GEN_H "#ifdef CONFIG_RGB_MATRIX_KEEP_ON_WIRED\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_KEEP_ON_WIRED\n")
string(APPEND RGB_MATRIX_GEN_H "#endif\n")

string(APPEND RGB_MATRIX_GEN_H "#ifdef CONFIG_RGB_LED_CONFIG_CONST\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_LED_CONFIG_CONST\n")
string(APPEND RGB_MATRIX_GEN_H "#endif\n")

string(APPEND RGB_MATRIX_GEN_H "#ifdef CONFIG_RGB_MATRIX_IS_LEFT\n")
string(APPEND RGB_MATRIX_GEN_H "#define RGB_MATRIX_IS_LEFT true\n")
string(APPEND RGB_MATRIX_GEN_H "#endif\n")

string(APPEND RGB_MATRIX_GEN_H "\n")
string(APPEND RGB_MATRIX_GEN_H "/* ===== Enabled effects ===== */\n")

set(RGB_MATRIX_EFFECTS
    ALPHAS_MODS GRADIENT_UP_DOWN GRADIENT_LEFT_RIGHT BREATHING
    BAND_SAT BAND_VAL BAND_PINWHEEL_SAT BAND_PINWHEEL_VAL
    BAND_SPIRAL_SAT BAND_SPIRAL_VAL CYCLE_ALL CYCLE_LEFT_RIGHT CYCLE_UP_DOWN
    RAINBOW_MOVING_CHEVRON CYCLE_OUT_IN CYCLE_OUT_IN_DUAL CYCLE_PINWHEEL CYCLE_SPIRAL
    DUAL_BEACON RAINBOW_BEACON RAINBOW_PINWHEELS FLOWER_BLOOMING RAINDROPS
    JELLYBEAN_RAINDROPS HUE_BREATHING HUE_PENDULUM HUE_WAVE PIXEL_RAIN
    PIXEL_FLOW PIXEL_FRACTAL STARLIGHT STARLIGHT_DUAL_SAT STARLIGHT_DUAL_HUE
    STARLIGHT_SMOOTH RIVERFLOW SOLID_REACTIVE_SIMPLE SOLID_REACTIVE SOLID_REACTIVE_WIDE
    SOLID_REACTIVE_MULTIWIDE SOLID_REACTIVE_CROSS SOLID_REACTIVE_MULTICROSS
    SOLID_REACTIVE_NEXUS SOLID_REACTIVE_MULTINEXUS SPLASH MULTISPLASH SOLID_SPLASH
    SOLID_MULTISPLASH TYPING_HEATMAP DIGITAL_RAIN
)

foreach(EFFECT ${RGB_MATRIX_EFFECTS})
    string(APPEND RGB_MATRIX_GEN_H "#ifdef CONFIG_RGB_MATRIX_EFFECT_${EFFECT}\n")
    string(APPEND RGB_MATRIX_GEN_H "#define ENABLE_RGB_MATRIX_${EFFECT}\n")
    string(APPEND RGB_MATRIX_GEN_H "#endif\n")
endforeach()

file(WRITE "${RGB_MATRIX_GEN_DIR}/rgb_matrix_generated_config.h"
    "${RGB_MATRIX_GEN_H}")

# ================================================================
# 生成 rgb_matrix_mode_select.h
# autoconf.h 中字符串 Kconfig 值带引号（"CYCLE_LEFT_RIGHT"），无法用 ##
# 拼成枚举标识符，改为运行时 strcmp 匹配已启用灯效。
# 该头文件由 rgb_matrix.c 在 rgb_matrix.h（enum 定义）之后 include。
# ================================================================
set(RGB_MATRIX_MODE_SELECT_H "")
string(APPEND RGB_MATRIX_MODE_SELECT_H "#pragma once\n")
string(APPEND RGB_MATRIX_MODE_SELECT_H "\n")
string(APPEND RGB_MATRIX_MODE_SELECT_H "static inline uint8_t rgb_matrix_default_mode_from_name(void)\n")
string(APPEND RGB_MATRIX_MODE_SELECT_H "{\n")
string(APPEND RGB_MATRIX_MODE_SELECT_H "#ifdef CONFIG_RGB_MATRIX_DEFAULT_MODE\n")
string(APPEND RGB_MATRIX_MODE_SELECT_H "    if (strcmp(CONFIG_RGB_MATRIX_DEFAULT_MODE, \"SOLID_COLOR\") == 0) return (uint8_t)RGB_MATRIX_SOLID_COLOR;\n")
foreach(EFFECT ${RGB_MATRIX_EFFECTS})
    string(APPEND RGB_MATRIX_MODE_SELECT_H "#ifdef CONFIG_RGB_MATRIX_EFFECT_${EFFECT}\n")
    string(APPEND RGB_MATRIX_MODE_SELECT_H "    if (strcmp(CONFIG_RGB_MATRIX_DEFAULT_MODE, \"${EFFECT}\") == 0) return (uint8_t)RGB_MATRIX_${EFFECT};\n")
    string(APPEND RGB_MATRIX_MODE_SELECT_H "#endif\n")
endforeach()
string(APPEND RGB_MATRIX_MODE_SELECT_H "#endif\n")
string(APPEND RGB_MATRIX_MODE_SELECT_H "    return (uint8_t)RGB_MATRIX_NONE;\n")
string(APPEND RGB_MATRIX_MODE_SELECT_H "}\n")
file(WRITE "${RGB_MATRIX_GEN_DIR}/rgb_matrix_mode_select.h"
    "${RGB_MATRIX_MODE_SELECT_H}")

zephyr_include_directories(${RGB_MATRIX_GEN_DIR})

message(STATUS "rgb_matrix: generated config at ${RGB_MATRIX_GEN_DIR}/rgb_matrix_generated_config.h")

endif() # RGB_MATRIX_ACTIVE