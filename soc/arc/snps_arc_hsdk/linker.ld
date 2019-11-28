/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the HS Development Kit
 */

#include <generated_dts_board.h>
#include <autoconf.h>

/*
 * SRAM base address and size
 */
#if defined(CONFIG_SRAM_BASE_ADDRESS) && (CONFIG_SRAM_SIZE > 0)
#define SRAM_START		CONFIG_SRAM_BASE_ADDRESS
#define SRAM_SIZE		CONFIG_SRAM_SIZE
#endif


#include <arch/arc/v2/linker.ld>
