# Copyright (c) 2025 Quectel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
zephyr_library_include_directories(driver)

zephyr_library_sources_ifdef(USBH_STM32_DRIVE_QUECTEL driver/quec_uhc_driver_stm32.c)

zephyr_library_sources(driver/quec_ringbuffer.c)
zephyr_library_sources(driver/quec_uhc_memory.c)
zephyr_library_sources(driver/quec_uhc_enumeration.c)

zephyr_library_sources(quec_uhc_userial.c)


