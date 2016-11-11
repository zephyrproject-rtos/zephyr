/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nrf.h"

#include "debug.h"

/* RTC instance to use */
#define NRF_RTC NRF_RTC0

static uint8_t _rtc_refcount;

void rtc_init(void)
{
	NRF_RTC->PRESCALER = 0;
	NRF_RTC->EVTENSET =
	    (RTC_EVTENSET_COMPARE0_Msk | RTC_EVTENSET_COMPARE1_Msk);
	NRF_RTC->INTENSET =
	    (RTC_INTENSET_COMPARE0_Msk | RTC_INTENSET_COMPARE1_Msk);
}

uint32_t rtc_start(void)
{
	if (_rtc_refcount++) {
		return 1;
	}

	NRF_RTC->TASKS_START = 1;

	return 0;
}

uint32_t rtc_stop(void)
{
	LL_ASSERT(_rtc_refcount);

	if (--_rtc_refcount) {
		return 1;
	}

	NRF_RTC->TASKS_STOP = 1;

	return 0;
}

uint32_t rtc_tick_get(void)
{
	return NRF_RTC->COUNTER;
}

void rtc_compare_set(uint8_t instance, uint32_t value)
{
	NRF_RTC->CC[instance] = value;
}
