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
