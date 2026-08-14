#!/bin/bash
set -e

# 进入 demo/metapushstream7 目录并执行 CMake 构建。
cd demo/metapushstream7

# 清理旧构建目录
rm -rf build

# 生成构建系统
cmake -B build

# 并行编译
cmake --build build -j4
