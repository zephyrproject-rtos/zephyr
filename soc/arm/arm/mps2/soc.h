/*
 * Copyright (c) 2017-2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#define __MPU_PRESENT 1

#if defined(CONFIG_SOC_MPS2_AN521)
#define __SAUREGION_PRESENT       1U        /* SAU regions present */
#define __FPU_PRESENT             CONFIG_CPU_HAS_FPU
#define __DSP_PRESENT             1U        /* DSP extension present */
#endif

#include <generated_dts_board.h>
#include <soc_registers.h>

extern void wakeup_cpu1(void);

extern u32_t sse_200_platform_get_cpu_id(void);

#endif /* _SOC_H_ */
