# RGB Matrix 配置项表

> 全部配置在 `config/<keyboard>.conf` 中以 `CONFIG_<名称>=<值>` 形式设置。适配步骤见[适配指南](RGB_Matrix_适配指南.md)。

## 1. 总开关与子系统默认值

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `ZMK_RGB_MATRIX` | bool | `y` | 模块总开关 |
| `RGB_MATRIX_PERSISTENCE` | bool | `y` | RGB 设置持久化。关闭后持久化代码整体裁剪，每次开机用出厂默认值 |
| `SETTINGS` | bool | `y`（随 `RGB_MATRIX_PERSISTENCE`） | Zephyr settings 子系统，持久化载体 |
| `SETTINGS_NVS` | bool | `y`（随 `RGB_MATRIX_PERSISTENCE`） | settings 的 NVS 后端 |
| `NVS` | bool | `y`（随 `RGB_MATRIX_PERSISTENCE`） | 非易失存储 |
| `FLASH` | bool | `y`（随 `RGB_MATRIX_PERSISTENCE`） | Flash 驱动 |
| `FLASH_PAGE_LAYOUT` | bool | `y`（随 `RGB_MATRIX_PERSISTENCE`） | Flash 分区布局（NVS 依赖） |
| `FLASH_MAP` | bool | `y`（随 `RGB_MATRIX_PERSISTENCE`） | Flash 映射 API |
| `ZMK_HID_INDICATORS` | bool | `y` | HID 指示灯（Caps/Num/Scroll Lock 查询）。未启用时 `host_keyboard_led_state()` 不编译 |
| `ZMK_RGB_UNDERGLOW` | bool | `n` | ZMK 内置 underglow 由模块接管 `&rgb_ug`，默认关闭避免双重处理 |

> 以上子系统默认值均用 `default` 而非强制 `select` 实现：在键盘仓显式写 `CONFIG_SETTINGS=n` 等仍可关闭，此时模块相应功能（持久化）自动裁剪。

## 2. 矩阵与 LED（必填）

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `RGB_MATRIX_ROWS` | int | – | **必填**，矩阵行数 |
| `RGB_MATRIX_COLS` | int | – | **必填**，矩阵列数 |
| `RGB_MATRIX_LED_COUNT` | int | – | **必填**，灯珠总数 |

## 3. 渲染参数

| 配置项 | 类型 | 默认值 | 范围 | 说明 |
|--------|------|--------|------|------|
| `RGB_MATRIX_CENTER_X` | int | `108` | 0~255 | 对称/波浪类灯效中心 X |
| `RGB_MATRIX_CENTER_Y` | int | `32` | 0~255 | 对称/波浪类灯效中心 Y |
| `RGB_MATRIX_LED_FLUSH_LIMIT` | int | `16` | 1~100 | LED 刷新间隔（ms），即灯效帧率节流 |
| `RGB_WORKQ_STACK_SIZE` | int | `2048` | 0~4096 | 独立渲染 workqueue 栈（字节）；`0` = 复用系统 workqueue（省 RAM） |

> `CENTER` 参考计算：`x = 255 * 物理列号 / (总列数 - 1)`，`y = 255 * 物理行号 / (总行数 - 1)`。

## 4. 步进值（键控调节增量）

| 配置项 | 类型 | 默认值 | 范围 | 说明 |
|--------|------|--------|------|------|
| `RGB_MATRIX_HUE_STEP` | int | `8` | 1~255 | 色相步进 |
| `RGB_MATRIX_SAT_STEP` | int | `16` | 1~255 | 饱和度步进 |
| `RGB_MATRIX_VAL_STEP` | int | `16` | 1~255 | 亮度步进 |
| `RGB_MATRIX_SPD_STEP` | int | `16` | 1~255 | 速度步进 |

## 5. 限制与默认值

| 配置项 | 类型 | 默认值 | 范围 | 说明 |
|--------|------|--------|------|------|
| `RGB_MATRIX_MAXIMUM_BRIGHTNESS` | int | `225` | 1~255 | 最大亮度上限（调节时被 clamp） |
| `RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE` | int | 跟随 `ZMK_SETTINGS_SAVE_DEBOUNCE` | – | 持久化防抖（ms）：正 = 尾缘防抖；`0` = 立即写；负 = 不写 Flash |
| `RGB_MATRIX_DEFAULT_HUE` | int | `170` | 0~255 | 出厂默认色相 |
| `RGB_MATRIX_DEFAULT_SAT` | int | `255` | 0~255 | 出厂默认饱和度 |
| `RGB_MATRIX_DEFAULT_VAL` | int | `200` | 0~255 | 出厂默认亮度 |
| `RGB_MATRIX_DEFAULT_SPD` | int | `127` | 0~255 | 出厂默认速度 |
| `RGB_MATRIX_DEFAULT_ON` | bool | `y` | – | 开机默认开启 RGB |
| `RGB_MATRIX_DEFAULT_MODE` | string | `"CYCLE_LEFT_RIGHT"` | – | 出厂默认灯效名（不含 `RGB_MATRIX_` 前缀，须已启用） |
| `RGB_MATRIX_KEEP_ON_WIRED` | bool | `n` | – | 有线（USB）模式空闲不熄灯；无线仍遵循 `ZMK_IDLE_TIMEOUT` |
| `RGB_LED_CONFIG_CONST` | bool | `n` | – | `g_led_config` 声明为 const 存 Flash（省 RAM）；keymap.c 需同步 const |
| `RGB_MATRIX_IS_LEFT` | bool | `n` | – | 强制 `is_keyboard_left()` 为 true；默认遵循 ZMK split（central = 左半） |

## 6. 灯效开关

全部为 `bool`、默认 `n`，置 `y` 启用。`SOLID_COLOR`（单色常亮）始终内置，无开关。

### 6.1 普通灯效

| 配置项 | 灯效 |
|--------|------|
| `RGB_MATRIX_EFFECT_ALPHAS_MODS` | 主键区/修饰键双色（Alphas Mods） |
| `RGB_MATRIX_EFFECT_GRADIENT_UP_DOWN` | 上下渐变 |
| `RGB_MATRIX_EFFECT_GRADIENT_LEFT_RIGHT` | 左右渐变 |
| `RGB_MATRIX_EFFECT_BREATHING` | 呼吸 |
| `RGB_MATRIX_EFFECT_BAND_SAT` | 饱和度彩带 |
| `RGB_MATRIX_EFFECT_BAND_VAL` | 亮度彩带 |
| `RGB_MATRIX_EFFECT_BAND_PINWHEEL_SAT` | 饱和度风车彩带 |
| `RGB_MATRIX_EFFECT_BAND_PINWHEEL_VAL` | 亮度风车彩带 |
| `RGB_MATRIX_EFFECT_BAND_SPIRAL_SAT` | 饱和度螺旋彩带 |
| `RGB_MATRIX_EFFECT_BAND_SPIRAL_VAL` | 亮度螺旋彩带 |
| `RGB_MATRIX_EFFECT_CYCLE_ALL` | 全局色相循环 |
| `RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT` | 左右彩虹循环 |
| `RGB_MATRIX_EFFECT_CYCLE_UP_DOWN` | 上下彩虹循环 |
| `RGB_MATRIX_EFFECT_RAINBOW_MOVING_CHEVRON` | 移动人字形彩虹 |
| `RGB_MATRIX_EFFECT_CYCLE_OUT_IN` | 由外向内循环 |
| `RGB_MATRIX_EFFECT_CYCLE_OUT_IN_DUAL` | 双向由外向内循环 |
| `RGB_MATRIX_EFFECT_CYCLE_PINWHEEL` | 风车循环 |
| `RGB_MATRIX_EFFECT_CYCLE_SPIRAL` | 螺旋循环 |
| `RGB_MATRIX_EFFECT_DUAL_BEACON` | 双信标 |
| `RGB_MATRIX_EFFECT_RAINBOW_BEACON` | 彩虹信标 |
| `RGB_MATRIX_EFFECT_RAINBOW_PINWHEELS` | 彩虹风车 |
| `RGB_MATRIX_EFFECT_FLOWER_BLOOMING` | 花朵绽放 |
| `RGB_MATRIX_EFFECT_RAINDROPS` | 雨滴 |
| `RGB_MATRIX_EFFECT_JELLYBEAN_RAINDROPS` | 彩豆雨滴 |
| `RGB_MATRIX_EFFECT_HUE_BREATHING` | 色相呼吸 |
| `RGB_MATRIX_EFFECT_HUE_PENDULUM` | 色相摆动 |
| `RGB_MATRIX_EFFECT_HUE_WAVE` | 色相波 |
| `RGB_MATRIX_EFFECT_PIXEL_RAIN` | 像素雨 |
| `RGB_MATRIX_EFFECT_PIXEL_FLOW` | 像素流 |
| `RGB_MATRIX_EFFECT_PIXEL_FRACTAL` | 像素分形 |
| `RGB_MATRIX_EFFECT_STARLIGHT` | 星光 |
| `RGB_MATRIX_EFFECT_STARLIGHT_DUAL_SAT` | 双饱和度星光 |
| `RGB_MATRIX_EFFECT_STARLIGHT_DUAL_HUE` | 双色相星光 |
| `RGB_MATRIX_EFFECT_STARLIGHT_SMOOTH` | 平滑星光 |
| `RGB_MATRIX_EFFECT_RIVERFLOW` | 河流 |

### 6.2 Key Reactive 按键响应灯效（常驻 ~82 B BSS）

| 配置项 | 灯效 |
|--------|------|
| `RGB_MATRIX_EFFECT_SOLID_REACTIVE_SIMPLE` | 单色按键点亮（简单） |
| `RGB_MATRIX_EFFECT_SOLID_REACTIVE` | 单色按键点亮 |
| `RGB_MATRIX_EFFECT_SOLID_REACTIVE_WIDE` | 单色宽幅点亮 |
| `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTIWIDE` | 多键宽幅点亮 |
| `RGB_MATRIX_EFFECT_SOLID_REACTIVE_CROSS` | 十字点亮 |
| `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTICROSS` | 多键十字点亮 |
| `RGB_MATRIX_EFFECT_SOLID_REACTIVE_NEXUS` | Nexus 点亮 |
| `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTINEXUS` | 多键 Nexus 点亮 |
| `RGB_MATRIX_EFFECT_SPLASH` | 彩色溅射（最近一键） |
| `RGB_MATRIX_EFFECT_MULTISPLASH` | 彩色溅射（多键） |
| `RGB_MATRIX_EFFECT_SOLID_SPLASH` | 单色溅射（最近一键） |
| `RGB_MATRIX_EFFECT_SOLID_MULTISPLASH` | 单色溅射（多键） |

> Single/Multi 变体共用同一动画文件与数学函数，区别仅是击键记录遍历起点：Single 只响应最近一次击键，Multi 响应全部击键记录（上限 8 条）。
> 依赖设备树 `zmk,matrix_transform`，缺失时自动禁用。

### 6.3 Framebuffer 灯效（常驻 ~96 B BSS）

| 配置项 | 灯效 |
|--------|------|
| `RGB_MATRIX_EFFECT_TYPING_HEATMAP` | 打字热力图 |
| `RGB_MATRIX_EFFECT_DIGITAL_RAIN` | 数字雨 |

> 依赖设备树 `zmk,matrix_transform`，缺失时自动禁用。

## 7. `&rgb_ug` 键码一览

模块实现了 ZMK `dt-bindings/zmk/rgb.h` 的全部命令（param1），在 keymap 中通过 `&rgb_ug <命令>` 使用：

| 键码 | 作用 | Shift 反向 |
|------|------|-----------|
| `RGB_TOG` | 开关切换 | – |
| `RGB_ON` / `RGB_OFF` | 打开 / 关闭 | – |
| `RGB_HUI` / `RGB_HUD` | 色相 + / − | 支持 |
| `RGB_SAI` / `RGB_SAD` | 饱和度 + / − | 支持 |
| `RGB_BRI` / `RGB_BRD` | 亮度 + / −（受 `MAXIMUM_BRIGHTNESS` 限制） | 支持 |
| `RGB_SPI` / `RGB_SPD` | 速度 + / − | 支持 |
| `RGB_EFF` / `RGB_EFR` | 下一个 / 上一个灯效 | 支持 |
| `RGB_EFS_CMD <mode>` | 直选灯效：`mode` 为灯效枚举值，从 1 起按 `rgb_matrix_effects.inc` 中已启用灯效顺序编号（`SOLID_COLOR` = 1），无效值忽略 | – |
| `RGB_COLOR_HSB(h, s, v)` | 直接设置颜色：h/s/v 均为 0~255，v 超过 `MAXIMUM_BRIGHTNESS` 会被 clamp | – |

示例：

```dts
&rgb_ug RGB_EFS_CMD 3                       // 直选第 3 号灯效
&rgb_ug RGB_COLOR_HSB(170, 255, 200)        // 直接设置青蓝色
```

> `RGB_EFS_CMD` / `RGB_COLOR_HSB` 为较新 ZMK 引入的命令，需 ZMK 固件包含对应 `rgb.h` 定义。

## 8. LED 标志位（keymap.c 布局用）

| 宏 | 值 | 含义 |
|----|----|------|
| `LED_FLAG_MODIFIER` | `0x01` | 修饰键 |
| `LED_FLAG_UNDERGLOW` | `0x02` | 底灯 |
| `LED_FLAG_KEYLIGHT` | `0x04` | 键位灯 |
| `LED_FLAG_INDICATOR` | `0x08` | 指示灯 |
| `LED_FLAG_ALL` | `0xFF` | 全部 |
| `LED_FLAG_NONE` | `0x00` | 无 |
| `NO_LED` | `255` | 该矩阵位无 LED |
