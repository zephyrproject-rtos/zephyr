# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SOC_ESP32_APPCPU)
  zephyr_sources(
    soc_appcpu.c
    )
else()
  zephyr_sources(
    soc.c
    esp32-mp.c
    ../common/loader.c
    )
endif()

zephyr_include_directories(.)

zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)

zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fix.c)

zephyr_library_sources_ifdef(CONFIG_GDBSTUB gdbstub.c)

zephyr_library_sources_ifdef(CONFIG_PM power.c)
zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)
