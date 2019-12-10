# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(up_squared_adsp)

if(CONFIG_DMA)
  if (NOT CONFIG_I2S)
    target_sources(app PRIVATE src/dma_test.c)
  endif()
endif()

if(CONFIG_I2S)
  target_sources(app PRIVATE src/i2s_test.c)
endif()

target_sources(app PRIVATE src/main.c)
