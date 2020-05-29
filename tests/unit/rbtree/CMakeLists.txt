# SPDX-License-Identifier: Apache-2.0

project(rbtree)
set(SOURCES main.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
