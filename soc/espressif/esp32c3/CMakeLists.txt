# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  vectors.S
  soc_irq.S
  soc.c
  ../common/loader.c
  )

zephyr_include_directories(.)

zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)

zephyr_library_sources_ifdef(CONFIG_PM ../common/power.c)
zephyr_library_sources_ifdef(CONFIG_POWEROFF ../common/poweroff.c)
