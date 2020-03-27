# SPDX-License-Identifier: Apache-2.0

project(crc)
set(SOURCES main.c)
find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
