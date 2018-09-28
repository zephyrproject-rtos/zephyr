/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Synopsys EM Starterkit platform.
 */

#include <generated_dts_board.h>


/* Instruction Closely Coupled Memory (ICCM) base address and size */
#if defined(CONFIG_ICCM_BASE_ADDRESS) && (CONFIG_ICCM_SIZE > 0)
#define ICCM_START		CONFIG_ICCM_BASE_ADDRESS
#define ICCM_SIZE		CONFIG_ICCM_SIZE
#endif

/*
 * DCCM base address and size. DCCM is the data memory.
 */
/* Data Closely Coupled Memory (DCCM) base address and size */
#if defined(CONFIG_DCCM_BASE_ADDRESS) && (CONFIG_DCCM_SIZE > 0)
#define DCCM_START		CONFIG_DCCM_BASE_ADDRESS
#define DCCM_SIZE		CONFIG_DCCM_SIZE
#endif

#include <arch/arc/v2/linker.ld>
