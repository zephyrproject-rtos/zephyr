/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) Thomas Burdick <thomas.burdick@gmail.com>

 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the fsl_frdm_k22f platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'fsl_frdm_k22f' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* default system clock */

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(120)
#define BUSCLK_DEFAULT_IOSC_HZ (SYSCLK_DEFAULT_IOSC_HZ / \
				CONFIG_K22_BUS_CLOCK_DIVIDER)

/* address bases */

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <device.h>
#include <misc/util.h>
#include <random/rand32.h>

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
