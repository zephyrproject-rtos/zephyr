/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/dlist.h>
#include <misc/mempool_base.h>

#include "hal/cntr.h"

#include "common/log.h"
#include "hal/debug.h"
#include "nrf_rtc.h"

#ifndef NRF_RTC
#define NRF_RTC NRF_RTC0
#endif

static u8_t _refcount;

void cntr_init(void)
{
	NRF_RTC->PRESCALER = 0;
	nrf_rtc_event_enable(NRF_RTC, RTC_EVTENSET_COMPARE0_Msk |
			     RTC_EVTENSET_COMPARE1_Msk);
	nrf_rtc_int_enable(NRF_RTC, RTC_INTENSET_COMPARE0_Msk |
			     RTC_INTENSET_COMPARE1_Msk);
}

u32_t cntr_start(void)
{
	if (_refcount++) {
		return 1;
	}

	nrf_rtc_task_trigger(NRF_RTC, NRF_RTC_TASK_START);

	return 0;
}

u32_t cntr_stop(void)
{
	LL_ASSERT(_refcount);

	if (--_refcount) {
		return 1;
	}

	nrf_rtc_task_trigger(NRF_RTC, NRF_RTC_TASK_STOP);

	return 0;
}

u32_t cntr_cnt_get(void)
{
	return nrf_rtc_counter_get(NRF_RTC);
}

void cntr_cmp_set(u8_t cmp, u32_t value)
{
	nrf_rtc_cc_set(NRF_RTC, cmp, value);
}
