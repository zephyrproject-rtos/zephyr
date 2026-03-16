# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  ../common/loader.c
  )

zephyr_include_directories(.)

zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)

zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fix.c)

# Power Management
zephyr_library_sources_ifdef(CONFIG_PM ../common/power.c)
zephyr_library_sources_ifdef(CONFIG_POWEROFF ../common/poweroff.c)
