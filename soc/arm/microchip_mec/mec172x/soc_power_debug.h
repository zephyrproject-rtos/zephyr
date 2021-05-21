/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_POWER_DEBUG_H__
#define __SOC_POWER_DEBUG_H__

/* #define SOC_SLEEP_STATE_GPIO_MARKER_DEBUG */

#ifdef SOC_SLEEP_STATE_GPIO_MARKER_DEBUG
/* Select a gpio not used */
#define DP_GPIO_REG         GPIO_CTRL_REGS->CTRL_0062
#endif

#ifdef DP_GPIO_REG
#define PM_DP_ENTER()       (DP_GPIO_REG = 0x240ul)
#define PM_DP_EXIT()        (DP_GPIO_REG = 0x10240ul)
#else
#define PM_DP_ENTER()
#define PM_DP_EXIT()
#endif

#endif /* __SOC_POWER_DEBUG_H__ */
