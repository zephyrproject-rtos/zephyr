# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MCUBOOT_ESPRESSIF)
  return()
endif()

zephyr_sources(
  vectors.S
  soc_irq.S
  soc.c
  ../common/loader.c
  )

zephyr_include_directories(.)

zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)
