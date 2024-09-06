# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qomu)

target_sources(app PRIVATE src/main.c src/usb2serial_bit.c)
