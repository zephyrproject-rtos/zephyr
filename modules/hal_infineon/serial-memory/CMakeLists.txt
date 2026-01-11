# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

set(serial_memory_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/serial-memory)

zephyr_include_directories(${serial_memory_dir}/include)
zephyr_include_directories(${serial_memory_dir}/source)

zephyr_library_sources(${serial_memory_dir}/source/mtb_serial_memory.c)
