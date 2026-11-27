/*
 * Copyright The Zephyr Project Contributors
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree/sram.h>

/** \brief MPU Region Base Address Register Definitions */
#define MPU_RBAR_BASE_Pos		5					/*!< MPU RBAR: BASE Position */
#define MPU_RBAR_BASE_Msk		(0x7FFFFFF << MPU_RBAR_BASE_Pos)	/*!< MPU RBAR: BASE Mask */

/** \brief MPU Region Limit Address Register Definitions */
#define MPU_RLAR_LIMIT_Pos		5					/*!< MPU RLAR: LIMIT Position */
#define MPU_RLAR_LIMIT_Msk		(0x7FFFFFF << MPU_RLAR_LIMIT_Pos)	/*!< MPU RLAR: LIMIT Mask */

#define REGION_LIMIT_ADDR(base, size) (((base & MPU_RBAR_BASE_Msk) + size - 1) & MPU_RLAR_LIMIT_Msk)

#define RAM_LIMIT_SIZE DT_CHOSEN_SRAM_SIZE

PROVIDE(__rom_region_limit = REGION_LIMIT_ADDR(__rom_region_start, __rom_region_size));
/* SRAM_RW region spans from SRAM start to SRAM end, covering all kernel-writable
 * memory including code, initialized data, heap, and stack. Use DT_CHOSEN_SRAM_ADDR
 * (device tree SRAM address) instead of __rom_region_start to explicitly reference
 * SRAM boundaries and avoid ROM/RAM naming confusion in RAM-loaded images.
 */
PROVIDE(__image_ram_limit = REGION_LIMIT_ADDR(DT_CHOSEN_SRAM_ADDR, DT_CHOSEN_SRAM_SIZE));
