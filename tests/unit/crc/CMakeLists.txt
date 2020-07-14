# SPDX-License-Identifier: Apache-2.0

project(crc)
set(SOURCES main.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
