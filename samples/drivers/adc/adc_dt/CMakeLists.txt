# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(eval_cn0391_ardz IN_LIST SHIELD)
  set(EXTRA_DTC_OVERLAY_FILE boards/eval_cn0391_ardz.overlay)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ADC)

target_sources(app PRIVATE src/main.c)
