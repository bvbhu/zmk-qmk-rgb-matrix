# RGB Matrix 模块 CMakeLists.txt
#
# 将 src/rgb_matrix/controller/ 下的 .c 源文件加入编译。
# 模板文件 keymap.c（LED 布局示例）不包含在内，由各键盘仓通过
# snippet 注入各自的 keymap.c。
#
# 包含路径：
# - 模块源码目录（PUBLIC，供本模块及下游使用）
# - 键盘仓 config/ 目录（由 snippet 通过 PRIVATE 加入，包含 keymap.c）
# - 生成的 rgb_matrix_generated_config.h（由 snippet 生成并加入 include 路径）

# 模块源码包含路径
target_include_directories(${ZEPHYR_CURRENT_TARGET} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src/rgb_matrix
    ${CMAKE_CURRENT_LIST_DIR}/src/rgb_matrix/controller
    ${CMAKE_CURRENT_LIST_DIR}/src/rgb_matrix/animations
    ${CMAKE_CURRENT_LIST_DIR}/src/rgb_matrix/animations/runners
)

# 仅编译 controller/ 下的模块源码（排除模板 keymap.c）
file(GLOB RGB_MATRIX_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/rgb_matrix/controller/*.c"
)

target_sources(${ZEPHYR_CURRENT_TARGET} PRIVATE ${RGB_MATRIX_SOURCES})
