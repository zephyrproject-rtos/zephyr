/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/poweroff.h>

#include <adsp_power.h>

void z_sys_poweroff(void)
{
        intel_adsp_power_off();

/* if IMR restore is enabled, we should perform a system resume */
#ifdef CONFIG_ADSP_IMR_CONTEXT_SAVE
        pm_system_resume();
#endif
}
