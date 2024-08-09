#
# Copyright (c) 2024, Chaim Zax
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensor_adxl34x_test)

target_include_directories(app PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../../../drivers/sensor/adi/adxl34x"
)
target_sources(app PRIVATE
  src/adxl34x_test.c
  src/adxl34x_test.h
  src/adxl34x_basic.c
  src/adxl34x_decoder.c
  src/adxl34x_streaming.c
  src/adxl34x_extended.c
  src/adxl34x_i2c.c
  src/adxl34x_spi.c
)
# Let's speedup development. DISABLE IN PRODUCTION!
#target_compile_options(app PRIVATE -Wno-error=unused-variable -Wno-error=unused-function)
