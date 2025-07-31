/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E single-core M4
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file for LPC54S018J4MET180E
 *
 * Memory layout for LPC54S018J4MET180E:
 * - SRAMX: 192KB @ 0x00000000 (code execution, XIP)
 * - SRAM0: 64KB @ 0x20000000
 * - SRAM1: 64KB @ 0x20010000  
 * - SRAM2: 32KB @ 0x20020000
 * - USB_SRAM: 8KB @ 0x40100000
 * - SPIFI Flash: 4MB @ 0x10000000 (external QSPI flash, W25Q32JV)
 */

/* The SDK CMSIS SystemInit function sets VTOR equal to &__Vectors,
 * so we create an alias for the symbol to Zephyr's vector table
 */
__Vectors = _vector_table;

/* Include the common Cortex-M linker script */
#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>