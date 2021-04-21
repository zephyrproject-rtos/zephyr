# SPDX-License-Identifier: Apache-2.0

project(math_extras)
set(SOURCES main.c portable.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
