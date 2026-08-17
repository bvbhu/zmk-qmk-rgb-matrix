# RGB Matrix 模块 snippet
#
# 此 snippet 在 ZMK 编译时：
# 1. 从 Kconfig 选项自动生成 rgb_matrix_generated_config.h
#    （包含 #define MATRIX_ROWS / MATRIX_COLS / RGB_MATRIX_LED_COUNT 等）
# 2. 注入键盘仓 keymap.c（LED 布局 g_led_config 与可选的 indicators 函数）
# 3. 将生成文件和键盘仓目录加入包含路径
#
# 将 ZMK_CONFIG 环境变量指向键盘仓 config/ 目录。
# Kconfig 值在 .conf 文件中设置，例如：
#   CONFIG_RGB_MATRIX_ROWS=5
#   CONFIG_RGB_MATRIX_COLS=15
#   CONFIG_RGB_MATRIX_LED_COUNT=69

# ===== 生成 rgb_matrix_generated_config.h =====
set(RGB_MATRIX_GEN_H "")

string(APPEND RGB_MATRIX_GEN_H
    "#pragma once\n"
    "\n"
    "#include <stdint.h>\n"
    "\n"
    "/* ===== Matrix dimensions ===== */\n"
    "#define MATRIX_ROWS ${CONFIG_RGB_MATRIX_ROWS}\n"
    "#define MATRIX_COLS ${CONFIG_RGB_MATRIX_COLS}\n"
    "#define RGB_MATRIX_LED_COUNT ${CONFIG_RGB_MATRIX_LED_COUNT}\n"
    "\n"
    "/* ===== Rendering parameters ===== */\n"
    "#define RGB_MATRIX_CENTER { ${CONFIG_RGB_MATRIX_CENTER_X}, ${CONFIG_RGB_MATRIX_CENTER_Y} }\n"
    "#define RGB_MATRIX_LED_FLUSH_LIMIT ${CONFIG_RGB_MATRIX_LED_FLUSH_LIMIT}\n"
)

if(${CONFIG_RGB_WORKQ_STACK_SIZE} GREATER 0)
    string(APPEND RGB_MATRIX_GEN_H
        "#define RGB_WORKQ_STACK_SIZE ${CONFIG_RGB_WORKQ_STACK_SIZE}\n"
    )
endif()

string(APPEND RGB_MATRIX_GEN_H
    "\n"
    "/* ===== Step values ===== */\n"
    "#define RGB_MATRIX_HUE_STEP ${CONFIG_RGB_MATRIX_HUE_STEP}\n"
    "#define RGB_MATRIX_SAT_STEP ${CONFIG_RGB_MATRIX_SAT_STEP}\n"
    "#define RGB_MATRIX_VAL_STEP ${CONFIG_RGB_MATRIX_VAL_STEP}\n"
    "#define RGB_MATRIX_SPD_STEP ${CONFIG_RGB_MATRIX_SPD_STEP}\n"
)

# DEFAULT_ON: Kconfig bool 在 CMake 中为 ON/OFF，C 代码需要 true/false
if(${CONFIG_RGB_MATRIX_DEFAULT_ON})
    set(RGB_MATRIX_DEFAULT_ON_VAL "true")
else()
    set(RGB_MATRIX_DEFAULT_ON_VAL "false")
endif()

string(APPEND RGB_MATRIX_GEN_H
    "\n"
    "/* ===== Limits and defaults ===== */\n"
    "#define RGB_MATRIX_MAXIMUM_BRIGHTNESS ${CONFIG_RGB_MATRIX_MAXIMUM_BRIGHTNESS}\n"
    "#define RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE ${CONFIG_RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE}\n"
    "#define RGB_MATRIX_DEFAULT_FLAGS LED_FLAG_ALL\n"
    "#define RGB_MATRIX_DEFAULT_HSV ${CONFIG_RGB_MATRIX_DEFAULT_HSV}\n"
    "#define RGB_MATRIX_DEFAULT_SPD ${CONFIG_RGB_MATRIX_DEFAULT_SPD}\n"
    "#define RGB_MATRIX_DEFAULT_ON ${RGB_MATRIX_DEFAULT_ON_VAL}\n"
    "#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_${CONFIG_RGB_MATRIX_DEFAULT_MODE}\n"
)

if(${CONFIG_RGB_MATRIX_KEEP_ON_WIRED})
    string(APPEND RGB_MATRIX_GEN_H
        "\n"
        "#define RGB_MATRIX_KEEP_ON_WIRED\n"
    )
endif()

if(${CONFIG_RGB_LED_CONFIG_CONST})
    string(APPEND RGB_MATRIX_GEN_H
        "\n"
        "#define RGB_LED_CONFIG_CONST\n"
    )
endif()

string(APPEND RGB_MATRIX_GEN_H
    "\n"
    "/* ===== Enabled effects ===== */\n"
)

# 为每个启用的灯效生成 #define
foreach(EFFECT ALPHAS_MODS GRADIENT_UP_DOWN GRADIENT_LEFT_RIGHT BREATHING \
    BAND_SAT BAND_VAL BAND_PINWHEEL_SAT BAND_PINWHEEL_VAL \
    BAND_SPIRAL_SAT BAND_SPIRAL_VAL CYCLE_ALL CYCLE_LEFT_RIGHT CYCLE_UP_DOWN \
    RAINBOW_MOVING_CHEVRON CYCLE_OUT_IN CYCLE_OUT_IN_DUAL CYCLE_PINWHEEL CYCLE_SPIRAL \
    DUAL_BEACON RAINBOW_BEACON RAINBOW_PINWHEELS FLOWER_BLOOMING RAINDROPS \
    JELLYBEAN_RAINDROPS HUE_BREATHING HUE_PENDULUM HUE_WAVE PIXEL_RAIN \
    PIXEL_FLOW PIXEL_FRACTAL STARLIGHT STARLIGHT_DUAL_SAT STARLIGHT_DUAL_HUE \
    STARLIGHT_SMOOTH RIVERFLOW SOLID_REACTIVE_SIMPLE SOLID_REACTIVE SOLID_REACTIVE_WIDE \
    SOLID_REACTIVE_MULTIWIDE SOLID_REACTIVE_CROSS SOLID_REACTIVE_MULTICROSS \
    SOLID_REACTIVE_NEXUS SOLID_REACTIVE_MULTINEXUS SPLASH MULTISPLASH SOLID_SPLASH \
    SOLID_MULTISPLASH TYPING_HEATMAP DIGITAL_RAIN)
    set(EFFECT_ON "CONFIG_RGB_MATRIX_EFFECT_${EFFECT}")
    if(${EFFECT_ON})
        string(APPEND RGB_MATRIX_GEN_H
            "#define ENABLE_RGB_MATRIX_${EFFECT}\n"
        )
    endif()
endforeach()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/rgb_matrix_generated_config.h"
    "${RGB_MATRIX_GEN_H}")

# 将生成文件所在目录加入包含路径（使用 zephyr_include_directories 宏）
zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR})

# ===== 注入键盘仓 keymap.c =====
# 支持两种命名：keymap.c 或 <keyboard>.keymap.c
file(GLOB KEYMAP_SOURCES
    "${ZMK_CONFIG}/keymap.c"
    "${ZMK_CONFIG}/*.keymap.c"
)
if(KEYMAP_SOURCES)
    zephyr_sources(${KEYMAP_SOURCES})
endif()

# 将键盘仓 config/ 加入包含路径（供 keymap.c 使用）
zephyr_include_directories(${ZMK_CONFIG})
