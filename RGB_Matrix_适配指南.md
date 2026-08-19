# RGB Matrix 适配指南

> 面向 ZMK 键盘开发者，介绍如何把 `zmk-qmk-rgb-matrix` 模块接入一把新键盘。全部配置参数见[配置项表](RGB_Matrix_配置项表.md)。

## 前置条件

- ZMK 用户配置仓（`config/` 结构完整）
- 键盘矩阵行数、列数、LED 总数与物理坐标已知
- 主控有可用 Flash 分区（持久化需要，模块已默认启用 settings 栈）

## 适配步骤

### 第 1 步：west.yml 中添加模块

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
  self:
    path: config
```

> 模块自带 `west.yml` 的默认 remote 指向 `https://github.com/bvbhu`，按实际 fork 调整。

### 第 2 步：在 `config/<keyboard>.conf` 设置必要 Kconfig

模块已内置全部合理默认值（渲染参数、步进、默认灯色、持久化栈、HID 指示灯等），**只需填写键盘必填项**：

```conf
# ---- 必填：矩阵尺寸与 LED 数量 ----
CONFIG_RGB_MATRIX_ROWS=5
CONFIG_RGB_MATRIX_COLS=15
CONFIG_RGB_MATRIX_LED_COUNT=69

# ---- 必选：至少启用一个灯效（SOLID_COLOR 单色常亮始终内置，无需配置）----
CONFIG_RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT=y
CONFIG_RGB_MATRIX_EFFECT_BREATHING=y
```

其余配置（中心坐标、刷新率、默认色、RAM 优化等）均有默认值，需要调整时查阅[配置项表](RGB_Matrix_配置项表.md)。

### 第 3 步：在 `config/keymap.c` 定义 LED 布局

`g_led_config` 三部分对应 QMK 的矩阵映射、物理坐标、LED 标志，格式与 QMK 完全兼容：

```c
#include <rgb_matrix.h>

// clang-format off
led_config_t g_led_config =
{
    {
        /* [0] matrix_co[ROWS][COLS]：矩阵位 → LED 索引，无 LED 填 NO_LED */
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 },
        /* ... 其余行 ... */
    },
    {
        /* [1] point[LED_COUNT]：每颗 LED 物理坐标 {x, y}，范围 0~255 ,注意按照灯珠编号顺序填写而不是行列顺序*/
        { 0, 64 }, { 16, 64 }, { 32, 64 }, /* ... */
    },
    {
        /* [2] flags[LED_COUNT]：LED_FLAG_KEYLIGHT / UNDERGLOW / MODIFIER / INDICATOR ，也是灯珠编号顺序*/
        LED_FLAG_KEYLIGHT, LED_FLAG_KEYLIGHT, /* ... */
    }
};
// clang-format on

/* 可选：指示灯（CapsLock 等），需 CONFIG_ZMK_HID_INDICATORS（默认已启用） */
bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max)
{
    if(host_keyboard_led_state().caps_lock)
        RGB_MATRIX_INDICATOR_SET_COLOR(25, 185, 0, 0);
    return true;
}
```

注意：

- `matrix_co` 行列数与 `RGB_MATRIX_ROWS`/`COLS` 一致；`point`/`flags` 长度等于 `RGB_MATRIX_LED_COUNT`。
- 若启用 `CONFIG_RGB_LED_CONFIG_CONST=y`，声明处同步改为 `const led_config_t g_led_config = {...}`。

### 第 4 步：设备树（.dts/.overlay）

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
- 灯带节点用 ZMK 常规 `led_strip` 配置（如 `worldsemi,ws2812-spi` / `ws2812-gpio`），需保证驱动 `update_rgb` 不修改传入的 pixels 缓冲区（`raindrops`、`starlight` 等灯效依赖缓冲区跨帧保留）。

### 第 5 步（可选）：LED 驱动与其余 Kconfig

持久化栈（SETTINGS/NVS/FLASH）、HID 指示灯、关闭 ZMK 内置 underglow **均已由模块默认启用**，无需手动配置。仅按硬件选配 LED 驱动等：

```conf
# 例：WS2812 驱动（视硬件与 Zephyr 版本按需选择）
CONFIG_WS2812_STRIP=y

# 例：RAM 紧张 MCU（如 STM32F072）
# CONFIG_RGB_LED_CONFIG_CONST=y
# CONFIG_RGB_WORKQ_STACK_SIZE=0
```

### 第 6 步：在 keymap 中绑定键码

直接使用 ZMK 内置 `&rgb_ug` 行为，无需额外定义：

```dts
// 开关 / 循环灯效 / 调节
&rgb_ug RGB_TOG      &rgb_ug RGB_EFF      &rgb_ug RGB_HUI
&rgb_ug RGB_BRI      &rgb_ug RGB_SPI

// 切换指定灯效（mode 为灯效枚举值，从 1 开始按 rgb_matrix_effects.inc 中已启用灯效顺序编号）
&rgb_ug RGB_EFS_CMD 3

// 直接设置颜色（h/s/v 均为 0-255）
&rgb_ug RGB_COLOR_HSB(170, 255, 200)
```

按住 Shift 操作为反向调节（如 `Shift + RGB_HUI` = 色相减少）。全部键码见[配置项表](RGB_Matrix_配置项表.md)的键码一节。

### 第 7 步（可选）：分体键盘

在 `config/<keyboard>.conf` 中同时配置左右半灯珠数量即自动启用分裂键盘逻辑：

```conf
CONFIG_RGB_MATRIX_SPLIT_LED_COUNT_LEFT=36
CONFIG_RGB_MATRIX_SPLIT_LED_COUNT_RIGHT=36
```


- 左右数量必须**同时设置或同时不设置**，只设一侧会编译报错；同时设置会自动计算`RGB_MATRIX_LED_COUNT`。
- `is_keyboard_left()` 默认遵循 ZMK split 惯例（central = 左半）；`CONFIG_RGB_MATRIX_IS_LEFT=y` 可强制为 true。
- Key Reactive 灯效仅在 master（central）端记录击键。

### 第 8 步：构建验证

`west build` 后确认日志出现 `RGB matrix controller initialized`；用 `RGB_TOG` 测试开关、`RGB_EFF` 循环灯效。切换无反应时检查对应 `RGB_MATRIX_EFFECT_*` 是否已置 `y`、`RGB_MATRIX_DEFAULT_MODE` 是否指向已启用的灯效名。

## 工作原理

### Kconfig → 生成 C 宏

模块 `CMakeLists.txt` 读取 Kconfig 值生成 `rgb_matrix_generated_config.h` 注入编译。`CONFIG_RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT=y` 展开为 `#define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT`，驱动灯效注册与枚举。字符串形式的默认灯效名由生成的 `rgb_matrix_mode_select.h` 在运行时 `strcmp` 匹配。

### 灯效框架

- `rgb_matrix_effects.inc` 按顺序 include 各灯效文件，**该顺序决定枚举编号**（`RGB_EFF` 循环顺序、`RGB_EFS` 直选索引）。
- 灯效文件以 `#ifdef ENABLE_RGB_MATRIX_*` 包裹，未启用的不注册、不占枚举位。
- 渲染统一由 `runners/effect_runner_*.h` 驱动（按索引、按坐标差、按键响应、溅射等模板）。
- `SOLID_COLOR` 始终内置。
- Single/Multi 变体共用同一动画文件与数学函数，区别仅在 `effect_runner_reactive_splash(start, ...)` 的遍历起点：Single 传 `count-1`（只响应最近一次击键），Multi 传 `0`（响应全部击键记录，上限 `LED_HITS_TO_REMEMBER=8`）。

### 渲染状态机

`rgb_matrix_task()` 四阶段循环，由 `RGB_MATRIX_LED_FLUSH_LIMIT`（默认 16 ms）周期的内核定时器驱动：

```
SYNCING → STARTING → RENDERING → FLUSHING → SYNCING ...
```

渲染任务提交到独立 workqueue（`RGB_WORKQ_STACK_SIZE > 0`）或系统 workqueue（=0，省 RAM）。

### 键码与持久化

`rgb_matrix_behavior.c` 以 ZMK 行为驱动的方式接管 `&rgb_ug`：与 ZMK 内置 underglow 驱动共用同一设备树节点（`DT_DRV_COMPAT`），仅在 `CONFIG_ZMK_RGB_UNDERGLOW=n`（模块默认）时编译，若键盘仓显式设 `y` 会在编译期报错。`binding_pressed` 直接处理全部 15 个 RGB 命令并返回 OPAQUE 消费键码。每次调节经 `RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE` 防抖后写入 Flash（settings 键 `rgb_matrix/state`，8 字节打包）。

持久化默认启用；`CONFIG_RGB_MATRIX_PERSISTENCE=n` 或显式 `CONFIG_SETTINGS=n` 时，持久化代码整体裁剪，每次开机用出厂默认值。

### 按键事件 → 位置映射

Key Reactive / 打字热力图需要 (row, col)。模块在编译期从设备树 chosen `zmk_matrix_transform` 生成 `position → (row, col)` 映射表；缺失该节点时相关灯效自动禁用。

## 常见问题（FAQ）

**Q1：灯效开关了但现象不变？**
确认 `RGB_MATRIX_DEFAULT_MODE` 指向的灯效名已启用（名称需与 Kconfig 后缀完全一致）。

**Q2：按键响应 / 热力图灯效不生效？**
设备树 `chosen` 需声明 `zmk,matrix_transform`；缺失时相关灯效被 `post_config.h` 自动禁用。

**Q3：RAM 不足 / 刷入后无法识别？**
STM32F072（16 KB SRAM）等紧张 MCU 上：启用 `CONFIG_RGB_LED_CONFIG_CONST=y`；减少启用灯效；`CONFIG_RGB_WORKQ_STACK_SIZE=0` 复用系统 workqueue。Key Reactive 灯效常驻 ~82 B、Framebuffer 灯效常驻 ~96 B BSS。

**Q3b：单帧渲染耗时过长 / CPU 占用高（低端 MCU）？**
把 `CONFIG_RGB_MATRIX_LED_PROCESS_LIMIT` 调小（默认 `(LED_COUNT+4)/5`，QMK 同款算法；必须 `> 0`，建议 `16`/`24`/`32`）：把一帧的 LED 渲染拆成多帧完成，显著降低单帧 CPU 峰值，代价是完整画面刷新率变为 `1000 / (ceil(LED_COUNT / LIMIT) × FLUSH_LIMIT) ms`。例如 69 颗 LED + LIMIT=16 + FLUSH_LIMIT=16 → 约 12.5 完整 FPS。也可配合调大 `RGB_MATRIX_LED_FLUSH_LIMIT` 进一步节流。

**Q4：有线/无线熄灯行为？**
`CONFIG_ZMK_IDLE_TIMEOUT` 控制空闲熄灯；`CONFIG_RGB_MATRIX_KEEP_ON_WIRED=y` 时仅 USB 模式空闲不熄灯，无线仍超时熄灯。

**Q5：`host_keyboard_led_state()` 编译报错未定义？**
该函数需 `CONFIG_ZMK_HID_INDICATORS`（模块已设默认 y）。若显式关闭了该配置，函数不编译；重新启用即可。

**Q6：`RGB_EFS` 直选的编号怎么确定？**
编号 = 已启用灯效按 `src/rgb_matrix/animations/rgb_matrix_effects.inc` 的 include 顺序从 1 编号（`SOLID_COLOR` 固定为 1）。也可在生成的 `rgb_matrix_mode_select.h` 或 `rgb_matrix.h` 的枚举中确认。
