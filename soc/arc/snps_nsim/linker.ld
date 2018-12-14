/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Synopsys EM Starterkit platform.
 */

#include <generated_dts_board.h>
#include <autoconf.h>

/* Instruction Closely Coupled Memory (ICCM) base address and size */
#if defined(DT_ICCM_BASE_ADDRESS) && (DT_ICCM_SIZE > 0)
#define ICCM_START		DT_ICCM_BASE_ADDRESS
#define ICCM_SIZE		DT_ICCM_SIZE
#endif

/*
 * DCCM base address and size. DCCM is the data memory.
 */
/* Data Closely Coupled Memory (DCCM) base address and size */
#if defined(DT_DCCM_BASE_ADDRESS) && (DT_DCCM_SIZE > 0)
#define DCCM_START		DT_DCCM_BASE_ADDRESS
#define DCCM_SIZE		DT_DCCM_SIZE
#endif

#include <arch/arc/v2/linker.ld>
