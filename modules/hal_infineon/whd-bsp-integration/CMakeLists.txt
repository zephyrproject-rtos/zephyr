# Copyright (c) 2022 Cypress Semiconductor Corporation.
#
# SPDX-License-Identifier: Apache-2.0

# Add whd-bsp-integration sources
zephyr_include_directories(${ZEPHYR_HAL_INFINEON_MODULE_DIR}/whd-bsp-integration/)
zephyr_library_sources(${ZEPHYR_HAL_INFINEON_MODULE_DIR}/whd-bsp-integration/cybsp_wifi.c)
