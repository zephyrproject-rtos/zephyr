# SPDX-License-Identifier: Apache-2.0

project(util)
set(SOURCES main.c ../../../lib/os/dec.c)
find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
