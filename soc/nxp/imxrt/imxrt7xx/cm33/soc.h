/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the MIMXRT7XX platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'MIMXRT7XX' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE

#include <zephyr/sys/util.h>
#include <fsl_common.h>

/* CPU 0 has an instruction and data cache, provide the defines for XCACHE */
#ifdef CONFIG_SOC_MIMXRT798S_CM33_CPU0
#define NXP_XCACHE_INSTR XCACHE1
#define NXP_XCACHE_DATA XCACHE0
#endif

#ifdef __cplusplus
extern "C" {
#endif

void xspi_clock_safe_config(void);
void xspi_setup_clock(XSPI_Type *base, uint32_t src, uint32_t divider);

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
