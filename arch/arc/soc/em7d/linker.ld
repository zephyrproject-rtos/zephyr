/*
 * Copyright (c) 2016 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Synopsys EM Starterkit v2.2 EM7D platform.
 */

/*
 * DRAM base address and size
 *
 * DRAM includes the exception vector table at reset, which is at
 * the beginning of the region.
 */
#define SRAM_START		CONFIG_SRAM_BASE_ADDRESS
#define SRAM_SIZE		CONFIG_SRAM_SIZE
/* TODO: Using SRAM config for now, even though this is really DRAM. */

/* Instruction Closely Coupled Memory (ICCM) base address and size */
#define ICCM_START		CONFIG_ICCM_BASE_ADDRESS
#define ICCM_SIZE		CONFIG_ICCM_SIZE

/*
 * DCCM base address and size. DCCM is the data memory.
 */
/* Data Closely Coupled Memory (DCCM) base address and size */
#define DCCM_START		CONFIG_DCCM_BASE_ADDRESS
#define DCCM_SIZE		CONFIG_DCCM_SIZE

#include <arch/arc/v2/linker.ld>
