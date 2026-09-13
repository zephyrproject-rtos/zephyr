# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

# Place the Infineon CAT1C SROM shared-memory scratch buffers. The PDL flash
# driver (cy_flash_srom.c) tags them with CY_SECTION_SHAREDMEM (".cy_sharedmem").
# Without explicit placement the section is an orphan; under the default
# ORPHAN_SECTION_WARN the linker silently puts this must-be-SRAM scratch into
# read-only flash, which faults at runtime. Anchor it in SRAM via NOINIT.
zephyr_linker_sources(NOINIT noinit.ld)
