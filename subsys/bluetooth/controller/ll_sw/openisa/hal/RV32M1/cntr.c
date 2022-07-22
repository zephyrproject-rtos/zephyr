/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/dlist.h>

#include "hal/cntr.h"

#define LOG_MODULE_NAME bt_ctlr_cntr
#include "common/log.h"
#include "hal/debug.h"
#include <zephyr/dt-bindings/interrupt-controller/openisa-intmux.h>
#include "ll_irqs.h"


#define PCS_SOURCE_RTC 2

void cntr_init(void)
{
	/* RTC config */
	/*
	 * Reset all RTC registers except for the SWR bit
	 * and the RTC_WAR and RTC_RAR
	 */
	RTC->CR |= RTC_CR_SWR_MASK;
	RTC->CR &= ~RTC_CR_SWR_MASK;

	/*
	 * Set TSR register to 0x1 to avoid the timer invalid (TIF) bit being
	 * set in the SR register
	 */
	RTC->TSR = 1;

	/* Enable the RTC 32.768 kHz oscillator and clock on RTC_CLKOUT  */
	RTC->CR |= (RTC_CR_CPS(1) | RTC_CR_OSCE(1));

	/* LPTMR config */
	/* Disable the timer and clear any pending IRQ. */
	LPTMR1->CSR = LPTMR_CSR_TEN(0);

	/*
	 * TMS = 0: time counter mode, not pulse counter
	 * TFC = 1: reset counter on overflow
	 * TIE = 1: enable interrupt
	 */
	LPTMR1->CSR = (LPTMR_CSR_TFC(1) | LPTMR_CSR_TIE(1));

	/*
	 * PCS = 2: clock source is RTC - 32 kHz clock (SoC dependent)
	 * PBYP = 1: bypass the prescaler
	 */
	LPTMR1->PSR = (LPTMR_PSR_PBYP(1) | LPTMR_PSR_PCS(PCS_SOURCE_RTC));

	irq_enable(LL_RTC0_IRQn_2nd_lvl);
}

static uint8_t refcount;
static uint32_t cnt_diff;

uint32_t cntr_start(void)
{
	if (refcount++) {
		return 1;
	}

	LPTMR1->CMR = 0xFFFFFFFF;
	LPTMR1->CSR |= LPTMR_CSR_TEN(1);

	return 0;
}

uint32_t cntr_stop(void)
{
	LL_ASSERT(refcount);

	if (--refcount) {
		return 1;
	}

	cnt_diff = cntr_cnt_get();
	/*
	 * When TEN is clear, it resets the LPTMR internal logic,
	 * including the CNR and TCF.
	 */
	LPTMR1->CSR &= ~LPTMR_CSR_TEN_MASK;

	return 0;
}

uint32_t cntr_cnt_get(void)
{
	/*
	 * On each read of the CNR,
	 * software must first write to the CNR with any value.
	 */
	LPTMR1->CNR = 0;
	return (LPTMR1->CNR + cnt_diff);
}

void cntr_cmp_set(uint8_t cmp, uint32_t value)
{
	/*
	 * When the LPTMR is enabled, the first increment will take an
	 * additional one or two prescaler clock cycles due to
	 * synchronization logic.
	 */
	cnt_diff = cntr_cnt_get() + 1;
	LPTMR1->CSR &= ~LPTMR_CSR_TEN_MASK;

	value -= cnt_diff;
	/*
	 * If the CMR is 0, the hardware trigger will remain asserted until
	 * the LPTMR is disabled. If the LPTMR is enabled, the CMR must be
	 * altered only when TCF is set.
	 */
	if (value == 0) {
		value = 1;
	}

	LPTMR1->CMR = value;
	LPTMR1->CSR |= LPTMR_CSR_TEN(1);
}
