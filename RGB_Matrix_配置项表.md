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
| `ZMK_HID_INDICATORS` | bool | `y` | HID 指示灯（Caps/Num/Scroll Lock 查询）。未启用时不可使用`host_keyboard_led_state()`  |
| `ZMK_RGB_UNDERGLOW` | bool | `n` | ZMK 内置 underglow 由模块接管 `&rgb_ug`，默认关闭避免双重处理 |

> 以上子系统默认值均用 `default` 而非强制 `select` 实现：在键盘仓显式写 `CONFIG_SETTINGS=n` 等仍可关闭，此时模块相应功能（持久化）自动裁剪。

## 2. 矩阵与 LED（必填，未配置会触发 `#error`）

| 配置项 | 类型 |  说明 |
|--------|------|------|
| `RGB_MATRIX_ROWS` | int | 矩阵行数 |
| `RGB_MATRIX_COLS` | int | 矩阵列数 |
| `RGB_MATRIX_LED_COUNT` | int | 灯珠数量 |

## 3. 渲染参数

| 配置项 | 类型 | 默认值 | 范围 | 说明 |
|--------|------|--------|------|------|
| `RGB_MATRIX_CENTER_X` | int | `108` | 0~255 | 灯效显示区域中心横坐标 |
| `RGB_MATRIX_CENTER_Y` | int | `32` | 0~255 | 灯效显示区域中心纵坐标 |
| `RGB_MATRIX_LED_FLUSH_LIMIT` | int | `16` | 1~100 | LED 刷新间隔（ms），控制灯效帧率 |
| `RGB_MATRIX_LED_PROCESS_LIMIT` | int | `RGB_MATRIX_LED_COUNT` | 1~255 | 每帧最多处理的 LED 数（分帧渲染），必须 `> 0`； |
| `RGB_WORKQ_STACK_SIZE` | int | `1024` | 建议640~1024或0 | 独立低优先workqueue栈（字节）；`0` = 复用系统 workqueue（省 RAM） |


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
| `RGB_MATRIX_MAXIMUM_BRIGHTNESS` | int | `225` | 1~255 | 最大亮度上限 |
| `RGB_MATRIX_SETTINGS_SAVE_DEBOUNCE` | int | 跟随 `ZMK_SETTINGS_SAVE_DEBOUNCE` | – | 灯效设置保存延迟（ms）：负值则不写入Flash |
| `RGB_MATRIX_DEFAULT_HUE` | int | `170` | 0~255 | 默认色相 |
| `RGB_MATRIX_DEFAULT_SAT` | int | `255` | 0~255 | 默认饱和度 |
| `RGB_MATRIX_DEFAULT_VAL` | int | `200` | 0~255 | 默认亮度 |
| `RGB_MATRIX_DEFAULT_SPD` | int | `127` | 0~255 | 默认速度 |
| `RGB_MATRIX_DEFAULT_ON` | bool | `y` | – | 开机默认开启 RGB |
| `RGB_MATRIX_DEFAULT_MODE` | string | `"CYCLE_LEFT_RIGHT"` | – | 默认灯效名（不含 `RGB_MATRIX_` 前缀，须已启用） |
| `RGB_MATRIX_KEEP_ON_WIRED` | bool | `n` | – | 有线（USB）模式空闲不熄灯；无线仍遵循 `ZMK_IDLE_TIMEOUT` |
| `RGB_LED_CONFIG_CONST` | bool | `n` | – | `g_led_config` 声明为 const 存 Flash（省 RAM）；keymap.c 需同步 const |
| `RGB_MATRIX_IS_LEFT` | bool | `n` | – | 强制 `is_keyboard_left()` 为 true；默认遵循 ZMK split（central = 左半） |

## 6. 灯效开关

全部为 `bool`、默认 `n`，置 `y` 启用。

枚举值用于 `&rgb_ug RGB_EFS_CMD <mode>` 键码切换到指定灯效，`<mode>` 即为枚举值，表中枚举值为启用所有灯效时的值，如有未启用灯效，后续枚举值前移。

| 值 | 配置项 | 分类 | 灯效 |
|----|--------|------|------|
| `1` | – | 普通 | `RGB_MATRIX_SOLID_COLOR` 单色常亮（始终内置） |
| `2` | `RGB_MATRIX_EFFECT_ALPHAS_MODS` | 普通 | 主键区/修饰键双色（Alphas Mods） |
| `3` | `RGB_MATRIX_EFFECT_GRADIENT_UP_DOWN` | 普通 | 上下渐变 |
| `4` | `RGB_MATRIX_EFFECT_GRADIENT_LEFT_RIGHT` | 普通 | 左右渐变 |
| `5` | `RGB_MATRIX_EFFECT_BREATHING` | 普通 | 呼吸 |
| `6` | `RGB_MATRIX_EFFECT_BAND_SAT` | 普通 | 饱和度彩带 |
| `7` | `RGB_MATRIX_EFFECT_BAND_VAL` | 普通 | 亮度彩带 |
| `8` | `RGB_MATRIX_EFFECT_BAND_PINWHEEL_SAT` | 普通 | 饱和度风车彩带 |
| `9` | `RGB_MATRIX_EFFECT_BAND_PINWHEEL_VAL` | 普通 | 亮度风车彩带 |
| `10` | `RGB_MATRIX_EFFECT_BAND_SPIRAL_SAT` | 普通 | 饱和度螺旋彩带 |
| `11` | `RGB_MATRIX_EFFECT_BAND_SPIRAL_VAL` | 普通 | 亮度螺旋彩带 |
| `12` | `RGB_MATRIX_EFFECT_CYCLE_ALL` | 普通 | 全局色相循环 |
| `13` | `RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT` | 普通 | 左右彩虹循环 |
| `14` | `RGB_MATRIX_EFFECT_CYCLE_UP_DOWN` | 普通 | 上下彩虹循环 |
| `15` | `RGB_MATRIX_EFFECT_RAINBOW_MOVING_CHEVRON` | 普通 | 移动人字形彩虹 |
| `16` | `RGB_MATRIX_EFFECT_CYCLE_OUT_IN` | 普通 | 由外向内循环 |
| `17` | `RGB_MATRIX_EFFECT_CYCLE_OUT_IN_DUAL` | 普通 | 双向由外向内循环 |
| `18` | `RGB_MATRIX_EFFECT_CYCLE_PINWHEEL` | 普通 | 风车循环 |
| `19` | `RGB_MATRIX_EFFECT_CYCLE_SPIRAL` | 普通 | 螺旋循环 |
| `20` | `RGB_MATRIX_EFFECT_DUAL_BEACON` | 普通 | 双信标 |
| `21` | `RGB_MATRIX_EFFECT_RAINBOW_BEACON` | 普通 | 彩虹信标 |
| `22` | `RGB_MATRIX_EFFECT_RAINBOW_PINWHEELS` | 普通 | 彩虹风车 |
| `23` | `RGB_MATRIX_EFFECT_FLOWER_BLOOMING` | 普通 | 花朵绽放 |
| `24` | `RGB_MATRIX_EFFECT_RAINDROPS` | 普通 | 雨滴 |
| `25` | `RGB_MATRIX_EFFECT_JELLYBEAN_RAINDROPS` | 普通 | 彩豆雨滴 |
| `26` | `RGB_MATRIX_EFFECT_HUE_BREATHING` | 普通 | 色相呼吸 |
| `27` | `RGB_MATRIX_EFFECT_HUE_PENDULUM` | 普通 | 色相摆动 |
| `28` | `RGB_MATRIX_EFFECT_HUE_WAVE` | 普通 | 色相波 |
| `29` | `RGB_MATRIX_EFFECT_PIXEL_RAIN` | 普通 | 像素雨 |
| `30` | `RGB_MATRIX_EFFECT_PIXEL_FLOW` | 普通 | 像素流 |
| `31` | `RGB_MATRIX_EFFECT_PIXEL_FRACTAL` | 普通 | 像素分形 |
| `32` | `RGB_MATRIX_EFFECT_TYPING_HEATMAP` | Framebuffer | 打字热力图 |
| `33` | `RGB_MATRIX_EFFECT_DIGITAL_RAIN` | Framebuffer | 数字雨 |
| `34` | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_SIMPLE` | Key Reactive | 单色按键点亮（简单） |
| `35` | `RGB_MATRIX_EFFECT_SOLID_REACTIVE` | Key Reactive | 单色按键点亮 |
| `36` | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_WIDE` | Key Reactive | 单色宽幅点亮 |
| `37` | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTIWIDE` | Key Reactive | 多键宽幅点亮 |
| `38` | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_CROSS` | Key Reactive | 十字点亮 |
| `39` | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTICROSS` | Key Reactive | 多键十字点亮 |
| `40` | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_NEXUS` | Key Reactive | Nexus 点亮 |
| `41` | `RGB_MATRIX_EFFECT_SOLID_REACTIVE_MULTINEXUS` | Key Reactive | 多键 Nexus 点亮 |
| `42` | `RGB_MATRIX_EFFECT_SPLASH` | Key Reactive | 彩色溅射（最近一键） |
| `43` | `RGB_MATRIX_EFFECT_MULTISPLASH` | Key Reactive | 彩色溅射（多键） |
| `44` | `RGB_MATRIX_EFFECT_SOLID_SPLASH` | Key Reactive | 单色溅射（最近一键） |
| `45` | `RGB_MATRIX_EFFECT_SOLID_MULTISPLASH` | Key Reactive | 单色溅射（多键） |
| `46` | `RGB_MATRIX_EFFECT_STARLIGHT_SMOOTH` | 普通 | 平滑星光 |
| `47` | `RGB_MATRIX_EFFECT_STARLIGHT` | 普通 | 星光 |
| `48` | `RGB_MATRIX_EFFECT_STARLIGHT_DUAL_SAT` | 普通 | 双饱和度星光 |
| `49` | `RGB_MATRIX_EFFECT_STARLIGHT_DUAL_HUE` | 普通 | 双色相星光 |
| `50` | `RGB_MATRIX_EFFECT_RIVERFLOW` | 普通 | 河流 |

> Key Reactive灯效 Single变体只处理最近一次击键，Multi变体处理更多（上限 8 条）击键记录。
>
> Key Reactive 和 Framebuffer 灯效都依赖设备树 `zmk,matrix_transform`，缺失时自动禁用。

## 7. `&rgb_ug` 键码一览

模块复用了 ZMK `&rgb_ug` 的键码，功能一致。

| 键码 | 作用 | Shift 反向 |
|------|------|-----------|
| `RGB_TOG` | 开关切换 | – |
| `RGB_ON` / `RGB_OFF` | 打开 / 关闭 | – |
| `RGB_HUI` / `RGB_HUD` | 色相 + / − | 支持 |
| `RGB_SAI` / `RGB_SAD` | 饱和度 + / − | 支持 |
| `RGB_BRI` / `RGB_BRD` | 亮度 + / −（受 `MAXIMUM_BRIGHTNESS` 限制） | 支持 |
| `RGB_SPI` / `RGB_SPD` | 速度 + / − | 支持 |
| `RGB_EFF` / `RGB_EFR` | 下一个 / 上一个灯效 | 支持 |
| `RGB_EFS_CMD <mode>` | 切换到指定灯效 | – |
| `RGB_COLOR_HSB(h, s, v)` | 直接设置颜色：h/s/v 均为 0~255，（亮度受 `MAXIMUM_BRIGHTNESS` 限制）  | – |


> `RGB_EFS_CMD` / `RGB_COLOR_HSB` 为较新 ZMK 引入的命令，通过条件编译兼容。

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
