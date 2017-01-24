/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <errno.h>
#include <atomic.h>
#include <device.h>
#include <clock_control.h>
#include <misc/__assert.h>
#include <arch/arm/cortex_m/cmsis.h>

static uint8_t m16src_ref;
static uint8_t m16src_grd;

static int _m16src_start(struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t imask;
	bool blocking;

	/* If the clock is already started then just increment refcount.
	 * If the start and stop don't happen in pairs, a rollover will
	 * be caught and in that case system should assert.
	 */

	/* Test for reference increment from zero and resource guard not taken.
	 */
	imask = irq_lock();

	if (m16src_ref++) {
		irq_unlock(imask);
		goto hf_already_started;
	}

	if (m16src_grd) {
		m16src_ref--;
		irq_unlock(imask);
		return -EAGAIN;
	}

	m16src_grd = 1;

	irq_unlock(imask);

	/* If blocking then spin-wait in CPU sleep until 16MHz clock settles. */
	blocking = POINTER_TO_UINT(sub_system);
	if (blocking) {
		uint32_t intenset;

		irq_disable(POWER_CLOCK_IRQn);

		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		intenset = NRF_CLOCK->INTENSET;
		NRF_CLOCK->INTENSET = CLOCK_INTENSET_HFCLKSTARTED_Msk;

		NRF_CLOCK->TASKS_HFCLKSTART = 1;

		while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
			__WFE();
			__SEV();
			__WFE();
		}

		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		if (!(intenset & CLOCK_INTENSET_HFCLKSTARTED_Msk)) {
			NRF_CLOCK->INTENCLR = CLOCK_INTENCLR_HFCLKSTARTED_Msk;
		}

		NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);

		irq_enable(POWER_CLOCK_IRQn);
	} else {
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;

		NRF_CLOCK->TASKS_HFCLKSTART = 1;
	}

	/* release resource guard */
	m16src_grd = 0;

hf_already_started:
	/* rollover should not happen as start and stop shall be
	 * called in pairs.
	 */
	__ASSERT_NO_MSG(m16src_ref);

	if (NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) {
		return 0;
	} else {
		return -EINPROGRESS;
	}
}

static int _m16src_stop(struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t imask;

	ARG_UNUSED(sub_system);

	/* Test for started resource, if so, decrement reference and acquire
	 * resource guard.
	 */
	imask = irq_lock();

	if (!m16src_ref) {
		irq_unlock(imask);
		return -EALREADY;
	}

	if (--m16src_ref) {
		irq_unlock(imask);
		return 0;
	}

	if (m16src_grd) {
		m16src_ref++;
		irq_unlock(imask);
		return -EAGAIN;
	}

	m16src_grd = 1;

	irq_unlock(imask);

	/* re-entrancy and mult-context safe, and reference count is zero, */

	NRF_CLOCK->TASKS_HFCLKSTOP = 1;

	/* release resource guard */
	m16src_grd = 0;

	return 0;
}

static int _k32src_start(struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t lf_clk_src;
	uint32_t intenset;

	/* TODO: implement the ref count and re-entrancy guard, if a use-case
	 * needs it.
	 */

	if ((NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk)) {
		return 0;
	}

	irq_disable(POWER_CLOCK_IRQn);

	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

	intenset = NRF_CLOCK->INTENSET;
	NRF_CLOCK->INTENSET = CLOCK_INTENSET_LFCLKSTARTED_Msk;

	/* Set LF Clock Source */
	lf_clk_src = POINTER_TO_UINT(sub_system);
	NRF_CLOCK->LFCLKSRC = lf_clk_src;

	/* Start and spin-wait until clock settles */
	NRF_CLOCK->TASKS_LFCLKSTART = 1;

	while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0) {
		__WFE();
		__SEV();
		__WFE();
	}

	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

	if (!(intenset & CLOCK_INTENSET_LFCLKSTARTED_Msk)) {
		NRF_CLOCK->INTENCLR = CLOCK_INTENCLR_LFCLKSTARTED_Msk;
	}

	NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);

	irq_enable(POWER_CLOCK_IRQn);

	/* If RC selected, calibrate and start timer for consecutive
	 * calibrations.
	 */
	NRF_CLOCK->INTENCLR = CLOCK_INTENCLR_DONE_Msk | CLOCK_INTENCLR_CTTO_Msk;
	NRF_CLOCK->EVENTS_DONE = 0;
	NRF_CLOCK->EVENTS_CTTO = 0;

	if ((lf_clk_src & CLOCK_LFCLKSRC_SRC_Msk) == CLOCK_LFCLKSRC_SRC_RC) {
		int err;

		/* Set the Calibration Timer Initial Value */
		NRF_CLOCK->CTIV = 16;	/* 4s in 0.25s units */

		/* Enable DONE and CTTO IRQs */
		NRF_CLOCK->INTENSET =
		    CLOCK_INTENSET_DONE_Msk | CLOCK_INTENSET_CTTO_Msk;

		/* Start HF clock, if already started then explicitly
		 * assert IRQ.
		 * NOTE: The INTENSET is used as state flag to start
		 * calibration in ISR.
		 */
		NRF_CLOCK->INTENSET = CLOCK_INTENSET_HFCLKSTARTED_Msk;

		err = _m16src_start(dev, false);
		if (!err) {
			NVIC_SetPendingIRQ(POWER_CLOCK_IRQn);
		} else {
			__ASSERT_NO_MSG(err == -EINPROGRESS);
		}
	}

	return !(NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk);
}

static void _power_clock_isr(void *arg)
{
	uint8_t pof, hf_intenset, hf_stat, hf, lf, done, ctto;
	struct device *dev = arg;

	pof = (NRF_POWER->EVENTS_POFWARN != 0);

	hf_intenset =
	    ((NRF_CLOCK->INTENSET & CLOCK_INTENSET_HFCLKSTARTED_Msk) != 0);
	hf_stat = ((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) != 0);
	hf = (NRF_CLOCK->EVENTS_HFCLKSTARTED != 0);

	lf = (NRF_CLOCK->EVENTS_LFCLKSTARTED != 0);

	done = (NRF_CLOCK->EVENTS_DONE != 0);
	ctto = (NRF_CLOCK->EVENTS_CTTO != 0);

	__ASSERT_NO_MSG(pof || hf || hf_intenset || lf || done || ctto);

	if (pof) {
		NRF_POWER->EVENTS_POFWARN = 0;
	}

	if (hf) {
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	}

	if (hf_intenset && hf_stat) {
		/* INTENSET is used as state flag to start calibration,
		 * hence clear it here.
		 */
		NRF_CLOCK->INTENCLR = CLOCK_INTENCLR_HFCLKSTARTED_Msk;

		/* Start Calibration */
		NRF_CLOCK->TASKS_CAL = 1;
	}

	if (lf) {
		NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;

		__ASSERT_NO_MSG(0);
	}

	if (done) {
		int err;

		NRF_CLOCK->EVENTS_DONE = 0;

		/* Calibration done, stop 16M Xtal. */
		err = _m16src_stop(dev, NULL);
		__ASSERT_NO_MSG(!err);

		/* Start timer for next calibration. */
		NRF_CLOCK->TASKS_CTSTART = 1;
	}

	if (ctto) {
		int err;

		NRF_CLOCK->EVENTS_CTTO = 0;

		/* Start HF clock, if already started
		 * then explicitly assert IRQ; we use the INTENSET
		 * as a state flag to start calibration.
		 */
		NRF_CLOCK->INTENSET = CLOCK_INTENSET_HFCLKSTARTED_Msk;

		err = _m16src_start(dev, false);
		if (!err) {
			NVIC_SetPendingIRQ(POWER_CLOCK_IRQn);
		} else {
			__ASSERT_NO_MSG(err == -EINPROGRESS);
		}
	}
}

static int _clock_control_init(struct device *dev)
{
	/* TODO: Initialization will be called twice, once for 32KHz and then
	 * for 16 MHz clock. The vector is also shared for other power related
	 * features. Hence, design a better way to init IRQISR when adding
	 * power peripheral driver and/or new SoC series.
	 * NOTE: Currently the operations here are idempotent.
	 */
	IRQ_CONNECT(NRF5_IRQ_POWER_CLOCK_IRQn,
		    CONFIG_CLOCK_CONTROL_NRF5_IRQ_PRIORITY,
		    _power_clock_isr, 0, 0);

	irq_enable(POWER_CLOCK_IRQn);

	return 0;
}

static const struct clock_control_driver_api _m16src_clock_control_api = {
	.on = _m16src_start,
	.off = _m16src_stop,
	.get_rate = NULL,
};

DEVICE_AND_API_INIT(clock_nrf5_m16src,
		    CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME,
		    _clock_control_init, NULL, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &_m16src_clock_control_api);

static const struct clock_control_driver_api _k32src_clock_control_api = {
	.on = _k32src_start,
	.off = NULL,
	.get_rate = NULL,
};

DEVICE_AND_API_INIT(clock_nrf5_k32src,
		    CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME,
		    _clock_control_init, NULL, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &_k32src_clock_control_api);
