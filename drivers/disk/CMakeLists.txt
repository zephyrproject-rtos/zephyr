# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DISK_DRIVERS)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_FLASH flashdisk.c)
zephyr_library_sources_ifdef(CONFIG_DISK_DRIVER_RAM ramdisk.c)

zephyr_library_sources_ifdef(CONFIG_SDMMC_OVER_SPI sdmmc_spi.c)
zephyr_library_sources_ifdef(CONFIG_SDMMC_STM32 sdmmc_stm32.c)
zephyr_library_sources_ifdef(CONFIG_SDMMC_USDHC usdhc.c)
zephyr_library_sources_ifdef(CONFIG_SDMMC_DW mmc_ll.c mmc_dw_ll.c sdmmc_dw.c)

endif()
