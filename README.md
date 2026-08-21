# zmk-qmk-rgb-matrix

> **⚠️** 本项目代码由 AI 生成，仍处于开发测试阶段，**不保证可用性**，请自行验证是否可用。


在 ZMK 键盘使用 QMK 的 RGB Matrix 灯效系统。

模块使用了 QMK RGB Matrix 源码（GPL-2.0-or-later许可），故项目统一为GPL-2.0-or-later（具体文件归属参考 [LICENSE](LICENSE)，以各文件头部的版权与 SPDX 声明为准）。

**最终固件必须以 GPL-2.0-or-later 兼容条款分发**。

## 核心特性

- **实现 QMK 所有内置灯效** ：在 zmk 中适配了 QMK RGB Matrix 系统，可以配置启用的灯效。
- **使用 `&rgb_ug` 键码控制** ：实现了所有`&rgb_ug` 键码的功能，按Shift可反向调节，减少控制占用按键数。
- Kconfig 配置，尽量少的配置项：大部分参数使用Kconfig配置，自动转换为#define定义，除了与键盘强相关的配置和灯效列表，其他配置项都可以使用默认值
- **QMK 兼容**：g_led_config、rgb_matrix_indicators_advanced_user 与 QMK 格式一致

## 快速接入

仅需要修改/创建四个文件, 完整步骤见 [USAGE](USAGE.md)。

```yaml
# 1. config/west.yml 添加模块
# manifest:
#   defaults:
#     revision: main
#   remotes:
    # - name: zmkfirmware
    #   url-base: https://github.com/zmkfirmware
    - name: bvbhu-zmk-qmk-rgb-matrix # 可以修改，但要与projects.remote一致
      url-base: https://github.com/bvbhu
#   projects:
    # - name: zmk
    #   remote: zmkfirmware
    #   import: app/west.yml
    - name: zmk-qmk-rgb-matrix
      remote: bvbhu-zmk-qmk-rgb-matrix
      revision: main
      path: modules/rgb-matrix
#   self:
#     path: config
```

```conf
# 2. config/<keyboard>.conf — 配置必须项
# 矩阵行数
CONFIG_RGB_MATRIX_ROWS=6
# 矩阵列数
CONFIG_RGB_MATRIX_COLS=16
# 灯珠数量（非分体）
CONFIG_RGB_MATRIX_LED_COUNT=96
# 分体键盘需要分别定义左右半的灯珠数量（非分体无需定义）
RGB_MATRIX_SPLIT_LED_COUNT_LEFT=48
RGB_MATRIX_SPLIT_LED_COUNT_RIGHT=48

# 配置LED 驱动（视硬件与 Zephyr 版本按需选择）----
CONFIG_WS2812_STRIP=y               # WS2812 驱动（示例，按实际硬件调整）

# 启用一些灯效（以下仅是部分）
CONFIG_RGB_MATRIX_EFFECT_CYCLE_LEFT_RIGHT=y # 默认灯效的默认值，建议保留
CONFIG_RGB_MATRIX_EFFECT_GRADIENT_UP_DOWN=y
CONFIG_RGB_MATRIX_EFFECT_BREATHING=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_ALL=y
CONFIG_RGB_MATRIX_EFFECT_CYCLE_PINWHEEL=y
```

```c
// 3. config/keymap.c 定义 LED 布局
#include <rgb_matrix.h>
led_config_t g_led_config =
{
    {
        /* [0] matrix_co[RGB_MATRIX_ROWS][RGB_MATRIX_COLS]：矩阵位置 → 灯珠编号，无 LED 填 NO_LED */
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 },
        /* ... 其余行 ... */
    },
    {
        /* [1] point[LED_COUNT]：每颗 LED 物理坐标 {x, y}，范围 0~255 ,注意按照灯珠编号顺序填写而不是行列顺序*/
        { 0, 64 }, { 16, 64 }, { 32, 64 }, /* ... */
    },
    {
        /* [2] flags[LED_COUNT]：建议统一使用键位灯4，mod灯1仅ALPHAS_MODS灯效有意义，也按灯珠编号顺序*/
        4, 4, 4, 4, 4, 4, /* ... */
    }
}
```

```dts
// 4. config/<keyboard>.dtsi 或 overlay — 设备树配置
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

## 更多资料

- [适配指南](RGB_Matrix_适配指南.md) / [配置项表](RGB_Matrix_配置项表.md)
- [ZMK Module Creation](https://zmk.dev/docs/development/module-creation)
- [Zephyr Modules](https://docs.zephyrproject.org/3.5.0/develop/modules.html)
- [ZMK 使用模块](https://zmk.dev/docs/features/modules)
- [Zephyr west manifest](https://docs.zephyrproject.org/3.5.0/develop/west/manifest.html#west-manifests)
