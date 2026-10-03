/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

/* The BL/BX instructions themselves will be worth 1-3 overhead cycles depending on optimisation */
void __noinline msp_delay_peripheral_startup(void)
{
#define NOP(i, ...)    "nop;"
#define NOP_N_TIMES(n) LISTIFY(n, NOP, ())

	__asm__ volatile(NOP_N_TIMES(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));
}
