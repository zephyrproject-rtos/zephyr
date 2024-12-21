# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(usb_audio_async_i2s)

include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
target_sources(app PRIVATE src/main.c)

if (CONFIG_SOC_COMPATIBLE_NRF5340_CPUAPP)
	target_sources(app PRIVATE src/feedback_nrf53.c)
else()
	target_sources(app PRIVATE src/feedback_dummy.c)
endif()
