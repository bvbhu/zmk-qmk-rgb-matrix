# zmk-qmk-rgb-matrix 使用文档

> 面向 ZMK 键盘开发者，涵盖完整适配流程、全部配置项、键码等。

---

## 目录

1. [模块概述](#1-模块概述)
2. [适配步骤](#2-适配步骤)
3. [灯效编号表](#3-灯效编号表)
4. [工作原理](#4-工作原理)
5. [常见问题（FAQ）](#5-常见问题faq)

---

## 1. 模块概述

`zmk-qmk-rgb-matrix` 是一个 ZMK 模块，将 QMK 的 RGB Matrix 灯效系统移植到 ZMK 中。它复用 ZMK 的 `&rgb_ug` 行为接口，兼容 QMK 的 `led_config_t` 布局格式，提供 QMK RGB Matrix 的所有灯效、Flash 持久化、独立渲染 workqueue 以及分体键盘支持。

**核心特性：**

- **支持已有的所有QMK灯效**：单色常亮、渐变、彩虹循环、呼吸、彩带、风车、螺旋、雨滴、星光、河流、按键响应、溅射、热力图等
- **QMK 兼容布局**：`g_led_config` 格式与 QMK 完全一致，可直接迁移
- **设置自动保存**：调整灯效后，自动通过 ZMK Settings/NVS 保存到 Flash，重启不丢失（可关闭）
- **独立渲染线程**：低优先级 workqueue，不阻塞按键扫描
- **分帧渲染**：单帧处理 LED 数量可配置，适配低端 MCU
- **分体键盘支持**：该功能暂未测试（没有合适的键盘）

---

## 2. 适配步骤

### 第 0 步：前置条件

- ZMK 用户配置仓库（`config/` 结构完整）
- 键盘矩阵行数、列数、LED 总数与物理坐标已知
- 主控有可用 Flash 分区（持久化需要，模块已默认启用 settings 栈）

### 第 1 步：`config/west.yml` 中添加模块

```yaml
manifest:
  remotes:
    - name: <your-modulename> # 模块 remote 名称，可自定义
      url-base: https://github.com/bvbhu # 默认仓库为 bvbhu，如有 fork 替换为 <your-username>
  projects:
    - name: zmk-qmk-rgb-matrix
      remote: <your-modulename>
      revision: main
      path: modules/rgb-matrix
  self:
    path: config
```

> `<your-username>` 默认为 `bvbhu`（模块原始仓库），如有自己的 fork 则替换为你的 GitHub 用户名。`<your-modulename>` 为 remote 名称，可自定义，但需与 `projects.remote` 保持一致。

### 第 2 步：在 `config/<keyboard>.conf` 设置 Kconfig

模块大部分配置项有默认值，下方给出**完整可复制的配置模板**，包含所有可配置项，直接添加到现有配置中，按注释按需启用/修改：

```conf
# zmk-qmk-rgb-matrix 完整配置模板

# ---- [必填] 矩阵尺寸与 LED 数量 ----
CONFIG_RGB_MATRIX_ROWS=6           # 矩阵行数
CONFIG_RGB_MATRIX_COLS=16          # 矩阵列数
CONFIG_RGB_MATRIX_LED_COUNT=96     # 灯珠总数（分体键盘见下方 split 配置）

# ---- [按需] 分体键盘 ----
# 同时设置左右半灯珠数，自动启用 split 逻辑，自动计算 RGB_MATRIX_LED_COUNT
# RGB_MATRIX_SPLIT_LED_COUNT_LEFT=48    # 左半（central）灯珠数量
# RGB_MATRIX_SPLIT_LED_COUNT_RIGHT=48   # 右半（peripheral）灯珠数量
# CONFIG_RGB_MATRIX_IS_LEFT=y           # 指定键盘是否为左半，未配置默认central为左半

# ---- [必填] LED 驱动（视硬件与 Zephyr 版本按需选择）----
CONFIG_WS2812_STRIP=y               # WS2812 驱动（示例，按实际硬件调整）

# ---- [必选] 设置默认灯效，启用灯效 ----

# 请保证启用了DEFAULT_MODE对应的灯效
# CONFIG_RGB_MATRIX_DEFAULT_MODE="CYCLE_LEFT_RIGHT" # 默认值

# 普通灯效
CONFIG_RGB_MATRIX_EFFECT_ALPHAS_MODS=y
CONFIG_RGB_MATRIX_EFFECT_GRADIENT_UP_DOWN=y
CONFIG_RGB_MATRIX_EFFECT_GRADIENT_LEFT_RIGHT=y
CONFIG_RGB_MATRIX_EFFECT_BREATHING=y
CONFIG_RGB_MATRIX_EFFECT_BAND_SAT=y
CONFIG_RGB_MATRIX_EFFECT_BAND_VAL=y
CONFIG_RGB_MATRIX_EFFECT_BAND_PINWHEEL_SAT=y
CONFIG_RGB_MATRIX_EFFECT_BAND_PINWHEEL_VAL=y
CONFIG_RGB_MATRIX_EFFECT_BAND_SPIRAL_SAT=y
CONFIG_RGB_MATRIX_EFFECT_BAND_SPIRAL_VAL=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_ALL=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT=y # 默认灯效的默认值
CONFIG_RGB_MATRIX_EFFECT_CYCLE_UP_DOWN=y
CONFIG_RGB_MATRIX_EFFECT_RAINBOW_MOVING_CHEVRON=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_OUT_IN=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_OUT_IN_DUAL=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_PINWHEEL=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_SPIRAL=y
CONFIG_RGB_MATRIX_EFFECT_DUAL_BEACON=y
CONFIG_RGB_MATRIX_EFFECT_RAINBOW_BEACON=y
CONFIG_RGB_MATRIX_EFFECT_RAINBOW_PINWHEELS=y
CONFIG_RGB_MATRIX_EFFECT_FLOWER_BLOOMING=y
CONFIG_RGB_MATRIX_EFFECT_RAINDROPS=y
CONFIG_RGB_MATRIX_EFFECT_JELLYBEAN_RAINDROPS=y
CONFIG_RGB_MATRIX_EFFECT_HUE_BREATHING=y
CONFIG_RGB_MATRIX_EFFECT_HUE_PENDULUM=y
CONFIG_RGB_MATRIX_EFFECT_HUE_WAVE=y
CONFIG_RGB_MATRIX_EFFECT_PIXEL_RAIN=y
CONFIG_RGB_MATRIX_EFFECT_PIXEL_FLOW=yUSAGE.md
CONFIG_RGB_MATRIX_EFFECT_PIXEL_FRACTAL=y
CONFIG_RGB_MATRIX_EFFECT_STARLIGHT=y
CONFIG_RGB_MATRIX_EFFECT_STARLIGHT_DUAL_SAT=y
CONFIG_RGB_MATRIX_EFFECT_STARLIGHT_DUAL_HUE=y
CONFIG_RGB_MATRIX_EFFECT_STARLIGHT_SMOOTH=y
CONFIG_RGB_MATRIX_EFFECT_RIVERFLOW=y

# Key Reactive 灯效
CONFIG_RGB_MATRIX_EFFECT_SOLID_REACTIVE_SIMPLE=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_REACTIVE=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_REACTIVE_WIDE=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTIWIDE=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_REACTIVE_CROSS=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTICROSS=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_REACTIVE_NEXUS=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTINEXUS=y
CONFIG_RGB_MATRIX_EFFECT_SPLASH=y
CONFIG_RGB_MATRIX_EFFECT_MULTISPLASH=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_SPLASH=y
CONFIG_RGB_MATRIX_EFFECT_SOLID_MULTISPLASH=y

# Framebuffer 灯效
CONFIG_RGB_MATRIX_EFFECT_TYPING_HEATMAP=y
CONFIG_RGB_MATRIX_EFFECT_DIGITAL_RAIN=y

# ---- [可选] 渲染参数 ----
# CONFIG_RGB_MATRIX_CENTER_X=112          # 灯效显示区域中心横坐标（0~255）
# CONFIG_RGB_MATRIX_CENTER_Y=32           # 灯效显示区域中心纵坐标（0~255）
# CONFIG_RGB_MATRIX_LED_FLUSH_LIMIT=16    # LED 刷新间隔 ms（1~100，控制帧率）
# CONFIG_RGB_MATRIX_LED_PROCESS_LIMIT=96  # 每帧最多处理 LED 数（1~255，分帧渲染）

# ---- [可选] 步进值（键控调节增量）----
# CONFIG_RGB_MATRIX_HUE_STEP=8   # 色相步进（1~255）
# CONFIG_RGB_MATRIX_SAT_STEP=16  # 饱和度步进（1~255）
# CONFIG_RGB_MATRIX_VAL_STEP=16  # 亮度步进（1~255）
# CONFIG_RGB_MATRIX_SPD_STEP=16  # 速度步进（1~255）

# ---- [可选] 限制与默认值 ----
# CONFIG_RGB_MATRIX_MAXIMUM_BRIGHTNESS=225      # 最大亮度上限（1~255）
# CONFIG_RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE=5000 # 保存延迟 ms（负值=不写入 Flash）
# CONFIG_RGB_MATRIX_DEFAULT_HUE=170             # 默认色相（0~255）
# CONFIG_RGB_MATRIX_DEFAULT_SAT=255             # 默认饱和度（0~255）
# CONFIG_RGB_MATRIX_DEFAULT_VAL=200             # 默认亮度（0~255）
# CONFIG_RGB_MATRIX_DEFAULT_SPD=127             # 默认速度（0~255）
# CONFIG_RGB_MATRIX_DEFAULT_ON=y                # 开机默认开启 RGB
# CONFIG_RGB_MATRIX_KEEP_ON_WIRED=n            # 有线模式空闲不熄灯（无线仍超时熄灯）

# ---- [可选] 持久化（默认启用）----
# CONFIG_RGB_MATRIX_PERSISTENCE=n    # 关闭后每次开机恢复默认值
# 关闭持久化后可一并裁剪 settings/NVS/FLASH 子系统以节省空间
# CONFIG_SETTINGS=n

# ---- [可选] 其他 ----
# CONFIG_RGB_LED_CONFIG_CONST=y      # 将keymap.c中的l_led_config定义改为const（可减少RAM使用）
# CONFIG_RGB_WORKQ_STACK_SIZE=0      # 复用系统 workqueue（如选用非0值建议640-1024）（可减少RAM使用）  
# CONFIG_ZMK_RGB_MATRIX = y          # 模块总开关，不开用模块干嘛
# CONFIG_ZMK_HID_INDICATORS=y        # HID 指示灯（caps/num/scrlk），默认启用，使用host_keyboard_led_state必须启用
# CONFIG_ZMK_RGB_UNDERGLOW=n         # 使用本模块必须关闭其他灯光控制器
```

### 第 3 步：在 `config/<keyboard>.keymap.c` 定义 LED 布局

#### 3.1 `g_led_config` 结构

`g_led_config` 三部分对应 QMK 的矩阵映射、物理坐标、LED 标志，格式与 QMK 完全兼容：

```c
#include <rgb_matrix.h>

led_config_t g_led_config =
{
    {
        /* [0] matrix_co[RGB_MATRIX_ROWS][RGB_MATRIX_COLS]：矩阵位置 → 灯珠编号，无 LED 填 NO_LED */
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 },
        /* ... 其余行 ... */
    },
    {
        /* [1] point[LED_COUNT]：每颗 LED 物理坐标 {x, y}，范围 0~255 ,按照灯珠编号顺序填写而不是行列顺序*/
        { 0, 64 }, { 16, 64 }, { 32, 64 }, /* ... */
    },
    {
        /* [2] flags[LED_COUNT]：建议统一使用键位灯4，也按灯珠编号顺序*/
        4, 4, 4, 4, 4, 4, /* ... */
    }
}
```

> `matrix_co` 行列数与 `RGB_MATRIX_ROWS`/`COLS` 一致；`point`/`flags` 长度等于 `RGB_MATRIX_LED_COUNT`。
> 若启用 `CONFIG_RGB_LED_CONFIG_CONST=y`，声明处同步改为 `const led_config_t g_led_config = { ... };`

flags可用值如下，可以通过`|`进行组合，如 `LED_FLAG_KEYLIGHT|LED_FLAG_MODIFIER`同时代表修饰键和键位灯。
但是，仅有`LED_FLAG_MODIFIER`在ALPHAS_MODS灯效下有意义，其余值没有被使用。

| 宏                   | 值     | 含义           |
| -------------------- | ------ | -------------- |
| `LED_FLAG_MODIFIER`  | `0x01` | 修饰键         |
| `LED_FLAG_UNDERGLOW` | `0x02` | 底灯           |
| `LED_FLAG_KEYLIGHT`  | `0x04` | 键位灯         |
| `LED_FLAG_INDICATOR` | `0x08` | 指示灯         |
| `LED_FLAG_ALL`       | `0xFF` | 全部           |
| `LED_FLAG_NONE`      | `0x00` | 无             |

#### 3.2 矩阵坐标计算

`point[]` 数组中的 `{x, y}` 坐标范围为 0~255，表示 LED 在键盘面上的相对位置。坐标计算器[QMK RGB Matrix Calculator](https://myst729.github.io/qmk-rgb-matrix/)。
计算器给出的值是标准的(112, 32)为中心，也是模块默认值，无需再配置中心横纵坐标 `RGB_MATRIX_CENTER_X` / `RGB_MATRIX_CENTER_Y` 。

> 坐标不需要精确到像素级（数据类型是uint8_t），大致反映 LED 的相对位置即可。坐标误差主要影响渐变、径向类灯效的视效果，对按键响应、溅射等灯效无影响。

#### 3.3 指示灯设置：`rgb_matrix_indicators_advanced_user`

`rgb_matrix_indicators_advanced_user` 用于自定义 LED ，可将灯珠绑定为系统指示灯（CapsLock、NumLock、ScrollLock 等）等，需 `CONFIG_ZMK_HID_INDICATORS`（默认已启用）。

**参考代码：**

```c
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max)
{
    // 建议定义使用的灯珠在所有情况下的行为，避免在部分灯效下指示灯残留，这是QMK原有问题。
    // 目前仅实现了下面出现的函数。
    led_t led_state = host_keyboard_led_state();
    uint8_t mods = get_mods();
    uint8_t layer = get_highest_layer(layer_state);

    if(led_state.caps_lock) //caps_lock 开启
        RGB_MATRIX_INDICATOR_SET_COLOR(32, 0xC8, 0x00, 0x00);
    else //caps_lock 关闭
        RGB_MATRIX_INDICATOR_SET_COLOR(32, 0x00, 0x00, 0x00);
    if(!led_state.num_lock) //num_lock关闭，行为与常规num指示灯相反
        RGB_MATRIX_INDICATOR_SET_COLOR(33, 0xC8, 0x00, 0x00);
    else //num_lock开启
        RGB_MATRIX_INDICATOR_SET_COLOR(33, 0x00, 0x00, 0x00);
    if(led_state.scroll_lock) //scroll_lock 开启
        RGB_MATRIX_INDICATOR_SET_COLOR(34, 0xC8, 0x00, 0x00);
    else //scroll_lock 关闭
        RGB_MATRIX_INDICATOR_SET_COLOR(34, 0x00, 0x00, 0x00);

    //SHIFT、CTRL、GUI、ALT
    if(mods & MOD_MASK_SHIFT)
        RGB_MATRIX_INDICATOR_SET_COLOR(16, 0xC8, 0x00, 0x00);
    if(mods & MOD_MASK_CTRL)
        RGB_MATRIX_INDICATOR_SET_COLOR(0, 0xC8, 0x00, 0x00);
    if(mods & MOD_MASK_GUI)
        RGB_MATRIX_INDICATOR_SET_COLOR(2, 0xC8, 0x00, 0x00);
    if(mods & MOD_MASK_ALT)
        RGB_MATRIX_INDICATOR_SET_COLOR(3, 0xC8, 0x00, 0x00);

    // 当前层指示
    static const int layer_leds[] = { -1, 22, 23, 24 };
    if(layer < sizeof(layer_leds) / sizeof(layer_leds[0]) && layer_leds[layer] >= 0)
        RGB_MATRIX_INDICATOR_SET_COLOR(layer_leds[layer], 0x00, 0xC8, 0x00);

    return true;
}

```

### 第 4 步：设备树（`.dts` / `.overlay`）

在 `/chosen` 声明节点，并预留 Flash 分区：

```dts
/ {
    chosen {
        zephyr,settings-partition = &storage_partition;  /* 持久化（默认启用）*/
        zmk,kscan = &kscan0;
        zmk,matrix_transform = &default_transform;       /* Key Reactive / Framebuffer 灯效必需 */
        zmk,underglow = &led_strip;                      /* RGB Matrix 从该节点获取 LED 设备，必需 */
    };
};

&flash0 {
    partitions {
        compatible = "fixed-partitions";
        #address-cells = <1>;
        #size-cells = <1>;
        storage_partition: partition@1e800 {
            label = "storage";
            reg = <0x0001e800 DT_SIZE_K(6)>;
        };
    };
};
```

- 分区地址/大小按芯片容量调整，不得覆盖固件区域。
- 灯带节点用 ZMK 常规 `led_strip` 配置（如 `worldsemi,ws2812-spi` / `ws2812-gpio`）。

  > **注意事项**：`led_strip` 驱动的 `update_rgb` 实现**不得修改传入的 `pixels` 缓冲区**。`raindrops`、`starlight` 等灯效依赖该缓冲区跨帧保留状态；若驱动内部修改了该缓冲区，会导致这些灯效闪烁或异常。使用非标准 `led_strip` 驱动时请检查。

### 第 5 步：在 keymap 中绑定键码

可以使用键位编辑器[Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)或手动修改`config/<keyboard>.keymap`。
使用 ZMK 内置 `&rgb_ug` 行为，功能一致，无需额外定义，按住 Shift 操作为反向调节：

| 键码                     | 作用                                                             | Shift 反向 |
| ------------------------ | ---------------------------------------------------------------- | ---------- |
| `RGB_TOG`                | 开关切换                                                         | –          |
| `RGB_ON` / `RGB_OFF`     | 打开 / 关闭                                                      | –          |
| `RGB_HUI` / `RGB_HUD`    | 色相 + / −                                                       | 支持       |
| `RGB_SAI` / `RGB_SAD`    | 饱和度 + / −                                                     | 支持       |
| `RGB_BRI` / `RGB_BRD`    | 亮度 + / −（受 `MAXIMUM_BRIGHTNESS` 限制）                       | 支持       |
| `RGB_SPI` / `RGB_SPD`    | 速度 + / −                                                       | 支持       |
| `RGB_EFF` / `RGB_EFR`    | 下一个 / 上一个灯效                                              | 支持       |
| `RGB_EFS_CMD <mode>`     | 切换到指定灯效                                                   | –          |
| `RGB_COLOR_HSB(h, s, v)` | 直接设置颜色：h/s/v 均为 0~255，亮度受 `MAXIMUM_BRIGHTNESS` 限制 | –          |

> 通过条件编译兼容了未引入`RGB_EFS_CMD` / `RGB_COLOR_HSB` 的ZMK版本。

```dts
// 开关 / 循环灯效 / 调节
&rgb_ug RGB_TOG      &rgb_ug RGB_EFF      &rgb_ug RGB_HUI
&rgb_ug RGB_BRI      &rgb_ug RGB_SPI

// 切换到指定灯效（mode 为灯效枚举值，从 1 开始按已启用灯效顺序编号）
&rgb_ug RGB_EFS_CMD 3

// 直接设置颜色（h/s/v 均为 0~255）
&rgb_ug RGB_COLOR_HSB(170, 255, 200)
```

### 第 6 步：构建、测试

测试各灯效是否显示正常、按键切换是否正常、是否可以正确保存，部分灯效较为相似，可能会误认为没有切换。

---

## 3. 灯效编号表

全部为 `bool`、默认 `n`，置 `y` 启用。

枚举值用于 `&rgb_ug RGB_EFS_CMD <mode>` 键码切换到指定灯效，`<mode>` 即为枚举值。表中数值为启用所有灯效时的完整编号；**实际编号取决于 Kconfig 中启用的灯效集合及其排列顺序**——若跳过某个灯效，后续所有编号会相应前移一位。建议在启用灯效集合变化后重新对照本表，后续可能实现映射转换。

| 值  | 配置项                                        | 分类         | 灯效(机翻仅供参考)                               |
| --- | --------------------------------------------- | ------------ | --------------------------------------------- |
| 1   | –                                             | 普通         | `RGB_MATRIX_SOLID_COLOR` 单色常亮（始终内置） |
| 2   | `RGB_MATRIX_EFFECT_ALPHAS_MODS`               | 普通         | 主键区/修饰键双色                             |
| 3   | `RGB_MATRIX_EFFECT_GRADIENT_UP_DOWN`          | 普通         | 上下渐变                                      |
| 4   | `RGB_MATRIX_EFFECT_GRADIENT_LEFT_RIGHT`       | 普通         | 左右渐变                                      |
| 5   | `RGB_MATRIX_EFFECT_BREATHING`                 | 普通         | 呼吸                                          |
| 6   | `RGB_MATRIX_EFFECT_BAND_SAT`                  | 普通         | 饱和度彩带                                    |
| 7   | `RGB_MATRIX_EFFECT_BAND_VAL`                  | 普通         | 亮度彩带                                      |
| 8   | `RGB_MATRIX_EFFECT_BAND_PINWHEEL_SAT`         | 普通         | 饱和度风车彩带                                |
| 9   | `RGB_MATRIX_EFFECT_BAND_PINWHEEL_VAL`         | 普通         | 亮度风车彩带                                  |
| 10  | `RGB_MATRIX_EFFECT_BAND_SPIRAL_SAT`           | 普通         | 饱和度螺旋彩带                                |
| 11  | `RGB_MATRIX_EFFECT_BAND_SPIRAL_VAL`           | 普通         | 亮度螺旋彩带                                  |
| 12  | `RGB_MATRIX_EFFECT_CYCLE_ALL`                 | 普通         | 全局色相循环                                  |
| 13  | `RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT`          | 普通         | 左右彩虹循环                                  |
| 14  | `RGB_MATRIX_EFFECT_CYCLE_UP_DOWN`             | 普通         | 上下彩虹循环                                  |
| 15  | `RGB_MATRIX_EFFECT_RAINBOW_MOVING_CHEVRON`    | 普通         | 移动人字形彩虹                                |
| 16  | `RGB_MATRIX_EFFECT_CYCLE_OUT_IN`              | 普通         | 由外向内循环                                  |
| 17  | `RGB_MATRIX_EFFECT_CYCLE_OUT_IN_DUAL`         | 普通         | 双向由外向内循环                              |
| 18  | `RGB_MATRIX_EFFECT_CYCLE_PINWHEEL`            | 普通         | 风车循环                                      |
| 19  | `RGB_MATRIX_EFFECT_CYCLE_SPIRAL`              | 普通         | 螺旋循环                                      |
| 20  | `RGB_MATRIX_EFFECT_DUAL_BEACON`               | 普通         | 双信标                                        |
| 21  | `RGB_MATRIX_EFFECT_RAINBOW_BEACON`            | 普通         | 彩虹信标                                      |
| 22  | `RGB_MATRIX_EFFECT_RAINBOW_PINWHEELS`         | 普通         | 彩虹风车                                      |
| 23  | `RGB_MATRIX_EFFECT_FLOWER_BLOOMING`           | 普通         | 花朵绽放                                      |
| 24  | `RGB_MATRIX_EFFECT_RAINDROPS`                 | 普通         | 雨滴                                          |
| 25  | `RGB_MATRIX_EFFECT_JELLYBEAN_RAINDROPS`       | 普通         | 彩豆雨滴                                      |
| 26  | `RGB_MATRIX_EFFECT_HUE_BREATHING`             | 普通         | 色相呼吸                                      |
| 27  | `RGB_MATRIX_EFFECT_HUE_PENDULUM`              | 普通         | 色相摆动                                      |
| 28  | `RGB_MATRIX_EFFECT_HUE_WAVE`                  | 普通         | 色相波                                        |
| 29  | `RGB_MATRIX_EFFECT_PIXEL_RAIN`                | 普通         | 像素雨                                        |
| 30  | `RGB_MATRIX_EFFECT_PIXEL_FLOW`                | 普通         | 像素流                                        |
| 31  | `RGB_MATRIX_EFFECT_PIXEL_FRACTAL`             | 普通         | 像素分形                                      |
| 32  | `RGB_MATRIX_EFFECT_TYPING_HEATMAP`            | Framebuffer  | 打字热力图                                    |
| 33  | `RGB_MATRIX_EFFECT_DIGITAL_RAIN`              | Framebuffer  | 数字雨                                        |
| 34  | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_SIMPLE`     | Key Reactive | 单色按键点亮（简单）                          |
| 35  | `RGB_MATRIX_EFFECT_SOLID_REACTIVE`            | Key Reactive | 单色按键点亮                                  |
| 36  | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_WIDE`       | Key Reactive | 单色宽幅点亮                                  |
| 37  | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTIWIDE`  | Key Reactive | 多键宽幅点亮                                  |
| 38  | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_CROSS`      | Key Reactive | 十字点亮                                      |
| 39  | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTICROSS` | Key Reactive | 多键十字点亮                                  |
| 40  | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_NEXUS`      | Key Reactive | Nexus 点亮                                    |
| 41  | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTINEXUS` | Key Reactive | 多键 Nexus 点亮                               |
| 42  | `RGB_MATRIX_EFFECT_SPLASH`                    | Key Reactive | 彩色溅射（最近一键）                          |
| 43  | `RGB_MATRIX_EFFECT_MULTISPLASH`               | Key Reactive | 彩色溅射（多键）                              |
| 44  | `RGB_MATRIX_EFFECT_SOLID_SPLASH`              | Key Reactive | 单色溅射（最近一键）                          |
| 45  | `RGB_MATRIX_EFFECT_SOLID_MULTISPLASH`         | Key Reactive | 单色溅射（多键）                              |
| 46  | `RGB_MATRIX_EFFECT_STARLIGHT_SMOOTH`          | 普通         | 平滑星光                                      |
| 47  | `RGB_MATRIX_EFFECT_STARLIGHT`                 | 普通         | 星光                                          |
| 48  | `RGB_MATRIX_EFFECT_STARLIGHT_DUAL_SAT`        | 普通         | 双饱和度星光                                  |
| 49  | `RGB_MATRIX_EFFECT_STARLIGHT_DUAL_HUE`        | 普通         | 双色相星光                                    |
| 50  | `RGB_MATRIX_EFFECT_RIVERFLOW`                 | 普通         | 河流                                          |

> **Key Reactive 灯效** Single 变体只处理最近一次击键，Multi 变体处理更多（上限 8 条）击键记录。
>
> Key Reactive 和 Framebuffer 灯效都依赖设备树 `zmk,matrix_transform`，缺失时自动禁用。

---

## 4. 工作原理

### 4.1 Kconfig → 生成 C 宏

模块 `CMakeLists.txt` 读取 Kconfig 值生成 `rgb_matrix_generated_config.h` 注入编译。`CONFIG_RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT=y` 展开为 `#define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT`，驱动灯效注册与枚举。字符串形式的默认灯效名由生成的 `rgb_matrix_mode_select.h` 在运行时 `strcmp` 匹配。

### 4.2 灯效框架

- `rgb_matrix_effects.inc` 按顺序 include 各灯效文件，**该顺序决定枚举编号**（`RGB_EFF` 循环顺序、`RGB_EFS` 直选索引）。
- 灯效文件以 `#ifdef ENABLE_RGB_MATRIX_*` 包裹，未启用的不注册、不占枚举位。
- 渲染统一由 `runners/effect_runner_*.h` 驱动（按索引、按坐标差、按键响应、溅射等模板）。
- `SOLID_COLOR` 始终内置。
- Single/Multi 变体共用同一动画文件与数学函数，区别仅在 `effect_runner_reactive_splash(start, ...)` 的遍历起点：Single 传 `count-1`（只响应最近一次击键），Multi 传 `0`（响应全部击键记录，上限 `LED_HITS_TO_REMEMBER=8`）。

### 4.3 渲染状态机

`rgb_matrix_task()` 四阶段循环，由 `RGB_MATRIX_LED_FLUSH_LIMIT`（默认 16 ms）周期的内核定时器驱动：

```
SYNCING → STARTING → RENDERING → FLUSHING → SYNCING ...
```

渲染任务提交到独立 workqueue（`RGB_WORKQ_STACK_SIZE > 0`）或系统 workqueue（`=0`，省 RAM）。

### 4.4 键码与持久化

`rgb_matrix_behavior.c` 以 ZMK 行为驱动的方式接管 `&rgb_ug`：与 ZMK 内置 underglow 驱动共用同一设备树节点（`DT_DRV_COMPAT`），故要求 `CONFIG_ZMK_RGB_UNDERGLOW=n`，若设为 `y` 会在编译期报错。`binding_pressed` 直接处理全部 15 个 RGB 命令并返回 OPAQUE 消费键码。每次调节经 `RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE` 防抖后**通过 ZMK Settings/NVS 写入 Flash**（settings 键 `rgb_matrix/state`，8 字节打包）。

持久化默认启用；`CONFIG_RGB_MATRIX_PERSISTENCE=n` 或显式 `CONFIG_SETTINGS=n` 时，Settings/NVS 持久化代码整体裁剪，每次开机用出厂默认值。

### 4.5 按键事件 → 位置映射

Key Reactive / 打字热力图需要 (row, col)。模块在编译期从设备树 chosen `zmk,matrix_transform` 生成 `position → (row, col)` 映射表；缺失该节点时相关灯效自动禁用。

---

## 5. 常见问题（FAQ）

### Q1：灯效开关了但现象不变？

确认 `RGB_MATRIX_DEFAULT_MODE` 指向的灯效名已启用（名称需与 Kconfig 后缀完全一致）。

### Q2：按键响应 / 热力图灯效不生效？

设备树 `chosen` 需声明 `zmk,matrix_transform`；缺失时相关灯效被 `post_config.h` 自动禁用。

### Q3：RAM 不足 / 刷入后无法识别？

启用 `CONFIG_RGB_LED_CONFIG_CONST=y`；`CONFIG_RGB_WORKQ_STACK_SIZE=0` 复用系统 workqueue。减少启用灯效，Key Reactive 灯效需要约82B、Framebuffer 灯效常驻需要约96B。

### Q4：单帧渲染耗时过长 / CPU 占用高（低端 MCU）？

把 `CONFIG_RGB_MATRIX_LED_PROCESS_LIMIT` 调小，QMK 默认值为灯珠数的五分之一（向上取整），必须 `> 0`，建议 `16`/`24`/`32`）：把一帧的 LED 渲染拆成多帧完成，显著降低单帧 CPU 峰值，代价是完整画面刷新率变为 `1000 / (ceil(LED_COUNT / LIMIT) × FLUSH_LIMIT) ms`。例如 69 颗 LED + LIMIT=16 + FLUSH_LIMIT=16 → 约 12.5 完整 FPS。也可配合调大 `RGB_MATRIX_LED_FLUSH_LIMIT` 进一步节流。

### Q5：有线/无线熄灯行为？

`CONFIG_ZMK_IDLE_TIMEOUT` 控制空闲熄灯；`CONFIG_RGB_MATRIX_KEEP_ON_WIRED=y` 时仅 USB 模式空闲不熄灯，无线仍超时熄灯。

### Q6：`host_keyboard_led_state()` 编译报错未定义？

该函数需 `CONFIG_ZMK_HID_INDICATORS`（模块已设默认 y）。若关闭了该配置，函数不编译；重新启用即可。

### Q7：`RGB_EFS` 的编号怎么确定？

编号 = 已启用灯效按 `src/rgb_matrix/animations/rgb_matrix_effects.inc` 的 include 顺序从 1 编号（`SOLID_COLOR` 固定为 1），后续可能会优化。
