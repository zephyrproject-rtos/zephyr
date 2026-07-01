# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rpi_zero_2w_usb_cdc_min)

target_sources(app PRIVATE src/main.c)
