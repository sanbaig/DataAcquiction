
# 指定 CMake 最低版本
cmake_minimum_required(VERSION 3.20.0)

# 查找并加载 Zephyr SDK 构建环境
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# 定义当前工程名称
project(StretchSense_BLE_Test)

# 将 main.c 添加到当前应用程序的编译列表
target_sources(app PRIVATE src/main.c)
