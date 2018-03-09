/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#if defined(CONFIG_SOC_MPS2_AN521)
#define __SAUREGION_PRESENT       1U        /* SAU regions present */
#define __FPU_PRESENT             CONFIG_CPU_HAS_FPU
#define __DSP_PRESENT             1U        /* no DSP extension present */
#endif

#include <soc_devices.h>

#endif /* _SOC_H_ */
