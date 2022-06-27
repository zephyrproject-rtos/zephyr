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

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* address bases */

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/random/rand32.h>

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
