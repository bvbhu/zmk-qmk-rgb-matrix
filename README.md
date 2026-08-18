# zmk-qmk-rgb-matrix

QMK 风格的 RGB Matrix 灯效系统，移植自 QMK，适配 ZMK。支持全部 QMK RGB Matrix 灯效（含 Key Reactive 与 Framebuffer 灯效）。

## 核心特性

- **完整 QMK 灯效集**：普通灯效、Key Reactive（按键响应）、Framebuffer（打字热力图/数字雨）三类全覆盖
- **Kconfig 全配置**：所有参数走 Kconfig，编译期自动生成配置头文件，无需手改 C 代码
- **开箱即用的默认值**：持久化（SETTINGS/NVS/FLASH）、HID 指示灯、内置 underglow 关闭等子系统配置已由模块默认处理
- **`g_led_config` 兼容 QMK**：LED 布局可直接从 QMK 键盘配置复制
- **完整 `&rgb_ug` 键码**：开关/色相/饱和度/亮度/速度/灯效循环/直选灯效（RGB_EFS）/直接设色（RGB_COLOR_HSB），Shift 反向调节
- **设置持久化**：settings 子系统 + NVS，可整体裁剪
- **省内存设计**：`g_led_config` 可 const 化入 Flash，独立/系统 workqueue 可选

## 架构

```
zmk-qmk-rgb-matrix/
├── CMakeLists.txt               # 编译模块源码 + 生成 rgb_matrix_generated_config.h
├── Kconfig                      # 全部 RGB Matrix 参数的 Kconfig 定义
├── zephyr/module.yml            # 模块元数据
├── west.yml                     # west manifest
├── LICENSE                      # 双许可证（MIT + GPL-2.0-or-later）
│
└── src/rgb_matrix/
    ├── rgb_matrix.h             # 公共 API（键盘仓 keymap.c 的唯一 include 入口）
    ├── rgb_matrix.c             # 控制器核心（状态机、渲染、按键事件）
    ├── rgb_matrix_settings.c/.h # &rgb_ug 键码监听 + 持久化
    ├── rgb_matrix_types.h       # 类型定义（led_config_t 等）
    ├── qmk_compat.c/.h          # QMK API 兼容层
    ├── post_config.h            # Key Reactive / Framebuffer 宏 + position→(row,col) 映射
    └── lib8tion.c/.h            # FastLED 数学运算
    └── animations/              # 灯效实现
        ├── rgb_matrix_effects.inc
        ├── *_anim.h
        └── runners/
            ├── rgb_matrix_runners.inc
            └── effect_runner_*.h

snippets/
└── rgb_matrix/CMakeLists.txt    # 键盘仓 keymap.c 注入与包含路径
```

## 机制概览

| 机制 | 说明 |
|------|------|
| **Kconfig → C 宏** | 模块 CMake 读取 Kconfig 值，自动生成 `rgb_matrix_generated_config.h` 注入编译 |
| **LED 布局** | 键盘仓 `keymap.c` 提供 `g_led_config`（矩阵映射 + 物理坐标 + 标志位），每键盘独立定义 |
| **灯效选择** | `CONFIG_RGB_MATRIX_EFFECT_<NAME>=y` 启用，自动展开为 `#define ENABLE_RGB_MATRIX_<NAME>` |
| **键码控制** | 监听 ZMK `&rgb_ug` 行为键码，覆盖全部 15 个 RGB 命令，Shift 反向调节 |
| **持久化** | settings 子系统（NVS 后端）默认启用，`CONFIG_RGB_MATRIX_PERSISTENCE=n` 可整体裁剪 |
| **HID 指示灯** | `CONFIG_ZMK_HID_INDICATORS` 默认启用，未启用时 `host_keyboard_led_state()` 不编译 |
| **const 布局** | `CONFIG_RGB_LED_CONFIG_CONST=y` 时 `g_led_config` 声明为 const，节省 RAM |

## 快速接入（三步）

```yaml
# 1. config/west.yml 添加模块
projects:
  - name: zmk-qmk-rgb-matrix
    remote: mykeyboard
    revision: main
    path: modules/rgb-matrix
```

```conf
# 2. config/<keyboard>.conf — 仅需必填项
CONFIG_RGB_MATRIX_ROWS=5
CONFIG_RGB_MATRIX_COLS=15
CONFIG_RGB_MATRIX_LED_COUNT=69
CONFIG_RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT=y
```

```c
// 3. config/keymap.c 定义 LED 布局
#include <rgb_matrix.h>
led_config_t g_led_config = { /* 矩阵映射 / 物理坐标 / 标志位 */ };
```

详细步骤（设备树、持久化分区、分体键盘等）见 [适配指南](RGB_Matrix_适配指南.md)，全部参数见 [配置项表](RGB_Matrix_配置项表.md)。

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

## 许可证

| 许可证 | 覆盖内容 |
|--------|----------|
| MIT | 集成/工具代码：settings、qmk_compat、lib8tion |
| GPL-2.0-or-later | QMK 派生的 RGB Matrix 核心、灯效动画 |

GPL 文件链接进同一固件，**最终固件必须以 GPL-2.0-or-later 兼容条款分发**。

## 更多资料

- [适配指南](RGB_Matrix_适配指南.md) / [配置项表](RGB_Matrix_配置项表.md)
- [ZMK Module Creation](https://zmk.dev/docs/development/module-creation)
- [Zephyr Modules](https://docs.zephyrproject.org/3.5.0/develop/modules.html)
- [ZMK 使用模块](https://zmk.dev/docs/features/modules)
- [Zephyr west manifest](https://docs.zephyrproject.org/3.5.0/develop/west/manifest.html#west-manifests)
