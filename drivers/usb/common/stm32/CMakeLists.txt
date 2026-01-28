# Copyright (c) 2025 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_STM32_USB_COMMON)
  zephyr_library()

  zephyr_include_directories(.)

  zephyr_library_sources(stm32_usb_pwr.c)
endif()
