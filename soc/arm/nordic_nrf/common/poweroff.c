/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <hal/nrf_power.h>

void z_sys_poweroff(void)
{
	nrf_power_system_off(NRF_POWER);

	CODE_UNREACHABLE;
}
