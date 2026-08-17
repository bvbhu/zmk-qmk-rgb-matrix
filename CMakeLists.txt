# RGB Matrix 模块 CMakeLists.txt
#
# 将 src/rgb_matrix/controller/ 下的 .c 源文件加入编译。
# 模板文件 keymap.c（LED 布局示例）不包含在内，由各键盘仓通过
# snippet 注入各自的 keymap.c。
#
# 使用 zephyr_include_directories() / zephyr_sources() 宏，
# 这些宏将配置传播到 app 目标，不依赖 ZEPHYR_CURRENT_TARGET 是否为 INTERFACE。

zephyr_include_directories(src/rgb_matrix)
zephyr_include_directories(src/rgb_matrix/controller)
zephyr_include_directories(src/rgb_matrix/animations)
zephyr_include_directories(src/rgb_matrix/animations/runners)

zephyr_sources(
    src/rgb_matrix/controller/rgb_matrix.c
    src/rgb_matrix/controller/rgb_matrix_settings.c
    src/rgb_matrix/controller/qmk_compat.c
    src/rgb_matrix/controller/lib8tion.c
)

# ================================================================
# 生成 rgb_matrix_generated_config.h（从 Kconfig）
# 直接在模块 CMakeLists 中生成，确保始终产出，不依赖 snippet 激活。
# ================================================================
set(RGB_MATRIX_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${RGB_MATRIX_GEN_DIR})

set(RGB_MATRIX_GEN_H
    "#pragma once\n"
    "\n"
    "/* 由 rgb_matrix 模块 CMakeLists 从 Kconfig 自动生成，请勿手改 */\n"
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

# DEFAULT_ON: Kconfig bool → C true/false
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
    set(EFFECT_ON "CONFIG_RGB_MATRIX_EFFECT_${EFFECT}")
    if(${EFFECT_ON})
        string(APPEND RGB_MATRIX_GEN_H
            "#define ENABLE_RGB_MATRIX_${EFFECT}\n"
        )
    endif()
endforeach()

file(WRITE "${RGB_MATRIX_GEN_DIR}/rgb_matrix_generated_config.h"
    "${RGB_MATRIX_GEN_H}")

zephyr_include_directories(${RGB_MATRIX_GEN_DIR})

message(STATUS "rgb_matrix: generated config at ${RGB_MATRIX_GEN_DIR}/rgb_matrix_generated_config.h")
