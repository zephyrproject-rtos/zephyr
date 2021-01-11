/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/dlist.h>

#include <hal/nrf_rtc.h>

#include "hal/cntr.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_hal_cntr
#include "common/log.h"
#include "hal/debug.h"

#ifndef NRF_RTC
#define NRF_RTC NRF_RTC0
#endif

static uint8_t _refcount;

void cntr_init(void)
{
	NRF_RTC->PRESCALER = 0;
	nrf_rtc_event_enable(NRF_RTC, RTC_EVTENSET_COMPARE0_Msk);
	nrf_rtc_int_enable(NRF_RTC, RTC_INTENSET_COMPARE0_Msk);
}

uint32_t cntr_start(void)
{
	if (_refcount++) {
		return 1;
	}

	nrf_rtc_task_trigger(NRF_RTC, NRF_RTC_TASK_START);

	return 0;
}

uint32_t cntr_stop(void)
{
	LL_ASSERT(_refcount);

	if (--_refcount) {
		return 1;
	}

	nrf_rtc_task_trigger(NRF_RTC, NRF_RTC_TASK_STOP);

	return 0;
}

uint32_t cntr_cnt_get(void)
{
	return nrf_rtc_counter_get(NRF_RTC);
}

void cntr_cmp_set(uint8_t cmp, uint32_t value)
{
	nrf_rtc_cc_set(NRF_RTC, cmp, value);
}
