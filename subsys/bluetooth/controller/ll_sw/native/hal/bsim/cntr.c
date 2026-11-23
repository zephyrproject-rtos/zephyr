/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/sys/util.h>

#include "hal/cntr.h"
#include "hal/swi.h"

#include "nbr_cntr_if.h"

#include "hal/debug.h"

/* The ticker counter is the low 32 bits of the free running 1MHz counter of
 * the BabbleSim radio model, which can not be stopped, so start and stop only
 * keep a reference count.
 */
static uint8_t refcount;

void cntr_init(void)
{
	nbr_cntr_init(HAL_CNTR_IRQn);
}

uint32_t cntr_start(void)
{
	refcount++;
	if (refcount != 1U) {
		return 1U;
	}

	return 0U;
}

uint32_t cntr_stop(void)
{
	LL_ASSERT_ERR(refcount != 0U);

	refcount--;
	if (refcount != 0U) {
		return 1U;
	}

	return 0U;
}

uint32_t cntr_cnt_get(void)
{
	return (uint32_t)nbr_cntr_get();
}

void cntr_cmp_set(uint8_t cmp, uint32_t value)
{
	uint64_t now = nbr_cntr_get();
	uint32_t delta = value - (uint32_t)now;

	ARG_UNUSED(cmp);

	/* As a 32 bit compare register, match the next time the low 32 bits have
	 * the value, so a value equal to the current one matches after a wrap.
	 */
	nbr_cntr_cmp_set(now + ((delta != 0U) ? delta : BIT64(32)));
}

bool cntr_cmp_evt_get_clear(void)
{
	return nbr_cntr_cmp_evt_get_clear();
}
