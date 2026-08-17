# RGB Matrix snippet — 注入键盘仓 keymap.c
#
# 配置头文件 rgb_matrix_generated_config.h 由模块主 CMakeLists.txt 生成，
# 本 snippet 仅负责将键盘仓的 keymap.c 加入编译并加入包含路径。
#
# 键盘仓需通过以下方式激活本 snippet：
#   - 在键盘仓 CMakeLists.txt 中 include(snippets/rgb_matrix/CMakeLists.txt)
#   - 或通过 west.yml / board/shield 配置显式引用

zephyr_include_directories(${ZMK_CONFIG})
