/*
 * Copyright (c) 2017-2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#define __MPU_PRESENT 1

#if defined(CONFIG_SOC_MPS3_AN547)
#define __SAUREGION_PRESENT       1U        /* SAU regions present */
#define __FPU_PRESENT             CONFIG_CPU_HAS_FPU
#define __DSP_PRESENT             1U        /* DSP extension present */
#define __MVE_PRESENT             1U        /* MVE extensions present */
#define __MVE_FP                  1U        /* MVE floating point present */
#endif


#endif /* _SOC_H_ */
