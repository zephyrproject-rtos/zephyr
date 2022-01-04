# Copyright (c) 2021 Teslabs Engineering S.L.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(pinctrl_gd32)

target_sources(app PRIVATE ../common/test_device.c)
if(CONFIG_PINCTRL_GD32_AF)
  target_sources(app PRIVATE src/main_af.c)
elseif(CONFIG_PINCTRL_GD32_AFIO)
  target_sources(app PRIVATE src/main_afio.c)
endif()
