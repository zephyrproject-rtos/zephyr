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

#include <soc.h>

#include "clock.h"

#include "debug.h"

static inline void hal_nop(void)
{
	__asm__ volatile ("mov r0,r0");
}

#if defined(__GNUC__)
#define NOP() hal_nop()
#else
#define NOP __nop
#endif

static struct {
	uint8_t m16src_refcount;
} clock_instance;

uint32_t clock_m16src_start(uint32_t async)
{
	/* if clock is already started then just increment refcount.
	 * refcount can handle 255 (uint8_t) requests, if the start
	 * and stop dont happen in pairs, a rollover will be caught
	 * and system should assert.
	 */
	if (clock_instance.m16src_refcount++) {
		goto hf_start_return;
	}

	DEBUG_RADIO_XTAL(1);

	NRF_CLOCK->TASKS_HFCLKSTOP = 1;

	if (!async) {
		uint32_t intenset;

		irq_disable(POWER_CLOCK_IRQn);

		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		intenset = NRF_CLOCK->INTENSET;
		NRF_CLOCK->INTENSET = CLOCK_INTENSET_HFCLKSTARTED_Msk;

		NRF_CLOCK->TASKS_HFCLKSTART = 1;
		NOP();
		NOP();
		NOP();
		NOP();

		while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
			__WFE();
		}
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		if (!(intenset & CLOCK_INTENSET_HFCLKSTARTED_Msk)) {
			NRF_CLOCK->INTENCLR = CLOCK_INTENCLR_HFCLKSTARTED_Msk;
		}

		_NvicIrqUnpend(POWER_CLOCK_IRQn);
		irq_enable(POWER_CLOCK_IRQn);
	} else {
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
		NRF_CLOCK->TASKS_HFCLKSTART = 1;
		NOP();
		NOP();
		NOP();
		NOP();
	}

hf_start_return:

	/* rollover should not happen as start and stop shall be
	 * called in pairs.
	 */
	BT_ASSERT(clock_instance.m16src_refcount);

	return (((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk)) ? 1 : 0);
}

void clock_m16src_stop(void)
{
	BT_ASSERT(clock_instance.m16src_refcount);

	if (--clock_instance.m16src_refcount) {
		return;
	}

	DEBUG_RADIO_XTAL(0);

	NRF_CLOCK->TASKS_HFCLKSTOP = 1;
}

uint32_t clock_k32src_start(uint32_t src)
{
	uint32_t intenset;

	if ((NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk)) {
		return 1;
	}

	NRF_CLOCK->TASKS_LFCLKSTOP = 1;

	irq_disable(POWER_CLOCK_IRQn);

	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

	intenset = NRF_CLOCK->INTENSET;
	NRF_CLOCK->INTENSET = CLOCK_INTENSET_LFCLKSTARTED_Msk;

	NRF_CLOCK->LFCLKSRC = src;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;

	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {
		__WFE();
	}
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

	if (!(intenset & CLOCK_INTENSET_LFCLKSTARTED_Msk)) {
		NRF_CLOCK->INTENCLR = CLOCK_INTENCLR_LFCLKSTARTED_Msk;
	}

	_NvicIrqUnpend(POWER_CLOCK_IRQn);
	irq_enable(POWER_CLOCK_IRQn);

	/* Calibrate RC, and start timer for consecutive calibrations */
	NRF_CLOCK->TASKS_CTSTOP = 1;
	NRF_CLOCK->INTENCLR = CLOCK_INTENCLR_DONE_Msk | CLOCK_INTENCLR_CTTO_Msk;
	NRF_CLOCK->EVENTS_DONE = 0;
	NRF_CLOCK->EVENTS_CTTO = 0;
	if (!src) {
		/* Set the Calibration Timer initial value */
		NRF_CLOCK->CTIV = 16;	/* 4s in 0.25s units */

		/* Enable DONE and CTTO IRQs */
		NRF_CLOCK->INTENSET =
		    CLOCK_INTENSET_DONE_Msk | CLOCK_INTENSET_CTTO_Msk;

		/* Start HF clock, if already started then explicitly
		 * assert IRQ
		 */
		NRF_CLOCK->INTENSET = CLOCK_INTENSET_HFCLKSTARTED_Msk;
		if (clock_m16src_start(1)) {
			_NvicIrqUnpend(POWER_CLOCK_IRQn);
		}
	}

	return ((NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk)) ? 1 : 0;
}

void power_clock_isr(void)
{
	uint8_t pof, hf_intenset, hf_stat, hf, lf, done, ctto;

	pof = (NRF_POWER->EVENTS_POFWARN != 0);

	hf_intenset =
	    ((NRF_CLOCK->INTENSET & CLOCK_INTENSET_HFCLKSTARTED_Msk) != 0);
	hf_stat = ((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) != 0);
	hf = (NRF_CLOCK->EVENTS_HFCLKSTARTED != 0);

	lf = (NRF_CLOCK->EVENTS_LFCLKSTARTED != 0);

	done = (NRF_CLOCK->EVENTS_DONE != 0);
	ctto = (NRF_CLOCK->EVENTS_CTTO != 0);

	BT_ASSERT(pof || hf || lf || done || ctto);

	if (pof) {
		NRF_POWER->EVENTS_POFWARN = 0;
	}

	if (hf) {
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	}

	if (hf_intenset && hf_stat) {
		NRF_CLOCK->INTENCLR = CLOCK_INTENCLR_HFCLKSTARTED_Msk;

		/* Start Calibration */
		NRF_CLOCK->TASKS_CAL = 1;
	}

	if (lf) {
		NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

		BT_ASSERT(0);
	}

	if (done) {
		NRF_CLOCK->EVENTS_DONE = 0;

		/* Calibration done, stop 16M Xtal. */
		clock_m16src_stop();

		/* Start timer for next calibration. */
		NRF_CLOCK->TASKS_CTSTART = 1;
	}

	if (ctto) {
		NRF_CLOCK->EVENTS_CTTO = 0;

		/* Start HF clock, if already started
		 * then explicitly assert IRQ
		 */
		NRF_CLOCK->INTENSET = CLOCK_INTENSET_HFCLKSTARTED_Msk;
		if (clock_m16src_start(1)) {
			_NvicIrqUnpend(POWER_CLOCK_IRQn);
		}
	}
}
