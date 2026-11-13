# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2c_target_api)

add_subdirectory(common)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_I2C_TARGET_API_PM app PRIVATE src/test_i2c_pm.c)
