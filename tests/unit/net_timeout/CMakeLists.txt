# SPDX-License-Identifier: Apache-2.0

project(lib_net_timeout)
set(SOURCES main.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
