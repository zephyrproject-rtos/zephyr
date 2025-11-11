# Copyright (c) 2024 Cypress Semiconductor Corporation (an Infineon company) or
# an affiliate of Cypress Semiconductor Corporation
#
# SPDX-License-Identifier: Apache-2.0

set(serial_flash_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/serial-flash)

zephyr_include_directories(${serial_flash_dir})

zephyr_library_sources(${serial_flash_dir}/cy_serial_flash_qspi.c)
