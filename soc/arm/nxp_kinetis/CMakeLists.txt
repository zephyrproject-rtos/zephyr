# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_KINETIS_FLASH_CONFIG	flash_configuration.c)

add_subdirectory(${SOC_SERIES})
