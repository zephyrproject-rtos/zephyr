#-------------------------------------------------------------------------------
# ATH20+BMP280 Board Sample
#
# Copyright (c) 2023 Russ Webber
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(ath20_bmp280_board LANGUAGES C)

target_sources(app PRIVATE src/main.c)
