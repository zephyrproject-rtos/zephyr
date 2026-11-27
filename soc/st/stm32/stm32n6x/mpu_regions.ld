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

PROVIDE(__rom_region_limit = REGION_LIMIT_ADDR(__rom_region_start, __rom_region_size));
/* SRAM_RW starts at _image_ram_start and extends to the end of the chosen SRAM,
 * covering the dynamic MPU area without overlapping the memory that follows.
 */
PROVIDE(__image_ram_limit = REGION_LIMIT_ADDR(_image_ram_start,
		DT_CHOSEN_SRAM_SIZE - (_image_ram_start - DT_CHOSEN_SRAM_ADDR)));
