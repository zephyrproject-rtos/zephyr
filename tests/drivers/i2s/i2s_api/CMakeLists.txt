# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2s_api)

FILE(GLOB app_sources src/*.c)
list(FILTER app_sources EXCLUDE REGEX "test_i2s_pm\\.c$")
target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_PM app PRIVATE src/test_i2s_pm.c)
