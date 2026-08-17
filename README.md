# zmk-qmk-rgb-matrix

QMK 风格的 RGB Matrix 灯效系统，移植自 QMK，适配 ZMK。支持全部 QMK RGB Matrix 灯效（含 Key Reactive 与 Framebuffer 灯效）。

## 架构

```
zmk-qmk-rgb-matrix/
├── CMakeLists.txt               # 编译模块源码
├── Kconfig                      # 所有 RGB Matrix 参数的 Kconfig 定义
├── zephyr/module.yml            # 模块元数据
├── west.yml                     # west manifest
├── LICENSE                      # 双许可证说明（MIT + GPL-2.0-or-later）
│
└── src/rgb_matrix/
    ├── rgb_matrix.h             # 公共 API（键盘仓 keymap.c 的唯一 include 入口）
    ├── rgb_matrix.h             # 公共 API + 控制器核心声明
    ├── rgb_matrix.c             # 控制器核心
    ├── rgb_matrix_settings.c/.h # &rgb_ug 监听 + 持久化
    ├── rgb_matrix_types.h       # 类型定义（led_config_t 等）
    ├── qmk_compat.c/.h          # QMK API 兼容层
    ├── post_config.h            # Key Reactive / Framebuffer 宏
    └── lib8tion.c/.h            # FastLED 数学运算
    └── animations/              # 编译型源码
        ├── rgb_matrix_effects.inc
        ├── *_anim.h
        └── runners/
            ├── rgb_matrix_runners.inc
            └── effect_runner_*.h

snippets/
└── rgb_matrix/CMakeLists.txt    # 生成 rgb_matrix_generated_config.h + 注入键盘仓 keymap.c
```

**说明：**

- `rgb_matrix.h` — 公共 API + 控制器核心声明。键盘仓 `keymap.c` 通过 `#include <rgb_matrix.h>` 获取所有类型和函数声明。
- `src/rgb_matrix/` 和 `animations/` 下的文件是模块的**实际编译代码**，由 `CMakeLists.txt` 收集。
- `snippets/rgb_matrix/CMakeLists.txt` 在编译时自动生成 `rgb_matrix_generated_config.h` 并注入键盘仓的 `keymap.c`。

---

## 键盘仓接入

### 1. west.yml 中添加模块

```yaml
manifest:
  remotes:
    - name: mykeyboard
      url-base: https://github.com/<your-username>
  projects:
    - name: zmk-qmk-rgb-matrix
      remote: mykeyboard
      revision: main
      path: modules/rgb-matrix
```

### 2. 在 `config/<keyboard>.conf` 中设置 Kconfig 参数

```conf
# 矩阵尺寸与 LED 数量
CONFIG_RGB_MATRIX_ROWS=5
CONFIG_RGB_MATRIX_COLS=15
CONFIG_RGB_MATRIX_LED_COUNT=69

# 渲染参数
CONFIG_RGB_MATRIX_CENTER_X=108
CONFIG_RGB_MATRIX_CENTER_Y=32
CONFIG_RGB_MATRIX_LED_FLUSH_LIMIT=16
CONFIG_RGB_WORKQ_STACK_SIZE=2048

# 步进值
CONFIG_RGB_MATRIX_HUE_STEP=8
CONFIG_RGB_MATRIX_SAT_STEP=16
CONFIG_RGB_MATRIX_VAL_STEP=16
CONFIG_RGB_MATRIX_SPD_STEP=16

# 限制与默认值
CONFIG_RGB_MATRIX_MAXIMUM_BRIGHTNESS=225
CONFIG_RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE=5000
CONFIG_RGB_MATRIX_DEFAULT_HUE=170
CONFIG_RGB_MATRIX_DEFAULT_SAT=255
CONFIG_RGB_MATRIX_DEFAULT_VAL=200
CONFIG_RGB_MATRIX_DEFAULT_SPD=127
CONFIG_RGB_MATRIX_DEFAULT_ON=y
CONFIG_RGB_MATRIX_DEFAULT_MODE="CYCLE_LEFT_RIGHT"
CONFIG_RGB_MATRIX_KEEP_ON_WIRED=n

# 若需要 const LED 布局（如 STM32F072 等 RAM 紧张的 MCU）：
# CONFIG_RGB_LED_CONFIG_CONST=y

# 启用灯效
CONFIG_RGB_MATRIX_EFFECT_ALPHAS_MODS=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT=y
# ... 其他灯效逐一列出
```

完整可用选项见 `Kconfig`。

### 3. 在 `config/keymap.c` 中定义 LED 布局

```c
#include <rgb_matrix.h>

// clang-format off
led_config_t g_led_config =
{
    {
        /* row × col 矩阵，值为 LED 索引，无 LED 的按键填 NO_LED */
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
        /* ... 其余行 ... */
    },
    {
        /* 每个 LED 的物理坐标 (x, y)，范围 0-255 */
        { 0.0, 64.0 }, { 14.9, 64.0 }, /* ... */
    },
    {
        /* 每个 LED 的标志位，参考 QMK LED_FLAG_* 定义 */
        4, 4, 4, /* ... 全部填 KEYLIGHT(4) ... */
    }
};
// clang-format on

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max)
{
    if(host_keyboard_led_state().caps_lock)
        RGB_MATRIX_INDICATOR_SET_COLOR(25, 185, 0, 0);
    return false;
}
```

若启用了 `CONFIG_RGB_LED_CONFIG_CONST=y`，则 `g_led_config` 也需声明为 `const led_config_t`。

### 4. 启用必要驱动

```conf
CONFIG_ZMK_RGB_UNDERGLOW=n
CONFIG_WS2812_STRIP=y
CONFIG_SETTINGS=y
CONFIG_NVS=y
CONFIG_FLASH=y
CONFIG_ZMK_HID_INDICATORS=y
```

---

## 机制说明

| 机制 | 说明 |
|------|------|
| **Kconfig → C 宏** | snippet 读取 Kconfig 值，自动生成 `rgb_matrix_generated_config.h` 注入编译，替换传统的 C 头文件配置方式 |
| **LED 布局** | 通过键盘仓 `keymap.c` 提供 `g_led_config`，每个键盘独立定义 |
| **灯效选择** | 通过 `CONFIG_RGB_MATRIX_EFFECT_<NAME>=y` 启用，snippet 自动展开为 `#define ENABLE_RGB_MATRIX_<NAME>` |
| **const 布局** | `CONFIG_RGB_LED_CONFIG_CONST=y` 时 `g_led_config` 声明为 const，节省 RAM（如 STM32F072） |

## 两把键盘示例

| 参数 | silicon65（nRF52840） | TL96F072v2（STM32F072） |
|------|----------------------|------------------------|
| ROWS / COLS | 5 × 15 | 6 × 16 |
| LED_COUNT | 69 | 96 |
| CENTER | (108, 32) | (112, 32) |
| FLUSH_LIMIT | 16 ms | 33 ms |
| WORKQ_STACK | 2048 | 640 |
| LED_CONFIG_CONST | 否 | 是 |
| Key Reactive / Framebuffer | 未启用 | 启用 |

---

## 更多资料

- [ZMK Module Creation](https://zmk.dev/docs/development/module-creation)
- [Zephyr Modules](https://docs.zephyrproject.org/3.5.0/develop/modules.html)
- [ZMK 使用模块](https://zmk.dev/docs/features/modules)
- [Zephyr west manifest](https://docs.zephyrproject.org/3.5.0/develop/west/manifest.html#west-manifests)
