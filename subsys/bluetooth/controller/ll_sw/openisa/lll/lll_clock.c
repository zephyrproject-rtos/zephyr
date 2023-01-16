/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>

#include "hal/debug.h"

static uint16_t const sca_ppm_lut[] = {500, 250, 150, 100, 75, 50, 30, 20};

int lll_clock_wait(void)
{
	return 0;
}

uint8_t lll_clock_sca_local_get(void)
{
	return 0;
}

uint32_t lll_clock_ppm_local_get(void)
{
	return sca_ppm_lut[0];
}

uint32_t lll_clock_ppm_get(uint8_t sca)
{
	return sca_ppm_lut[sca];
}
