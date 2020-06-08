# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(NO_QEMU_SERIAL_BT_SERVER 1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth)

target_sources(app PRIVATE src/bluetooth.c)
