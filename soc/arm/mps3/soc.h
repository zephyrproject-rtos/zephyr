/*
 * Copyright (c) 2017-2021 Linaro Limited
 * Copyright (c) 2026 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_H_
#define _SOC_H_

#if defined(CONFIG_CPU_CORTEX_M)
#include <cmsis_core_m_defaults.h>
#endif

#if defined(CONFIG_SOC_AN536)

/* Do not let CMSIS to handle GIC and Timer */
#define __GIC_PRESENT 0
#define __TIM_PRESENT 0

#endif

#endif /* _SOC_H_ */
