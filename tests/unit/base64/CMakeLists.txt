# SPDX-License-Identifier: Apache-2.0

project(base64)
set(SOURCES main.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
