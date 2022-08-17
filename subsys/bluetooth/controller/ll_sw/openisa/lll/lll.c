/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/toolchain.h>

#include <soc.h>
#include <zephyr/device.h>

#include <zephyr/drivers/entropy.h>

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
#include "lll_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_llsw_openisa_lll
#include "common/log.h"
#include "hal/debug.h"

static struct {
	struct {
		void              *param;
		lll_is_abort_cb_t is_abort_cb;
		lll_abort_cb_t    abort_cb;
	} curr;

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
	struct {
		uint8_t volatile lll_count;
		uint8_t          ull_count;
	} done;
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
} event;

/* Entropy device */
static const struct device *const dev_entropy = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

static int init_reset(void);
#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
static inline void done_inc(void);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
static struct lll_event *resume_enqueue(lll_prepare_cb_t resume_cb);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
static void ticker_stop_op_cb(uint32_t status, void *param);
static void ticker_start_op_cb(uint32_t status, void *param);
static void ticker_start_next_op_cb(uint32_t status, void *param);
static uint32_t preempt_ticker_start(struct lll_event *event,
				     ticker_op_func op_cb);
static void preempt_ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			      uint32_t remainder, uint16_t lazy, uint8_t force,
			      void *param);
static void preempt(void *param);
#else /* CONFIG_BT_CTLR_LOW_LAT */
#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void mfy_ticker_job_idle_get(void *param);
static void ticker_op_job_disable(uint32_t status, void *op_context);
#endif
#endif /* CONFIG_BT_CTLR_LOW_LAT */

static void rtc0_rv32m1_isr(const void *arg)
{
	DEBUG_TICKER_ISR(1);

	/* On compare0 run ticker worker instance0 */
	if (LPTMR1->CSR & LPTMR_CSR_TCF(1)) {
		LPTMR1->CSR |= LPTMR_CSR_TCF(1);

		ticker_trigger(0);
	}

	mayfly_run(TICKER_USER_ID_ULL_HIGH);

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	mayfly_run(TICKER_USER_ID_ULL_LOW);
#endif

	DEBUG_TICKER_ISR(0);
}

static void swi_lll_rv32m1_isr(const void *arg)
{
	DEBUG_RADIO_ISR(1);

	mayfly_run(TICKER_USER_ID_LLL);

	DEBUG_RADIO_ISR(0);
}

#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void swi_ull_low_rv32m1_isr(const void *arg)
{
	DEBUG_TICKER_JOB(1);

	mayfly_run(TICKER_USER_ID_ULL_LOW);

	DEBUG_TICKER_JOB(0);
}
#endif

int lll_init(void)
{
	int err;

	/* Check if entropy device is ready */
	if (!device_is_ready(dev_entropy)) {
		return -ENODEV;
	}

	/* Initialise LLL internals */
	event.curr.abort_cb = NULL;

	err = init_reset();
	if (err) {
		return err;
	}

	/* Initialize SW IRQ structure */
	hal_swi_init();

	/* Connect ISRs */
	IRQ_CONNECT(LL_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO, isr_radio, NULL, 0);
	IRQ_CONNECT(LL_RTC0_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO,
		    rtc0_rv32m1_isr, NULL, 0);
	IRQ_CONNECT(HAL_SWI_RADIO_IRQ, CONFIG_BT_CTLR_LLL_PRIO,
		    swi_lll_rv32m1_isr, NULL, 0);
#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
	IRQ_CONNECT(HAL_SWI_JOB_IRQ, CONFIG_BT_CTLR_ULL_LOW_PRIO,
		    swi_ull_low_rv32m1_isr, NULL, 0);
#endif

	/* Enable IRQs */
	irq_enable(LL_RADIO_IRQn);
	irq_enable(LL_RTC0_IRQn);
	irq_enable(HAL_SWI_RADIO_IRQ);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) ||
	    (CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)) {
		irq_enable(HAL_SWI_JOB_IRQ);
	}

	/* Call it after IRQ enable to be able to measure ISR latency */
	radio_setup();

	return 0;
}

int lll_deinit(void)
{
	/* Disable IRQs */
	irq_disable(LL_RADIO_IRQn);
	irq_disable(LL_RTC0_IRQn);
	irq_disable(HAL_SWI_RADIO_IRQ);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) ||
	    (CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)) {
		irq_disable(HAL_SWI_JOB_IRQ);
	}

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
	return 0;
}

int lll_rand_isr_get(void *buf, size_t len)
{
	return 0;
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

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
				/* NOTE: abort_cb called lll_done which modifies
				 *       the prepare pipeline hence re-iterate
				 *       through the prepare pipeline.
				 */
				idx = UINT8_MAX;
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
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

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
		done_inc();
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */

		if (param) {
			ull = HDR_LLL2ULL(param);
		}

		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) &&
		    (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)) {
			mayfly_enable(TICKER_USER_ID_LLL,
				      TICKER_USER_ID_ULL_LOW,
				      1);
		}

		DEBUG_RADIO_CLOSE(0);
	} else {
		ull = HDR_LLL2ULL(param);
	}

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
	ull_prepare_dequeue(TICKER_USER_ID_LLL);
#endif /* !CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	lll_done_score(param, 0, 0); /* TODO */
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

	/* Let ULL know about LLL event done */
	evdone = ull_event_done(ull);
	LL_ASSERT(evdone);

	return 0;
}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
void lll_done_ull_inc(void)
{
	LL_ASSERT(event.done.ull_count != event.done.lll_count);
	event.done.ull_count++;
}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */

bool lll_is_done(void *param)
{
	/* FIXME: use param to check */
	return !event.curr.abort_cb;
}

int lll_is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
{
	return -ECANCELED;
}

int lll_clk_on(void)
{
	int err;

	/* turn on radio clock in non-blocking mode. */
	err = radio_wake();
	if (!err) {
		DEBUG_RADIO_XTAL(1);
	}

	return err;
}

int lll_clk_on_wait(void)
{
	int err;

	/* turn on radio clock in blocking mode. */
	err = radio_wake();

	while (radio_is_off()) {
		k_cpu_idle();
	}

	DEBUG_RADIO_XTAL(1);

	return err;
}

int lll_clk_off(void)
{
	int err;

	/* turn off radio clock in non-blocking mode. */
	err = radio_sleep();
	if (!err) {
		DEBUG_RADIO_XTAL(0);
	}

	return err;
}

uint32_t lll_event_offset_get(struct ull_hdr *ull)
{
	if (0) {
#if defined(CONFIG_BT_CTLR_XTAL_ADVANCED)
	} else if (ull->ticks_prepare_to_start & XON_BITMASK) {
		return MAX(ull->ticks_active_to_start,
			   ull->ticks_preempt_to_start);
#endif /* CONFIG_BT_CTLR_XTAL_ADVANCED */
	} else {
		return MAX(ull->ticks_active_to_start,
			   ull->ticks_prepare_to_start);
	}
}

uint32_t lll_preempt_calc(struct ull_hdr *ull, uint8_t ticker_id,
		       uint32_t ticks_at_event)
{
	uint32_t ticks_now;
	uint32_t diff;

	ticks_now = ticker_ticks_now_get();
	diff = ticks_now - ticks_at_event;
	if (diff & BIT(HAL_TICKER_CNTR_MSBIT)) {
		return 0;
	}

	diff += HAL_TICKER_CNTR_CMP_OFFSET_MIN;
	if (diff > HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US)) {
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

void lll_isr_status_reset(void)
{
	radio_status_reset();
	radio_tmr_status_reset();
	radio_filter_status_reset();
	if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY)) {
		radio_ar_status_reset();
	}
	radio_rssi_status_reset();
}

static int init_reset(void)
{
	return 0;
}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
static inline void done_inc(void)
{
	event.done.lll_count++;
	LL_ASSERT(event.done.lll_count != event.done.ull_count);
}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */

static inline bool is_done_sync(void)
{
#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
	return event.done.lll_count == event.done.ull_count;
#else /* !CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
	return true;
#endif /* !CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
}

int lll_prepare_resolve(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
			lll_prepare_cb_t prepare_cb,
			struct lll_prepare_param *prepare_param,
			uint8_t is_resume, uint8_t is_dequeue)
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
	if ((!is_dequeue && !is_done_sync()) ||
	    event.curr.abort_cb ||
	    (p && is_resume)) {
#if defined(CONFIG_BT_CTLR_LOW_LAT)
		lll_prepare_cb_t resume_cb;
#endif /* CONFIG_BT_CTLR_LOW_LAT */
		struct lll_event *next;

		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) && event.curr.param) {
			/* early abort */
			event.curr.abort_cb(NULL, event.curr.param);
		}

		/* Store the next prepare for deferred call */
		next = ull_prepare_enqueue(is_abort_cb, abort_cb, prepare_param,
					   prepare_cb, is_resume);
		LL_ASSERT(next);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
		if (is_resume) {
			return -EINPROGRESS;
		}

		/* Start the preempt timeout */
		uint32_t ret;

		ret  = preempt_ticker_start(next, ticker_start_op_cb);
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));

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
			err = event.curr.is_abort_cb(NULL, event.curr.param,
						     &resume_cb);
			LL_ASSERT(err);

			if (err == -EAGAIN) {
				next = resume_enqueue(resume_cb);
				LL_ASSERT(next);
			} else {
				LL_ASSERT(err == -ECANCELED);
			}
		}
#endif /* CONFIG_BT_CTLR_LOW_LAT */

		return -EINPROGRESS;
	}

	LL_ASSERT(!p || &p->prepare_param == prepare_param);

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

	/* Find next prepare needing preempt timeout to be setup */
	do {
		p = ull_prepare_dequeue_iter(&idx);
		if (!p) {
			return err;
		}
	} while (p->is_aborted || p->is_resume);

	/* Start the preempt timeout */
	ret = preempt_ticker_start(p, ticker_start_next_op_cb);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
#endif /* !CONFIG_BT_CTLR_LOW_LAT */

	return err;
}

static struct lll_event *resume_enqueue(lll_prepare_cb_t resume_cb)
{
	struct lll_prepare_param prepare_param = {0};

	prepare_param.param = event.curr.param;
	event.curr.param = NULL;

	return ull_prepare_enqueue(event.curr.is_abort_cb, event.curr.abort_cb,
				   &prepare_param, resume_cb, 1);
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

static void ticker_start_next_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static uint32_t preempt_ticker_start(struct lll_event *event,
				     ticker_op_func op_cb)
{
	struct lll_prepare_param *p;
	uint32_t preempt_anchor;
	struct ull_hdr *ull;
	uint32_t preempt_to;
	uint32_t ret;

	/* Calc the preempt timeout */
	p = &event->prepare_param;
	ull = HDR_LLL2ULL(p->param);
	preempt_anchor = p->ticks_at_expire;
	preempt_to = MAX(ull->ticks_active_to_start,
			 ull->ticks_prepare_to_start) -
		     ull->ticks_preempt_to_start;

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
			   preempt_ticker_cb, event,
			   op_cb, event);

	return ret;
}

static void preempt_ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			      uint32_t remainder, uint16_t lazy, uint8_t force,
			      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, preempt};
	uint32_t ret;

	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);
}

static void preempt(void *param)
{
	lll_prepare_cb_t resume_cb;
	struct lll_event *next;
	uint8_t idx;
	int err;

	/* No event to abort */
	if (!event.curr.abort_cb || !event.curr.param) {
		return;
	}

	/* Check if any prepare in pipeline */
	idx = UINT8_MAX;
	next = ull_prepare_dequeue_iter(&idx);
	if (!next) {
		return;
	}

	/* Find a prepare that is ready and not a resume */
	while (next && (next->is_aborted || next->is_resume)) {
		next = ull_prepare_dequeue_iter(&idx);
	}

	/* No ready prepare */
	if (!next) {
		return;
	}

	/* Preemptor not in pipeline */
	if (next != param) {
		uint32_t ret;

		/* Start the preempt timeout */
		ret = preempt_ticker_start(next, ticker_start_next_op_cb);
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));

		return;
	}

	/* Check if current event want to continue */
	err = event.curr.is_abort_cb(next->prepare_param.param,
				     event.curr.param,
				     &resume_cb);
	if (!err) {
		/* Let preemptor LLL know about the cancelled prepare */
		next->is_aborted = 1;
		next->abort_cb(&next->prepare_param, next->prepare_param.param);

		return;
	}

	/* Abort the current event */
	event.curr.abort_cb(NULL, event.curr.param);

	/* Check if resume requested */
	if (err == -EAGAIN) {
		struct lll_event *iter;
		uint8_t iter_idx;

		/* Abort any duplicates so that they get dequeued */
		iter_idx = UINT8_MAX;
		iter = ull_prepare_dequeue_iter(&iter_idx);
		while (iter) {
			if (!iter->is_aborted &&
			    event.curr.param == iter->prepare_param.param) {
				iter->is_aborted = 1;
				iter->abort_cb(&iter->prepare_param,
					       iter->prepare_param.param);

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
				/* NOTE: abort_cb called lll_done which modifies
				 *       the prepare pipeline hence re-iterate
				 *       through the prepare pipeline.
				 */
				idx = UINT8_MAX;
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
			}

			iter = ull_prepare_dequeue_iter(&iter_idx);
		}

		/* Enqueue as resume event */
		iter = resume_enqueue(resume_cb);
		LL_ASSERT(iter);
	} else {
		LL_ASSERT(err == -ECANCELED);
	}
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
