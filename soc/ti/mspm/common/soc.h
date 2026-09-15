/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MSPM0_SOC_H
#define _MSPM0_SOC_H

#ifdef CONFIG_HAS_MSPM0_SDK
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/m0p/dl_core.h>
#endif

#include <cmsis_core_m_defaults.h>

static ALWAYS_INLINE void msp_delay_peripheral_startup()
{
#define NOP(i, ...)    "nop;"
#define NOP_N_TIMES(n) LISTIFY(n, NOP, ())

	__asm__ volatile(NOP_N_TIMES(CONFIG_MSPM0_PERIPH_STARTUP_DELAY));

#undef NOP_N_TIMES
#undef NOP
}

#endif /* _MSPM0_SOC_H */
