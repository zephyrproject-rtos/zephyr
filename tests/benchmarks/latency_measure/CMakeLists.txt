# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(small_freq_divider_frdm_k64f TRUE)
set(small_freq_divider_arduino_due TRUE)
set(small_freq_divider_qemu_cortex_m3 TRUE)

if(small_freq_divider_${BOARD})
  set(CONF_FILE prj_small_freq_divider.conf)
else()
  set(CONF_FILE prj.conf)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(latency_measure)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
