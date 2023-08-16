/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_POWER_DEBUG_H__
#define __SOC_POWER_DEBUG_H__

#ifdef CONFIG_SOC_SLEEP_STATE_DEBUG_FUNCTION

#define PM_DEBUG_ENTER	1
#define PM_DEBUG_EXIT	0

#define PM_DP_ENTER()	pm_debug_function(PM_DEBUG_ENTER)
#define PM_DP_EXIT()	pm_debug_function(PM_DEBUG_EXIT)

#else

#define PM_DP_ENTER()
#define PM_DP_EXIT()

#endif

#endif /* __SOC_POWER_DEBUG_H__ */
