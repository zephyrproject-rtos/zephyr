# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_STM32_BOOTLOADER)

  # Transport-agnostic sources.
  zephyr_library_sources(stm32_bootloader_gpio.c)

  # Transport-specific drivers.
  add_subdirectory_ifdef(CONFIG_STM32_BOOTLOADER_UART usart)

endif()
