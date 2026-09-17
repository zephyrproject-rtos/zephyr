# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)
add_subdirectory(${CONFIG_SOC_SERIES})

# Scratch buffers used by the SROM flash API live in .cy_sharedmem
zephyr_linker_sources(NOINIT common/noinit.ld)
