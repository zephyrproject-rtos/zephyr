/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <soc.h>

#include "hal/debug.h"

/* Clock setup timeouts are unlikely, below values are experimental */
#define LFCLOCK_TIMEOUT_MS 500
#define HFCLOCK_TIMEOUT_MS 2

int lll_clock_init(void)
{
	return 0;
}

int lll_clock_wait(void)
{
	return 0;
}

int lll_hfclock_on(void)
{
	return 0;
}

int lll_hfclock_on_wait(void)
{
	return 0;
}

int lll_hfclock_off(void)
{
	return 0;
}

uint8_t lll_clock_sca_local_get(void)
{
	return 0;
}

uint32_t lll_clock_ppm_local_get(void)
{
	return 0;
}

uint32_t lll_clock_ppm_get(uint8_t sca)
{
	ARG_UNUSED(sca);
	return 0;
}
