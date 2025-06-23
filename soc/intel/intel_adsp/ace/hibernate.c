/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/hibernate.h>
#include <zephyr/pm/pm.h>

#include <adsp_power.h>

void z_sys_hibernate(void)
{
    pm_increment_post_ops_required();

    /*
     * this function performs a context save if enabled and returns
     * during context restore
     */
    intel_adsp_power_off();

    /* perform a system resume */
    pm_system_resume();
}
 