/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "lll_clock.h"

#include "hal/debug.h"

/* The simulated clocks are always running, and the sleep clock accuracy is
 * modelled by the BabbleSim device clock drift settings.
 */
#define LLL_CLOCK_SCA_LOCAL 5U /* 31 to 50 ppm */

static uint16_t const sca_ppm_lut[] = { 500, 250, 150, 100, 75, 50, 30, 20 };

static atomic_val_t hf_refcnt;

int lll_clock_init(void)
{
	return 0;
}

int lll_clock_deinit(void)
{
	return 0;
}

int lll_clock_wait(void)
{
	return 0;
}

int lll_hfclock_on(void)
{
	if (atomic_inc(&hf_refcnt) > 0) {
		return 0;
	}

	DEBUG_RADIO_XTAL(1);

	return 0;
}

int lll_hfclock_on_wait(void)
{
	(void)atomic_inc(&hf_refcnt);

	DEBUG_RADIO_XTAL(1);

	return 0;
}

int lll_hfclock_off(void)
{
	if (atomic_get(&hf_refcnt) < 1) {
		return -EALREADY;
	}

	if (atomic_dec(&hf_refcnt) > 1) {
		return 0;
	}

	DEBUG_RADIO_XTAL(0);

	return 0;
}

uint8_t lll_clock_sca_local_get(void)
{
	return LLL_CLOCK_SCA_LOCAL;
}

uint32_t lll_clock_ppm_local_get(void)
{
	return sca_ppm_lut[LLL_CLOCK_SCA_LOCAL];
}

uint32_t lll_clock_ppm_get(uint8_t sca)
{
	return sca_ppm_lut[sca];
}
