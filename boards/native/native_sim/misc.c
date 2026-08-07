/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "posix_native_task.h"
#include "nsi_timer_model.h"

#if defined(CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME)

static void set_realtime_default(void)
{
	hwtimer_set_real_time_mode(true);
}

NATIVE_TASK(set_realtime_default, PRE_BOOT_1, 0);

#endif /* CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME */
