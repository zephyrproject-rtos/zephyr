# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DISK_DRIVERS)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_FLASH flashdisk.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_RAM ramdisk.c)

zephyr_library_sources_ifdef(CONFIG_SDMMC_STM32 sdmmc_stm32.c)
zephyr_library_sources_ifdef(CONFIG_SDMMC_SUBSYS sdmmc_subsys.c)

endif()
