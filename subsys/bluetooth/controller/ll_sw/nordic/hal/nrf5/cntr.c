/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hal/nrf_rtc.h>

#include "hal/cntr.h"

#include "hal/debug.h"

#if !defined(CONFIG_BT_CTLR_NRF_GRTC)
#ifndef NRF_RTC
#if defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
#define NRF_RTC NRF_RTC10
#else /* !CONFIG_SOC_COMPATIBLE_NRF54LX */
#define NRF_RTC NRF_RTC0
#endif /* !CONFIG_SOC_COMPATIBLE_NRF54LX */
#endif /* !NRF_RTC */
#endif /* CONFIG_BT_CTLR_NRF_GRTC */

static uint8_t _refcount;

void cntr_init(void)
{
#if defined(CONFIG_BT_CTLR_NRF_GRTC)
#if defined(CONFIG_BT_CTLR_NRF_GRTC_START)
	NRF_GRTC->MODE = (GRTC_MODE_SYSCOUNTEREN_Disabled <<
			  GRTC_MODE_SYSCOUNTEREN_Pos) &
			 GRTC_MODE_SYSCOUNTEREN_Msk;

	NRF_GRTC->TASKS_CLEAR = 1U;

#if defined(CONFIG_BT_CTLR_NRF_GRTC_KEEPRUNNING)
	NRF_GRTC->KEEPRUNNING =
		(GRTC_KEEPRUNNING_REQUEST1_Active <<
		 GRTC_KEEPRUNNING_REQUEST1_Pos) &
		GRTC_KEEPRUNNING_REQUEST1_Msk;
	NRF_GRTC->TIMEOUT = 0U;
	NRF_GRTC->INTERVAL = 0U;
	NRF_GRTC->WAKETIME = 4U;
#endif /* CONFIG_BT_CTLR_NRF_GRTC_KEEPRUNNING */

	NRF_GRTC->CLKCFG = ((GRTC_CLKCFG_CLKSEL_LFXO <<
			     GRTC_CLKCFG_CLKSEL_Pos) &
			    GRTC_CLKCFG_CLKSEL_Msk) |
			   ((GRTC_CLKCFG_CLKFASTDIV_Min <<
			     GRTC_CLKCFG_CLKFASTDIV_Pos) &
			    GRTC_CLKCFG_CLKFASTDIV_Msk);
#endif /* CONFIG_BT_CTLR_NRF_GRTC_START */

	NRF_GRTC->EVENTS_COMPARE[10] = 0U;
	NRF_GRTC->INTENSET1 = GRTC_INTENSET1_COMPARE10_Msk;

#if defined(CONFIG_BT_CTLR_NRF_GRTC_START)
	NRF_GRTC->MODE = ((GRTC_MODE_SYSCOUNTEREN_Enabled <<
			   GRTC_MODE_SYSCOUNTEREN_Pos) &
			  GRTC_MODE_SYSCOUNTEREN_Msk) |
#if defined(CONFIG_BT_CTLR_NRF_GRTC_AUTOEN_CPUACTIVE)
			 ((GRTC_MODE_AUTOEN_CpuActive <<
			   GRTC_MODE_AUTOEN_Pos) &
			  GRTC_MODE_AUTOEN_Msk) |
#endif /* CONFIG_BT_CTLR_NRF_GRTC_AUTOEN_CPUACTIVE */
#if defined(CONFIG_BT_CTLR_NRF_GRTC_AUTOEN_DEFAULT)
			 ((GRTC_MODE_AUTOEN_Default <<
			   GRTC_MODE_AUTOEN_Pos) &
			  GRTC_MODE_AUTOEN_Msk) |
#endif /* CONFIG_BT_CTLR_NRF_GRTC_AUTOEN_DEFAULT */
			 0U;

	NRF_GRTC->TASKS_START = 1U;
#endif /* CONFIG_BT_CTLR_NRF_GRTC_START */

#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	NRF_RTC->PRESCALER = 0;
	nrf_rtc_event_enable(NRF_RTC, RTC_EVTENSET_COMPARE0_Msk);
	nrf_rtc_int_enable(NRF_RTC, RTC_INTENSET_COMPARE0_Msk);
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */
}

uint32_t cntr_start(void)
{
	if (_refcount++) {
		return 1;
	}

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	/* TODO: if we own and start GRTC, implement start here */
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	nrf_rtc_task_trigger(NRF_RTC, NRF_RTC_TASK_START);
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */

	return 0;
}

uint32_t cntr_stop(void)
{
	LL_ASSERT(_refcount);

	if (--_refcount) {
		return 1;
	}

#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	/* TODO: if we own and stop GRTC, implement stop here */
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	nrf_rtc_task_trigger(NRF_RTC, NRF_RTC_TASK_STOP);
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */

	return 0;
}

uint32_t cntr_cnt_get(void)
{
#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	uint32_t l, h, ho;

	/* NOTE: For a 32-bit implementation, L value is read after H
	 *       to avoid another L value after SYSCOUNTER gets ready.
	 *       If both H and L values are desired, then swap the order and
	 *       ensure that L value does not change when H value is read.
	 */
	do {
		h = NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERH;
		l = NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERL;
		ho = NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERH;
	} while ((h & GRTC_SYSCOUNTER_SYSCOUNTERH_BUSY_Msk) ||
		 (ho & GRTC_SYSCOUNTER_SYSCOUNTERH_OVERFLOW_Msk));

	return l;
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	return nrf_rtc_counter_get(NRF_RTC);
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */
}

void cntr_cmp_set(uint8_t cmp, uint32_t value)
{
#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	uint32_t l, h, ho, stale;

	/* NOTE: We are going to use TASKS_CAPTURE to read current
	 *       SYSCOUNTER H and L, so that COMPARE registers can be set
	 *       considering that we need to set H compare value too.
	 */

	/* Read current syscounter value */
	do {
		h = NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERH;
		l = NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERL;
		ho = NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERH;
	} while ((h & GRTC_SYSCOUNTER_SYSCOUNTERH_BUSY_Msk) ||
		 (ho & GRTC_SYSCOUNTER_SYSCOUNTERH_OVERFLOW_Msk));

	/* Disable capture/compare */
	NRF_GRTC->CC[cmp].CCEN = 0U;

	/* Set a stale value in capture value */
	stale = l - 1U;
	NRF_GRTC->CC[cmp].CCL = stale;

	/* Trigger a capture */
	NRF_GRTC->TASKS_CAPTURE[cmp] = 1U;

	/* Wait to get a new L value */
	do {
		l = NRF_GRTC->CC[cmp].CCL;
	} while (l == stale);

	/* Read H value */
	h = NRF_GRTC->CC[cmp].CCH;

	/* NOTE: HERE, we have h and l in sync. */

	/* Handle rollover between current and expected value */
	if (value < l) {
		h++;
	}

	/* Set compare register values */
	NRF_GRTC->CC[cmp].CCL = value;
	NRF_GRTC->CC[cmp].CCH = h & GRTC_CC_CCH_CCH_Msk;

	/* Enable compare */
	NRF_GRTC->CC[cmp].CCEN = 1U;

#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	nrf_rtc_cc_set(NRF_RTC, cmp, value);
#endif /* !CONFIG_BT_CTLR_NRF_GRTC */
}
