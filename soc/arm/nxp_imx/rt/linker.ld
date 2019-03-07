/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <autoconf.h>
 #include <generated_dts_board.h>

/* extra memory area */
MEMORY
     {
#if DT_MCUX_OCRAM0_SIZE > 0
	OCRAM0	(wx) : ORIGIN = DT_MCUX_OCRAM0_BASE_ADDRESS, LENGTH = DT_MCUX_OCRAM0_SIZE
#endif
#if DT_MCUX_OCRAM1_SIZE > 0
	OCRAM1	(wx) : ORIGIN = DT_MCUX_OCRAM1_BASE_ADDRESS, LENGTH = DT_MCUX_OCRAM1_SIZE
#endif
#if (DT_MCUX_SRAM0_SIZE > 0) && !defined(CONFIG_DATA_SEMC)
	SDRAM0	(wx) : ORIGIN = DT_MCUX_SRAM0_BASE_ADDRESS, LENGTH = DT_MCUX_SRAM0_SIZE
#endif
     }

#include <arch/arm/cortex_m/scripts/linker.ld>
