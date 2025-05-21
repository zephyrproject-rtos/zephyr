/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>

#include <adsp_power.h>

void z_sys_poweroff(void)
{
        intel_adsp_power_off();

        CODE_UNREACHABLE;
}
