/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <device.h>
#include <drivers/entropy.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>

#include <soc.h>

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

#define LOG_MODULE_NAME bt_ctlr_llsw_nordic_lll
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

static struct {
	struct device *clk_hf;
} lll;

/* Entropy device */
static struct device *dev_entropy;

static int init_reset(void);
static int prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		   lll_prepare_cb_t prepare_cb, int prio,
		   struct lll_prepare_param *prepare_param, u8_t is_resume);
static int resume_enqueue(lll_prepare_cb_t resume_cb, int resume_prio);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
static void ticker_start_op_cb(u32_t status, void *param);
static void preempt_ticker_cb(u32_t ticks_at_expire, u32_t remainder,
			      u16_t lazy, void *param);
static void preempt(void *param);
#else /* CONFIG_BT_CTLR_LOW_LAT */
#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void ticker_op_job_disable(u32_t status, void *op_context);
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

static void rtc0_nrf5_isr(void *arg)
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

static void swi_lll_nrf5_isr(void *arg)
{
	DEBUG_RADIO_ISR(1);

	mayfly_run(TICKER_USER_ID_LLL);

	DEBUG_RADIO_ISR(0);
}

#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void swi_ull_low_nrf5_isr(void *arg)
{
	DEBUG_TICKER_JOB(1);

	mayfly_run(TICKER_USER_ID_ULL_LOW);

	DEBUG_TICKER_JOB(0);
}
#endif

int lll_init(void)
{
	struct device *clk_k32;
	int err;

	/* Get reference to entropy device */
	dev_entropy = device_get_binding(CONFIG_ENTROPY_NAME);
	if (!dev_entropy) {
		return -ENODEV;
	}

	/* Initialise LLL internals */
	event.curr.abort_cb = NULL;

	/* Initialize LF CLK */
	clk_k32 = device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_32K");
	if (!clk_k32) {
		return -ENODEV;
	}

	clock_control_on(clk_k32, NULL);

	/* Initialize HF CLK */
	lll.clk_hf =
		device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_16M");
	if (!lll.clk_hf) {
		return -ENODEV;
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
#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_HIGH_PRIO)
	IRQ_CONNECT(RTC0_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO,
		    rtc0_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#else
	IRQ_CONNECT(RTC0_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO,
		    rtc0_nrf5_isr, NULL, 0);
#endif
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

	return 0;
}

u8_t lll_entropy_get(u8_t len, void *rand)
{
	return entropy_get_entropy_isr(dev_entropy, rand, len, 0);
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
	struct lll_event *next = param;
	int ret;

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
		u8_t idx = UINT8_MAX;

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
	u32_t ret;

	/* Ticker Job Silence */
	ret = ticker_job_idle_get(TICKER_INSTANCE_ID_CTLR,
				  TICKER_USER_ID_LLL,
				  ticker_op_job_disable, NULL);

	return ((ret == TICKER_STATUS_SUCCESS) ||
		(ret == TICKER_STATUS_BUSY)) ? 0 : -EFAULT;
#else
	return 0;
#endif /* CONFIG_BT_CTLR_LOW_LAT */
}

int lll_done(void *param)
{
	struct lll_event *next = ull_prepare_dequeue_get();
	struct ull_hdr *ull = NULL;
	void *evdone;
	int ret = 0;

	/* Assert if param supplied without a pending prepare to cancel. */
	LL_ASSERT(!param || next);

	/* check if current LLL event is done */
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

int lll_clk_on(void)
{
	int err;

	/* turn on radio clock in non-blocking mode. */
	err = clock_control_on(lll.clk_hf, NULL);
	if (!err || err == -EINPROGRESS) {
		DEBUG_RADIO_XTAL(1);
	}

	return err;
}

int lll_clk_on_wait(void)
{
	int err;

	/* turn on radio clock in blocking mode. */
	err = clock_control_on(lll.clk_hf, NULL);

	while (clock_control_get_status(lll.clk_hf, NULL) !=
			CLOCK_CONTROL_STATUS_ON) {
		k_cpu_idle();
	}

	DEBUG_RADIO_XTAL(1);

	return err;
}

int lll_clk_off(void)
{
	int err;

	/* turn off radio clock in non-blocking mode. */
	err = clock_control_off(lll.clk_hf, NULL);
	if (!err) {
		DEBUG_RADIO_XTAL(0);
	} else if (err == -EBUSY) {
		DEBUG_RADIO_XTAL(1);
	}

	return err;
}

u32_t lll_evt_offset_get(struct evt_hdr *evt)
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

u32_t lll_preempt_calc(struct evt_hdr *evt, u8_t ticker_id,
		       u32_t ticks_at_event)
{
	u32_t ticks_now = ticker_ticks_now_get();
	u32_t diff;

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

void lll_chan_set(u32_t chan)
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


u32_t lll_radio_is_idle(void)
{
	return radio_is_idle();
}


static int init_reset(void)
{
	return 0;
}

static int prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		   lll_prepare_cb_t prepare_cb, int prio,
		   struct lll_prepare_param *prepare_param, u8_t is_resume)
{
	u8_t idx = UINT8_MAX;
	struct lll_event *p;
	int ret, err;

	/* Find the ready prepare in the pipeline */
	p = ull_prepare_dequeue_iter(&idx);
	while (p && (p->is_aborted || p->is_resume)) {
		p = ull_prepare_dequeue_iter(&idx);
	}

	/* Current event active or another prepare is ready in the pipeline */
	if (event.curr.abort_cb || (p && is_resume)) {
#if !defined(CONFIG_BT_CTLR_LOW_LAT)
		u32_t preempt_anchor;
		struct evt_hdr *evt;
		u32_t preempt_to;
#else /* CONFIG_BT_CTLR_LOW_LAT */
		lll_prepare_cb_t resume_cb;
		struct lll_event *next;
		int resume_prio;
#endif /* CONFIG_BT_CTLR_LOW_LAT */

		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) && event.curr.param) {
			/* early abort */
			event.curr.abort_cb(NULL, event.curr.param);
		}

		/* Store the next prepare for deferred call */
		ret = ull_prepare_enqueue(is_abort_cb, abort_cb, prepare_param,
					  prepare_cb, prio, is_resume);
		LL_ASSERT(!ret);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
		if (is_resume) {
			return -EINPROGRESS;
		}

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
			ret = event.curr.is_abort_cb(NULL, 0, event.curr.param,
						     &resume_cb, &resume_prio);
			LL_ASSERT(ret);

			if (ret == -EAGAIN) {
				ret = resume_enqueue(resume_cb, resume_prio);
				LL_ASSERT(!ret);
			} else {
				LL_ASSERT(ret == -ECANCELED);
			}
		}
#endif /* CONFIG_BT_CTLR_LOW_LAT */

		return -EINPROGRESS;
	}

	event.curr.param = prepare_param->param;
	event.curr.is_abort_cb = is_abort_cb;
	event.curr.abort_cb = abort_cb;

	err = prepare_cb(prepare_param);

	/* Stop running pre-empt timer, if any */
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_LLL,
			  TICKER_ID_LLL_PREEMPT, NULL, NULL);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_FAILURE) ||
		  (ret == TICKER_STATUS_BUSY));

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

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
static void ticker_start_op_cb(u32_t status, void *param)
{
	/* NOTE: this callback is present only for addition of debug messages
	 * when needed, else can be dispensed with.
	 */
	ARG_UNUSED(param);

	LL_ASSERT((status == TICKER_STATUS_SUCCESS) ||
		  (status == TICKER_STATUS_FAILURE));
}

static void preempt_ticker_cb(u32_t ticks_at_expire, u32_t remainder,
			       u16_t lazy, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, preempt};
	u32_t ret;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);
}

static void preempt(void *param)
{
	struct lll_event *next = ull_prepare_dequeue_get();
	lll_prepare_cb_t resume_cb;
	u8_t idx = UINT8_MAX;
	int resume_prio;
	int ret;

	next = ull_prepare_dequeue_iter(&idx);
	if (!next || !event.curr.abort_cb || !event.curr.param) {
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

		return;
	}

	event.curr.abort_cb(NULL, event.curr.param);

	if (ret == -EAGAIN) {
		struct lll_event *iter;
		u8_t idx = UINT8_MAX;

		iter = ull_prepare_dequeue_iter(&idx);
		while (iter) {
			if (!iter->is_aborted &&
			    event.curr.param == iter->prepare_param.param) {
				iter->is_aborted = 1;
				iter->abort_cb(&iter->prepare_param,
					       iter->prepare_param.param);
			}

			iter = ull_prepare_dequeue_iter(&idx);
		}

		ret = resume_enqueue(resume_cb, resume_prio);
		LL_ASSERT(!ret);
	} else {
		LL_ASSERT(ret == -ECANCELED);
	}
}
#else /* CONFIG_BT_CTLR_LOW_LAT */

#if (CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void ticker_op_job_disable(u32_t status, void *op_context)
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
