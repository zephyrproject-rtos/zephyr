# SPDX-License-Identifier: Apache-2.0

project(timeutil)
set(SOURCES main.c test_gmtime.c  test_s32.c test_s64.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
