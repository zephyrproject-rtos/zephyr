

# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# RW6xx SDIO device-side driver
zephyr_library_sources_ifdef(CONFIG_SD_DEV_RW6XX_SDIO
    rw6xx_sdio_device.c
)
