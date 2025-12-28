/*
 * Copyright (c) 2024 Microchip Technology Inc
 *
 * SPDX-License-Identifier: (GPL-2.0 OR MIT)
 */

/**
 * @file
 * @brief System/hardware module for PIC64GX processor
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_IPM_MCHP_IHC_REMOTEPROC_STOP_ADDR)
/* In case of a critical failure we need to let control to Master */
FUNC_NORETURN void arch_system_halt(unsigned int reason)
{
	// Use that trick to call the function in the scratchpad with -mo-pie activated
	// As a side note, we don't clean properly interrupt context here, this is left to
	// initialisation code
	__asm__ volatile ("mv a0, %0\n\t"
			  "jalr ra, 0(%0)\n\t"
			  :: "r" (CONFIG_IPM_MCHP_IHC_REMOTEPROC_STOP_ADDR),
			  "r" (reason)
			  :"ra", "a0");

	CODE_UNREACHABLE;
}
#endif
