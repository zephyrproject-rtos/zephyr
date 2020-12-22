/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <toolchain.h>

#include <soc.h>
#include <device.h>

#include <drivers/entropy.h>

#include "hal/swi.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "hal/ticker.h"

#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "lll.h"
#include "lll_vendor.h"
#include "lll_clock.h"
#include "lll_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_lll
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_CTLR_ZLI)
#define IRQ_CONNECT_FLAGS IRQ_ZERO_LATENCY
#else
#define IRQ_CONNECT_FLAGS 0
#endif

static struct {
	struct {
		void              *param;
		lll_is_abort_cb_t is_abort_cb;
		lll_abort_cb_t    abort_cb;
	} curr;
} event;

/* Entropy device */
static const struct device *dev_entropy;

static int init_reset(void);
static int prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		   lll_prepare_cb_t prepare_cb, int prio,
		   struct lll_prepare_param *prepare_param, uint8_t is_resume);
static int resume_enqueue(lll_prepare_cb_t resume_cb, int resume_prio);
static void isr_race(void *param);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
static void ticker_stop_op_cb(uint32_t status, void *param);
static void ticker_start_op_cb(uint32_t status, void *param);
static void preempt_ticker_start(struct lll_prepare_param *prepare_param);
static void preempt_ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
			      uint16_t lazy, void *param);
static void preempt(void *param);
#else /* CONFIG_BT_CTLR_LOW_LAT */
#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void mfy_ticker_job_idle_get(void *param);
static void ticker_op_job_disable(uint32_t status, void *op_context);
#endif
#endif /* CONFIG_BT_CTLR_LOW_LAT */

ISR_DIRECT_DECLARE(radio_nrf5_isr)
{
	DEBUG_RADIO_ISR(1);

	isr_radio();

	ISR_DIRECT_PM();

	DEBUG_RADIO_ISR(0);
	return 1;
}

static void rtc0_nrf5_isr(const void *arg)
{
	DEBUG_TICKER_ISR(1);

	/* On compare0 run ticker worker instance0 */
	if (NRF_RTC0->EVENTS_COMPARE[0]) {
		NRF_RTC0->EVENTS_COMPARE[0] = 0;

		ticker_trigger(0);
	}

	mayfly_run(TICKER_USER_ID_ULL_HIGH);

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	mayfly_run(TICKER_USER_ID_ULL_LOW);
#endif

	DEBUG_TICKER_ISR(0);
}

static void swi_lll_nrf5_isr(const void *arg)
{
	DEBUG_RADIO_ISR(1);

	mayfly_run(TICKER_USER_ID_LLL);

	DEBUG_RADIO_ISR(0);
}

#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void swi_ull_low_nrf5_isr(const void *arg)
{
	DEBUG_TICKER_JOB(1);

	mayfly_run(TICKER_USER_ID_ULL_LOW);

	DEBUG_TICKER_JOB(0);
}
#endif

int lll_init(void)
{
	int err;

	/* Get reference to entropy device */
	dev_entropy = device_get_binding(DT_LABEL(DT_NODELABEL(rng)));
	if (!dev_entropy) {
		return -ENODEV;
	}

	/* Initialise LLL internals */
	event.curr.abort_cb = NULL;

	/* Initialize Clocks */
	err = lll_clock_init();
	if (err < 0) {
		return err;
	}

	err = init_reset();
	if (err) {
		return err;
	}

	/* Initialize SW IRQ structure */
	hal_swi_init();

	/* Connect ISRs */
	IRQ_DIRECT_CONNECT(RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			   radio_nrf5_isr, IRQ_CONNECT_FLAGS);
	IRQ_CONNECT(RTC0_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO,
		    rtc0_nrf5_isr, NULL, 0);
	IRQ_CONNECT(HAL_SWI_RADIO_IRQ, CONFIG_BT_CTLR_LLL_PRIO,
		    swi_lll_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
	IRQ_CONNECT(HAL_SWI_JOB_IRQ, CONFIG_BT_CTLR_ULL_LOW_PRIO,
		    swi_ull_low_nrf5_isr, NULL, 0);
#endif

	/* Enable IRQs */
	irq_enable(RADIO_IRQn);
	irq_enable(RTC0_IRQn);
	irq_enable(HAL_SWI_RADIO_IRQ);
#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
	irq_enable(HAL_SWI_JOB_IRQ);
#endif

	radio_setup();

	return 0;
}

int lll_csrand_get(void *buf, size_t len)
{
	return entropy_get_entropy(dev_entropy, buf, len);
}

int lll_csrand_isr_get(void *buf, size_t len)
{
	return entropy_get_entropy_isr(dev_entropy, buf, len, 0);
}

int lll_rand_get(void *buf, size_t len)
{
	return entropy_get_entropy(dev_entropy, buf, len);
}

int lll_rand_isr_get(void *buf, size_t len)
{
	return entropy_get_entropy_isr(dev_entropy, buf, len, 0);
}

int lll_reset(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		lll_prepare_cb_t prepare_cb, int prio,
		struct lll_prepare_param *prepare_param)
{
	return prepare(is_abort_cb, abort_cb, prepare_cb, prio, prepare_param,
		       0);
}

void lll_resume(void *param)
{
	struct lll_event *next;
	int ret;

	next = param;
	ret = prepare(next->is_abort_cb, next->abort_cb, next->prepare_cb,
		      next->prio, &next->prepare_param, next->is_resume);
	LL_ASSERT(!ret || ret == -EINPROGRESS);
}

void lll_disable(void *param)
{
	/* LLL disable of current event, done is generated */
	if (!param || (param == event.curr.param)) {
		if (event.curr.abort_cb && event.curr.param) {
			event.curr.abort_cb(NULL, event.curr.param);
		} else {
			LL_ASSERT(!param);
		}
	}
	{
		struct lll_event *next;
		uint8_t idx;

		idx = UINT8_MAX;
		next = ull_prepare_dequeue_iter(&idx);
		while (next) {
			if (!next->is_aborted &&
			    (!param || (param == next->prepare_param.param))) {
				next->is_aborted = 1;
				next->abort_cb(&next->prepare_param,
					       next->prepare_param.param);
			}

			next = ull_prepare_dequeue_iter(&idx);
		}
	}
}

int lll_prepare_done(void *param)
{
#if defined(CONFIG_BT_CTLR_LOW_LAT) && \
	    (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_ticker_job_idle_get};
	uint32_t ret;

	ret = mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_LOW,
			     1, &mfy);
	if (ret) {
		return -EFAULT;
	}

	return 0;
#else
	return 0;
#endif /* CONFIG_BT_CTLR_LOW_LAT */
}

int lll_done(void *param)
{
	struct lll_event *next;
	struct ull_hdr *ull;
	void *evdone;
	int ret = 0;

	/* Assert if param supplied without a pending prepare to cancel. */
	next = ull_prepare_dequeue_get();
	LL_ASSERT(!param || next);

	/* check if current LLL event is done */
	ull = NULL;
	if (!param) {
		/* Reset current event instance */
		LL_ASSERT(event.curr.abort_cb);
		event.curr.abort_cb = NULL;

		param = event.curr.param;
		event.curr.param = NULL;

		if (param) {
			ull = HDR_ULL(((struct lll_hdr *)param)->parent);
		}

		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) &&
		    (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)) {
			mayfly_enable(TICKER_USER_ID_LLL,
				      TICKER_USER_ID_ULL_LOW,
				      1);
		}

		DEBUG_RADIO_CLOSE(0);
	} else {
		ull = HDR_ULL(((struct lll_hdr *)param)->parent);
	}

	/* Let ULL know about LLL event done */
	evdone = ull_event_done(ull);
	LL_ASSERT(evdone);

	return ret;
}

bool lll_is_done(void *param)
{
	/* FIXME: use param to check */
	return !event.curr.abort_cb;
}

int lll_is_abort_cb(void *next, int prio, void *curr,
			 lll_prepare_cb_t *resume_cb, int *resume_prio)
{
	return -ECANCELED;
}

void lll_abort_cb(struct lll_prepare_param *prepare_param, void *param)
{
	int err;

	/* NOTE: This is not a prepare being cancelled */
	if (!prepare_param) {
		/* Perform event abort here.
		 * After event has been cleanly aborted, clean up resources
		 * and dispatch event done.
		 */
		radio_isr_set(lll_isr_done, param);
		radio_disable();
		return;
	}

	/* NOTE: Else clean the top half preparations of the aborted event
	 * currently in preparation pipeline.
	 */
	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	lll_done(param);
}

uint32_t lll_evt_offset_get(struct evt_hdr *evt)
{
	if (0) {
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
	} else if (evt->ticks_xtal_to_start & XON_BITMASK) {
		return MAX(evt->ticks_active_to_start,
			   evt->ticks_preempt_to_start);
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	} else {
		return MAX(evt->ticks_active_to_start,
			   evt->ticks_xtal_to_start);
	}
}

uint32_t lll_preempt_calc(struct evt_hdr *evt, uint8_t ticker_id,
		       uint32_t ticks_at_event)
{
	uint32_t ticks_now;
	uint32_t diff;

	ticks_now = ticker_ticks_now_get();
	diff = ticker_ticks_diff_get(ticks_now, ticks_at_event);
	diff += HAL_TICKER_CNTR_CMP_OFFSET_MIN;
	if (!(diff & BIT(HAL_TICKER_CNTR_MSBIT)) &&
	    (diff > HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US))) {
		/* TODO: for Low Latency Feature with Advanced XTAL feature.
		 * 1. Release retained HF clock.
		 * 2. Advance the radio event to accommodate normal prepare
		 *    duration.
		 * 3. Increase the preempt to start ticks for future events.
		 */
		return 1;
	}

	return 0;
}

void lll_chan_set(uint32_t chan)
{
	switch (chan) {
	case 37:
		radio_freq_chan_set(2);
		break;

	case 38:
		radio_freq_chan_set(26);
		break;

	case 39:
		radio_freq_chan_set(80);
		break;

	default:
		if (chan < 11) {
			radio_freq_chan_set(4 + (chan * 2U));
		} else if (chan < 40) {
			radio_freq_chan_set(28 + ((chan - 11) * 2U));
		} else {
			LL_ASSERT(0);
		}
		break;
	}

	radio_whiten_iv_set(chan);
}


uint32_t lll_radio_is_idle(void)
{
	return radio_is_idle();
}

uint32_t lll_radio_tx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return radio_tx_ready_delay_get(phy, flags);
}

uint32_t lll_radio_rx_ready_delay_get(uint8_t phy, uint8_t flags)
{
	return radio_rx_ready_delay_get(phy, flags);
}

int8_t lll_radio_tx_pwr_min_get(void)
{
	return radio_tx_power_min_get();
}

int8_t lll_radio_tx_pwr_max_get(void)
{
	return radio_tx_power_max_get();
}

int8_t lll_radio_tx_pwr_floor(int8_t tx_pwr_lvl)
{
	return radio_tx_power_floor(tx_pwr_lvl);
}

void lll_isr_tx_status_reset(void)
{
	radio_status_reset();
	radio_tmr_status_reset();

	if (IS_ENABLED(CONFIG_BT_CTLR_GPIO_PA) ||
	    IS_ENABLED(CONFIG_BT_CTLR_GPIO_LNA)) {
		radio_gpio_pa_lna_disable();
	}
}

void lll_isr_rx_status_reset(void)
{
	radio_status_reset();
	radio_tmr_status_reset();
	radio_rssi_status_reset();

	if (IS_ENABLED(CONFIG_BT_CTLR_GPIO_PA) ||
	    IS_ENABLED(CONFIG_BT_CTLR_GPIO_LNA)) {
		radio_gpio_pa_lna_disable();
	}
}

void lll_isr_status_reset(void)
{
	radio_status_reset();
	radio_tmr_status_reset();
	radio_filter_status_reset();
	radio_ar_status_reset();
	radio_rssi_status_reset();

	if (IS_ENABLED(CONFIG_BT_CTLR_GPIO_PA) ||
	    IS_ENABLED(CONFIG_BT_CTLR_GPIO_LNA)) {
		radio_gpio_pa_lna_disable();
	}
}

inline void lll_isr_abort(void *param)
{
	lll_isr_status_reset();
	lll_isr_cleanup(param);
}

void lll_isr_done(void *param)
{
	lll_isr_abort(param);
}

void lll_isr_cleanup(void *param)
{
	int err;

	radio_isr_set(isr_race, param);
	if (!radio_is_idle()) {
		radio_disable();
	}

	radio_tmr_stop();

	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	lll_done(NULL);
}

static int init_reset(void)
{
	return 0;
}

static int prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		   lll_prepare_cb_t prepare_cb, int prio,
		   struct lll_prepare_param *prepare_param, uint8_t is_resume)
{
	struct lll_event *p;
	uint8_t idx;
	int err;

	/* Find the ready prepare in the pipeline */
	idx = UINT8_MAX;
	p = ull_prepare_dequeue_iter(&idx);
	while (p && (p->is_aborted || p->is_resume)) {
		p = ull_prepare_dequeue_iter(&idx);
	}

	/* Current event active or another prepare is ready in the pipeline */
	if (event.curr.abort_cb || (p && is_resume)) {
#if defined(CONFIG_BT_CTLR_LOW_LAT)
		lll_prepare_cb_t resume_cb;
		struct lll_event *next;
		int resume_prio;
#endif /* CONFIG_BT_CTLR_LOW_LAT */

		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) && event.curr.param) {
			/* early abort */
			event.curr.abort_cb(NULL, event.curr.param);
		}

		/* Store the next prepare for deferred call */
		err = ull_prepare_enqueue(is_abort_cb, abort_cb, prepare_param,
					  prepare_cb, prio, is_resume);
		LL_ASSERT(!err);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
		if (is_resume) {
			return -EINPROGRESS;
		}

		preempt_ticker_start(prepare_param);

#else /* CONFIG_BT_CTLR_LOW_LAT */
		next = NULL;
		while (p) {
			if (!p->is_aborted) {
				if (event.curr.param ==
				    p->prepare_param.param) {
					p->is_aborted = 1;
					p->abort_cb(&p->prepare_param,
						    p->prepare_param.param);
				} else {
					next = p;
				}
			}

			p = ull_prepare_dequeue_iter(&idx);
		}

		if (next) {
			/* check if resume requested by curr */
			err = event.curr.is_abort_cb(NULL, 0, event.curr.param,
						     &resume_cb, &resume_prio);
			LL_ASSERT(err);

			if (err == -EAGAIN) {
				err = resume_enqueue(resume_cb, resume_prio);
				LL_ASSERT(!err);
			} else {
				LL_ASSERT(err == -ECANCELED);
			}
		}
#endif /* CONFIG_BT_CTLR_LOW_LAT */

		return -EINPROGRESS;
	}

	event.curr.param = prepare_param->param;
	event.curr.is_abort_cb = is_abort_cb;
	event.curr.abort_cb = abort_cb;

	err = prepare_cb(prepare_param);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
	uint32_t ret;

	/* Stop any scheduled preempt ticker */
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR,
			  TICKER_USER_ID_LLL,
			  TICKER_ID_LLL_PREEMPT,
			  ticker_stop_op_cb, NULL);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_FAILURE) ||
		  (ret == TICKER_STATUS_BUSY));
#endif /* !CONFIG_BT_CTLR_LOW_LAT */

	return err;
}

static int resume_enqueue(lll_prepare_cb_t resume_cb, int resume_prio)
{
	struct lll_prepare_param prepare_param;

	prepare_param.param = event.curr.param;
	event.curr.param = NULL;

	return ull_prepare_enqueue(event.curr.is_abort_cb, event.curr.abort_cb,
				   &prepare_param, resume_cb, resume_prio, 1);
}

static void isr_race(void *param)
{
	/* NOTE: lll_disable could have a race with ... */
	radio_status_reset();
}

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
static void ticker_stop_op_cb(uint32_t status, void *param)
{
	/* NOTE: this callback is present only for addition of debug messages
	 * when needed, else can be dispensed with.
	 */
	ARG_UNUSED(param);

	LL_ASSERT((status == TICKER_STATUS_SUCCESS) ||
		  (status == TICKER_STATUS_FAILURE));
}

static void ticker_start_op_cb(uint32_t status, void *param)
{
	/* NOTE: this callback is present only for addition of debug messages
	 * when needed, else can be dispensed with.
	 */
	ARG_UNUSED(param);

	LL_ASSERT((status == TICKER_STATUS_SUCCESS) ||
		  (status == TICKER_STATUS_FAILURE));
}

static void preempt_ticker_start(struct lll_prepare_param *prepare_param)
{
	uint32_t preempt_anchor;
	struct evt_hdr *evt;
	uint32_t preempt_to;
	uint32_t ret;

	/* Calc the preempt timeout */
	evt = HDR_LLL2EVT(prepare_param->param);
	preempt_anchor = prepare_param->ticks_at_expire;
	preempt_to = MAX(evt->ticks_active_to_start,
			 evt->ticks_xtal_to_start) -
			 evt->ticks_preempt_to_start;

	/* Setup pre empt timeout */
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR,
			   TICKER_USER_ID_LLL,
			   TICKER_ID_LLL_PREEMPT,
			   preempt_anchor,
			   preempt_to,
			   TICKER_NULL_PERIOD,
			   TICKER_NULL_REMAINDER,
			   TICKER_NULL_LAZY,
			   TICKER_NULL_SLOT,
			   preempt_ticker_cb, NULL,
			   ticker_start_op_cb, NULL);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_FAILURE) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void preempt_ticker_cb(uint32_t ticks_at_expire, uint32_t remainder,
			       uint16_t lazy, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, preempt};
	uint32_t ret;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);
}

static void preempt(void *param)
{
	lll_prepare_cb_t resume_cb;
	struct lll_event *next;
	int resume_prio;
	uint8_t idx;
	int ret;

	if (!event.curr.abort_cb || !event.curr.param) {
		return;
	}

	idx = UINT8_MAX;
	next = ull_prepare_dequeue_iter(&idx);
	if (!next) {
		return;
	}

	while (next && (next->is_aborted || next->is_resume)) {
		next = ull_prepare_dequeue_iter(&idx);
	}

	if (!next) {
		return;
	}

	ret = event.curr.is_abort_cb(next->prepare_param.param, next->prio,
				     event.curr.param,
				     &resume_cb, &resume_prio);
	if (!ret) {
		/* Let LLL know about the cancelled prepare */
		next->is_aborted = 1;
		next->abort_cb(&next->prepare_param, next->prepare_param.param);

		goto preempt_next;
	}

	event.curr.abort_cb(NULL, event.curr.param);

	if (ret == -EAGAIN) {
		struct lll_event *iter;
		uint8_t iter_idx;

		iter_idx = UINT8_MAX;
		iter = ull_prepare_dequeue_iter(&iter_idx);
		while (iter) {
			if (!iter->is_aborted &&
			    event.curr.param == iter->prepare_param.param) {
				iter->is_aborted = 1;
				iter->abort_cb(&iter->prepare_param,
					       iter->prepare_param.param);
			}

			iter = ull_prepare_dequeue_iter(&iter_idx);
		}

		ret = resume_enqueue(resume_cb, resume_prio);
		LL_ASSERT(!ret);
	} else {
		LL_ASSERT(ret == -ECANCELED);
	}

preempt_next:
	do {
		next = ull_prepare_dequeue_iter(&idx);
		if (!next) {
			return;
		}
	} while (next->is_aborted || next->is_resume);

	preempt_ticker_start(&next->prepare_param);
}
#else /* CONFIG_BT_CTLR_LOW_LAT */

#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void mfy_ticker_job_idle_get(void *param)
{
	uint32_t ret;

	/* Ticker Job Silence */
	ret = ticker_job_idle_get(TICKER_INSTANCE_ID_CTLR,
				  TICKER_USER_ID_ULL_LOW,
				  ticker_op_job_disable, NULL);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
}

static void ticker_op_job_disable(uint32_t status, void *op_context)
{
	ARG_UNUSED(status);
	ARG_UNUSED(op_context);

	/* FIXME: */
	if (1 /* _radio.state != STATE_NONE */) {
		mayfly_enable(TICKER_USER_ID_ULL_LOW,
			      TICKER_USER_ID_ULL_LOW, 0);
	}
}
#endif

#endif /* CONFIG_BT_CTLR_LOW_LAT */
