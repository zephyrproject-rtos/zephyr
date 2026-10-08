/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/toolchain.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include "hal/swi.h"
#include "hal/cntr.h"
#include "hal/ticker.h"

#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_internal.h"
#include "lll_tim_internal.h"
#include "lll_radio.h"

#include "hal/debug.h"

/* Range of the transmit power of the radio model, in dBm */
#define TX_PWR_MIN (-40)
#define TX_PWR_MAX 8

#if defined(CONFIG_ENTROPY_HAS_DRIVER)
static const struct device *const dev_entropy = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
#endif /* CONFIG_ENTROPY_HAS_DRIVER */

static void radio_bsim_isr(const void *arg)
{
	DEBUG_RADIO_ISR(1);

	lll_radio_isr();

	DEBUG_RADIO_ISR(0);
}

static void cntr_bsim_isr(const void *arg)
{
	DEBUG_TICKER_ISR(1);

	/* The counter interrupt line is shared with the ULL mayflies */
	if (cntr_cmp_evt_get_clear()) {
		ticker_trigger(0);
	}

	mayfly_run(TICKER_USER_ID_ULL_HIGH);

	if (!IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) &&
	    (CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)) {
		mayfly_run(TICKER_USER_ID_ULL_LOW);
	}

	DEBUG_TICKER_ISR(0);
}

static void swi_lll_bsim_isr(const void *arg)
{
	DEBUG_RADIO_ISR(1);

	mayfly_run(TICKER_USER_ID_LLL);

	DEBUG_RADIO_ISR(0);
}

static void swi_ull_low_bsim_isr(const void *arg)
{
	DEBUG_TICKER_JOB(1);

	mayfly_run(TICKER_USER_ID_ULL_LOW);

	DEBUG_TICKER_JOB(0);
}

static bool is_ull_low_irq_used(void)
{
	return IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) ||
	       (CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO);
}

int lll_init(void)
{
	int err;

#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	if (!device_is_ready(dev_entropy)) {
		return -ENODEV;
	}
#endif /* CONFIG_ENTROPY_HAS_DRIVER */

	lll_prepare_pipeline_init();

	err = lll_clock_init();
	if (err < 0) {
		return err;
	}

	hal_swi_init();

	IRQ_CONNECT(HAL_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO, radio_bsim_isr, NULL, 0);
	IRQ_CONNECT(HAL_CNTR_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO, cntr_bsim_isr, NULL, 0);
	IRQ_CONNECT(HAL_SWI_RADIO_IRQ, CONFIG_BT_CTLR_LLL_PRIO, swi_lll_bsim_isr, NULL, 0);
	if (is_ull_low_irq_used()) {
		IRQ_CONNECT(HAL_SWI_JOB_IRQ, CONFIG_BT_CTLR_ULL_LOW_PRIO, swi_ull_low_bsim_isr,
			    NULL, 0);
	}

	irq_enable(HAL_RADIO_IRQn);
	irq_enable(HAL_CNTR_IRQn);
	irq_enable(HAL_SWI_RADIO_IRQ);
	if (is_ull_low_irq_used()) {
		irq_enable(HAL_SWI_JOB_IRQ);
	}

	lll_radio_init();

	return 0;
}

int lll_deinit(void)
{
	int err;

	err = lll_clock_deinit();
	if (err < 0) {
		return err;
	}

	irq_disable(HAL_RADIO_IRQn);
	irq_disable(HAL_CNTR_IRQn);
	irq_disable(HAL_SWI_RADIO_IRQ);
	if (is_ull_low_irq_used()) {
		irq_disable(HAL_SWI_JOB_IRQ);
	}

	return 0;
}

int lll_reset(void)
{
	return 0;
}

int lll_csrand_get(void *buf, size_t len)
{
#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	return entropy_get_entropy(dev_entropy, buf, len);
#else /* !CONFIG_ENTROPY_HAS_DRIVER */
	return -ENODEV;
#endif /* !CONFIG_ENTROPY_HAS_DRIVER */
}

int lll_csrand_isr_get(void *buf, size_t len)
{
#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	return entropy_get_entropy_isr(dev_entropy, buf, len, 0);
#else /* !CONFIG_ENTROPY_HAS_DRIVER */
	return -ENODEV;
#endif /* !CONFIG_ENTROPY_HAS_DRIVER */
}

int lll_rand_get(void *buf, size_t len)
{
	return lll_csrand_get(buf, len);
}

int lll_rand_isr_get(void *buf, size_t len)
{
	return lll_csrand_isr_get(buf, len);
}

/* The radio model has no ramp up: a radio operation starts with its first
 * bit on air at the requested time.
 */
uint32_t lll_radio_tx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return 0U;
}

uint32_t lll_radio_rx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return 0U;
}

int8_t lll_radio_tx_pwr_min_get(void)
{
	return TX_PWR_MIN;
}

int8_t lll_radio_tx_pwr_max_get(void)
{
	return TX_PWR_MAX;
}

int8_t lll_radio_tx_pwr_floor(int8_t tx_pwr_lvl)
{
	return CLAMP(tx_pwr_lvl, TX_PWR_MIN, TX_PWR_MAX);
}

uint32_t lll_event_start_get(const struct lll_prepare_param *p, uint32_t *ticks_ref)
{
	uint32_t remainder = p->remainder;
	uint32_t ticks;

	ticks = p->ticks_at_expire + HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US) +
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);

	/* Positive remainder in microseconds after the tick */
	hal_ticker_remove_jitter(&ticks, &remainder);

	*ticks_ref = ticks & HAL_TICKER_CNTR_MASK;

	return HAL_TICKER_TICKS_TO_US(*ticks_ref) + remainder;
}

uint32_t lll_preempt_calc(const struct lll_prepare_param *p)
{
	uint32_t ticks_at_event;
	uint32_t diff;

	ticks_at_event = p->ticks_at_expire + HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	diff = ticker_ticks_diff_get(ticker_ticks_now_get(), ticks_at_event);
	if ((diff & BIT(HAL_TICKER_CNTR_MSBIT)) != 0U) {
		return 0U;
	}

	/* The event start must be after now for the radio to accept it */
	diff += HAL_TICKER_CNTR_CMP_OFFSET_MIN;
	if (diff > HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US)) {
		return diff;
	}

	return 0U;
}

void lll_resume_param_set(struct lll_prepare_param *p)
{
	p->ticks_at_expire = ticker_ticks_now_get() -
			     HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	p->remainder = 0U;
	p->lazy = 0U;
}

static void isr_event_abort(const struct bsr_evt *evt, void *param)
{
	ARG_UNUSED(evt);

	lll_isr_cleanup(param);
}

void lll_event_abort(void *param)
{
	lll_radio_stop(isr_event_abort, param);
}

void lll_abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	int err;

	/* An event in progress rather than one in the prepare pipeline */
	if (prepare_param == NULL) {
		lll_event_abort(param);

		return;
	}

	err = lll_hfclock_off();
	LL_ASSERT_ERR(err >= 0);

	lll_done(param);
}

void lll_isr_cleanup(void *param)
{
	int err;

	ARG_UNUSED(param);

	lll_radio_abort();

	err = lll_hfclock_off();
	LL_ASSERT_ERR(err >= 0);

	lll_done(NULL);
}
