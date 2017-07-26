/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the nxp_lpc54114 platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'nxp_lpc54114' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>
#include <fsl_common.h>
#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#define BOARD_BOOTCLOCKFROHF48M_CORE_CLOCK		48000000U

#endif /* _SOC__H_ */
