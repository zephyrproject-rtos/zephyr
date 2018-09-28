/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Quark SE platform, both standard images and XIP
 * images.
 */

/* Flash base address and size */
#define FLASH_START     CONFIG_FLASH_BASE_ADDRESS  /* Flash bank 1 */
#define FLASH_SIZE      CONFIG_FLASH_SIZE

/*
 * SRAM base address and size
 *
 * Internal SRAM includes the exception vector table at reset, which is at
 * the beginning of the region.
 */
#define SRAM_START		CONFIG_SRAM_BASE_ADDRESS
#define SRAM_SIZE		CONFIG_SRAM_SIZE

/* Data Closely Coupled Memory (DCCM) base address and size */
#define DCCM_START		CONFIG_DCCM_BASE_ADDRESS
#define DCCM_SIZE		CONFIG_DCCM_SIZE

#include <generated_dts_board.h>
#include <arch/arc/v2/linker.ld>
