/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include "cntr.h"

#include <bluetooth/log.h>
#include "debug.h"

#ifndef NRF_RTC
#define NRF_RTC NRF_RTC0
#endif

static uint8_t _refcount;

void cntr_init(void)
{
	NRF_RTC->PRESCALER = 0;
	NRF_RTC->EVTENSET = (RTC_EVTENSET_COMPARE0_Msk |
			     RTC_EVTENSET_COMPARE1_Msk);
	NRF_RTC->INTENSET = (RTC_INTENSET_COMPARE0_Msk |
			     RTC_INTENSET_COMPARE1_Msk);
}

uint32_t cntr_start(void)
{
	if (_refcount++) {
		return 1;
	}

	NRF_RTC->TASKS_START = 1;

	return 0;
}

uint32_t cntr_stop(void)
{
	LL_ASSERT(_refcount);

	if (--_refcount) {
		return 1;
	}

	NRF_RTC->TASKS_STOP = 1;

	return 0;
}

uint32_t cntr_cnt_get(void)
{
	return NRF_RTC->COUNTER;
}

void cntr_cmp_set(uint8_t cmp, uint32_t value)
{
	NRF_RTC->CC[cmp] = value;
}
