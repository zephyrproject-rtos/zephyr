# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_TI_K3_OSPI_FLASH_BOOT_MODE_RESET common/ospi_flash_boot_reset.c)

add_subdirectory(${SOC_SERIES})
