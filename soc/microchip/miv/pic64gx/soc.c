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
#include <string.h>

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

#if defined(CONFIG_PIC64GX_RELOCATE_RESOURCE_TABLE)

/**
 * @brief Copy resource table to a specific address in order to allow linux
 * to find it in case the firmwaer was loaded by a bootloader and not Linux remoteproc
 *
 * @return 0
 */
static int pic64gx_rsctable_init(void)
{
	extern uintptr_t __rsctable_start;
	extern uintptr_t __rsctable_end;
	extern uintptr_t __rsctable_load_start;

	memcpy((void *)&__rsctable_start,
	       (void *)&__rsctable_load_start,
	       (uintptr_t)&__rsctable_end - (uintptr_t)&__rsctable_start);

	return 0;
}

SYS_INIT(pic64gx_rsctable_init, APPLICATION, 0);
#endif