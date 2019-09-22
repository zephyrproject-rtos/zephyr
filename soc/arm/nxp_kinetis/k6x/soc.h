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

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(120)
#define BUSCLK_DEFAULT_IOSC_HZ (SYSCLK_DEFAULT_IOSC_HZ / \
				CONFIG_K64_BUS_CLOCK_DIVIDER)

/* address bases */

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

/* Add include for DTS generated information */
#include <generated_dts_board.h>

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
