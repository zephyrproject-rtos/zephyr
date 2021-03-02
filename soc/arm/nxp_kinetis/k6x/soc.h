/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the fsl_frdm_k64f platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'fsl_frdm_k64f' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <sys/util.h>

/* default system clock */

#if defined(CONFIG_SOC_MK64F12)
#define SYSCLK_DEFAULT_IOSC_HZ MHZ(120)
#elif defined(CONFIG_SOC_MK66F18)
#define SYSCLK_DEFAULT_IOSC_HZ MHZ(180)
#endif

#define CLOCK_DIVIDER(prop) \
	DT_PROP_OR(DT_INST(0, nxp_kinetis_sim), prop, 1) - 1

#define BUSCLK_DEFAULT_IOSC_HZ (SYSCLK_DEFAULT_IOSC_HZ / \
				CLOCK_DIVIDER(bus_clk_divider))

/* address bases */

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

/* Add include for DTS generated information */
#include <devicetree.h>

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
