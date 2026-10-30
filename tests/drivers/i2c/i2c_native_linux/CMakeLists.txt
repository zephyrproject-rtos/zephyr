# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2c_native_linux)

target_sources(app PRIVATE src/main.c)

# Fake host i2c-dev adapters behind the host call hooks of the driver bottom
target_compile_options(native_simulator INTERFACE -DI2C_NATIVE_LINUX_TEST_HOOKS)
target_sources(native_simulator INTERFACE src/fake_i2c_dev_bottom.c)
