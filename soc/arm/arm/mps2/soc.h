/*
 * Copyright (c) 2017-2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#define __MPU_PRESENT 1

#if defined(CONFIG_SOC_MPS2_AN521)
#define __SAUREGION_PRESENT       CONFIG_CPU_HAS_ARM_SAU
#define __FPU_PRESENT             CONFIG_CPU_HAS_FPU
#define __DSP_PRESENT             CONFIG_ARMV8_M_DSP

#endif

#include <soc_registers.h>

extern void wakeup_cpu1(void);

extern uint32_t sse_200_platform_get_cpu_id(void);

#endif /* _SOC_H_ */
