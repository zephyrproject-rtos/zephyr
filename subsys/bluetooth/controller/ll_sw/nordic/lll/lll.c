/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
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
#include <zephyr/irq.h>

#include "hal/swi.h"
#include "hal/ccm.h"
#include "hal/cntr.h"
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
#include "lll_prof_internal.h"

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

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
	struct {
		uint8_t volatile lll_count;
		uint8_t          ull_count;
	} done;
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
} event;

/* Entropy device */
#if defined(CONFIG_ENTROPY_HAS_DRIVER)
/* FIXME: This could probably use a chosen entropy device instead on relying on
 * the nodelabel being the same as for the old nrf rng.
 */
static const struct device *const dev_entropy = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
#endif /* CONFIG_ENTROPY_HAS_DRIVER */

static int init_reset(void);
#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
static inline void done_inc(void);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
static inline bool is_done_sync(void);
static inline struct lll_event *prepare_dequeue_iter_ready_get(uint8_t *idx);
static inline struct lll_event *resume_enqueue(lll_prepare_cb_t resume_cb);
static void isr_race(void *param);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
static uint32_t preempt_ticker_start(struct lll_event *first,
				     struct lll_event *prev,
				     struct lll_event *next);
static uint32_t preempt_ticker_stop(void);
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

#if defined(CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS) && \
	defined(CONFIG_DYNAMIC_DIRECT_INTERRUPTS)
static void radio_nrf5_isr(const void *arg)
#else /* !CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */
ISR_DIRECT_DECLARE(radio_nrf5_isr)
#endif /* !CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */
{
	DEBUG_RADIO_ISR(1);

	lll_prof_enter_radio();

	isr_radio();

	ISR_DIRECT_PM();

	lll_prof_exit_radio();

	DEBUG_RADIO_ISR(0);

#if !defined(CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS) || \
	!defined(CONFIG_DYNAMIC_DIRECT_INTERRUPTS)
	return 1;
#endif /* !CONFIG_DYNAMIC_DIRECT_INTERRUPTS */
}

#if defined(CONFIG_BT_CTLR_RADIO_TIMER_ISR)
#if defined(CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS) && \
	defined(CONFIG_DYNAMIC_DIRECT_INTERRUPTS)
static void timer_nrf5_isr(const void *arg)
#else /* !CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */
ISR_DIRECT_DECLARE(timer_nrf5_isr)
#endif /* !CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */
{
	DEBUG_RADIO_ISR(1);

	lll_prof_enter_radio();

	isr_radio_tmr();

	ISR_DIRECT_PM();

	lll_prof_exit_radio();

	DEBUG_RADIO_ISR(0);

#if !defined(CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS) || \
	!defined(CONFIG_DYNAMIC_DIRECT_INTERRUPTS)
	return 1;
#endif /* !CONFIG_DYNAMIC_DIRECT_INTERRUPTS */
}
#endif /* CONFIG_BT_CTLR_RADIO_TIMER_ISR */

static void rtc0_nrf5_isr(const void *arg)
{
	DEBUG_TICKER_ISR(1);

	lll_prof_enter_ull_high();

	/* On compare0 run ticker worker instance0 */
#if defined(CONFIG_BT_CTLR_NRF_GRTC)
	if (NRF_GRTC->EVENTS_COMPARE[HAL_CNTR_GRTC_CC_IDX_TICKER]) {
		nrf_grtc_event_clear(NRF_GRTC, HAL_CNTR_GRTC_EVENT_COMPARE_TICKER);
#else /* !CONFIG_BT_CTLR_NRF_GRTC */
	if (NRF_RTC->EVENTS_COMPARE[0]) {
		nrf_rtc_event_clear(NRF_RTC, NRF_RTC_EVENT_COMPARE_0);
#endif  /* !CONFIG_BT_CTLR_NRF_GRTC */

		ticker_trigger(0);
	}

	mayfly_run(TICKER_USER_ID_ULL_HIGH);

	lll_prof_exit_ull_high();

#if !defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
	lll_prof_enter_ull_low();

	mayfly_run(TICKER_USER_ID_ULL_LOW);

	lll_prof_exit_ull_low();
#endif

	DEBUG_TICKER_ISR(0);
}

static void swi_lll_nrf5_isr(const void *arg)
{
	DEBUG_RADIO_ISR(1);

	lll_prof_enter_lll();

	mayfly_run(TICKER_USER_ID_LLL);

	lll_prof_exit_lll();

	DEBUG_RADIO_ISR(0);
}

#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
static void swi_ull_low_nrf5_isr(const void *arg)
{
	DEBUG_TICKER_JOB(1);

	lll_prof_enter_ull_low();

	mayfly_run(TICKER_USER_ID_ULL_LOW);

	lll_prof_exit_ull_low();

	DEBUG_TICKER_JOB(0);
}
#endif

int lll_init(void)
{
	int err;

#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	/* Get reference to entropy device */
	if (!device_is_ready(dev_entropy)) {
		return -ENODEV;
	}
#endif /* CONFIG_ENTROPY_HAS_DRIVER */

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

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		err = radio_gpio_pa_lna_init();
		if (err) {
			return err;
		}
	}

	/* Initialize SW IRQ structure */
	hal_swi_init();

	/* Connect ISRs */
#if defined(CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS)
#if defined(CONFIG_DYNAMIC_DIRECT_INTERRUPTS)
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(HAL_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
				       IRQ_CONNECT_FLAGS, no_reschedule);
	irq_connect_dynamic(HAL_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			    radio_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#if defined(CONFIG_BT_CTLR_RADIO_TIMER_ISR)
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(TIMER0_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
				       IRQ_CONNECT_FLAGS, no_reschedule);
	irq_connect_dynamic(TIMER0_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			    timer_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#endif /* CONFIG_BT_CTLR_RADIO_TIMER_ISR */
#else /* !CONFIG_DYNAMIC_DIRECT_INTERRUPTS */
	IRQ_DIRECT_CONNECT(HAL_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			   radio_nrf5_isr, IRQ_CONNECT_FLAGS);
#if defined(CONFIG_BT_CTLR_RADIO_TIMER_ISR)
	IRQ_DIRECT_CONNECT(TIMER0_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			   timer_nrf5_isr, IRQ_CONNECT_FLAGS);
#endif /* CONFIG_BT_CTLR_RADIO_TIMER_ISR */
#endif /* !CONFIG_DYNAMIC_DIRECT_INTERRUPTS */
	irq_connect_dynamic(HAL_RTC_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO,
			    rtc0_nrf5_isr, NULL, 0U);
	irq_connect_dynamic(HAL_SWI_RADIO_IRQ, CONFIG_BT_CTLR_LLL_PRIO,
			    swi_lll_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
	irq_connect_dynamic(HAL_SWI_JOB_IRQ, CONFIG_BT_CTLR_ULL_LOW_PRIO,
			    swi_ull_low_nrf5_isr, NULL, 0U);
#endif

#else /* !CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */
	IRQ_DIRECT_CONNECT(HAL_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			   radio_nrf5_isr, IRQ_CONNECT_FLAGS);
#if defined(CONFIG_BT_CTLR_RADIO_TIMER_ISR)
	IRQ_DIRECT_CONNECT(TIMER0_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			   timer_nrf5_isr, IRQ_CONNECT_FLAGS);
#endif /* CONFIG_BT_CTLR_RADIO_TIMER_ISR */
	IRQ_CONNECT(HAL_RTC_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO,
		    rtc0_nrf5_isr, NULL, 0);
#if defined(CONFIG_BT_CTLR_ZLI)
	IRQ_DIRECT_CONNECT(HAL_SWI_RADIO_IRQ, CONFIG_BT_CTLR_LLL_PRIO,
			   swi_lll_nrf5_isr, IRQ_CONNECT_FLAGS);
#else
	IRQ_CONNECT(HAL_SWI_RADIO_IRQ, CONFIG_BT_CTLR_LLL_PRIO,
		    swi_lll_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#endif
#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
	IRQ_CONNECT(HAL_SWI_JOB_IRQ, CONFIG_BT_CTLR_ULL_LOW_PRIO,
		    swi_ull_low_nrf5_isr, NULL, 0);
#endif
#endif /* !CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */

	/* Enable IRQs */
	irq_enable(HAL_RADIO_IRQn);
	irq_enable(HAL_RTC_IRQn);
	irq_enable(HAL_SWI_RADIO_IRQ);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) ||
		(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)) {
		irq_enable(HAL_SWI_JOB_IRQ);
	}

	radio_setup();

	return 0;
}

int lll_deinit(void)
{
	int err;

	/* Release clocks */
	err = lll_clock_deinit();
	if (err < 0) {
		return err;
	}

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_deinit();
	}

	/* Disable IRQs */
	irq_disable(HAL_RADIO_IRQn);
	irq_disable(HAL_RTC_IRQn);
	irq_disable(HAL_SWI_RADIO_IRQ);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT) ||
		(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)) {
		irq_disable(HAL_SWI_JOB_IRQ);
	}

	/* Disconnect dynamic ISRs used */
#if defined(CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS)
#if defined(CONFIG_SHARED_INTERRUPTS)
#if defined(CONFIG_DYNAMIC_DIRECT_INTERRUPTS)
	irq_disconnect_dynamic(HAL_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			       radio_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#if defined(CONFIG_BT_CTLR_RADIO_TIMER_ISR)
	irq_disconnect_dynamic(TIMER0_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			       timer_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#endif /* CONFIG_BT_CTLR_RADIO_TIMER_ISR */
#endif /* CONFIG_DYNAMIC_DIRECT_INTERRUPTS */
	irq_disconnect_dynamic(HAL_RTC_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO,
			       rtc0_nrf5_isr, NULL, 0U);
	irq_disconnect_dynamic(HAL_SWI_RADIO_IRQ, CONFIG_BT_CTLR_LLL_PRIO,
			       swi_lll_nrf5_isr, NULL, IRQ_CONNECT_FLAGS);
#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
	irq_disconnect_dynamic(HAL_SWI_JOB_IRQ, CONFIG_BT_CTLR_ULL_LOW_PRIO,
			       swi_ull_low_nrf5_isr, NULL, 0U);
#endif
#else /* !CONFIG_SHARED_INTERRUPTS */
#if defined(CONFIG_DYNAMIC_DIRECT_INTERRUPTS)
	irq_connect_dynamic(HAL_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO, NULL, NULL,
			    IRQ_CONNECT_FLAGS);
#endif /* CONFIG_DYNAMIC_DIRECT_INTERRUPTS */
	irq_connect_dynamic(HAL_RTC_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO, NULL, NULL,
			    0U);
	irq_connect_dynamic(HAL_SWI_RADIO_IRQ, CONFIG_BT_CTLR_LLL_PRIO, NULL,
			    NULL, IRQ_CONNECT_FLAGS);
#if defined(CONFIG_BT_CTLR_LOW_LAT) || \
	(CONFIG_BT_CTLR_ULL_HIGH_PRIO != CONFIG_BT_CTLR_ULL_LOW_PRIO)
	irq_connect_dynamic(HAL_SWI_JOB_IRQ, CONFIG_BT_CTLR_ULL_LOW_PRIO, NULL,
			    NULL, 0U);
#endif
#endif /* !CONFIG_SHARED_INTERRUPTS */
#endif /* CONFIG_BT_CTLR_DYNAMIC_INTERRUPTS */

	return 0;
}

int lll_csrand_get(void *buf, size_t len)
{
#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	return entropy_get_entropy(dev_entropy, buf, len);
#else /* !CONFIG_ENTROPY_HAS_DRIVER */
	/* FIXME: No suitable entropy device available yet.
	 *        It is required by Controller to use random numbers.
	 *        Hence, return uninitialized buf contents, for now.
	 */
	return 0;
#endif /* !CONFIG_ENTROPY_HAS_DRIVER */
}

int lll_csrand_isr_get(void *buf, size_t len)
{
#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	return entropy_get_entropy_isr(dev_entropy, buf, len, 0);
#else /* !CONFIG_ENTROPY_HAS_DRIVER */
	/* FIXME: No suitable entropy device available yet.
	 *        It is required by Controller to use random numbers.
	 *        Hence, return uninitialized buf contents, for now.
	 */
	return 0;
#endif /* !CONFIG_ENTROPY_HAS_DRIVER */
}

int lll_rand_get(void *buf, size_t len)
{
#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	return entropy_get_entropy(dev_entropy, buf, len);
#else /* !CONFIG_ENTROPY_HAS_DRIVER */
	/* FIXME: No suitable entropy device available yet.
	 *        It is required by Controller to use random numbers.
	 *        Hence, return uninitialized buf contents, for now.
	 */
	return 0;
#endif /* !CONFIG_ENTROPY_HAS_DRIVER */
}

int lll_rand_isr_get(void *buf, size_t len)
{
#if defined(CONFIG_ENTROPY_HAS_DRIVER)
	return entropy_get_entropy_isr(dev_entropy, buf, len, 0);
#else /* !CONFIG_ENTROPY_HAS_DRIVER */
	/* FIXME: No suitable entropy device available yet.
	 *        It is required by Controller to use random numbers.
	 *        Hence, return uninitialized buf contents, for now.
	 */
	return 0;
#endif /* !CONFIG_ENTROPY_HAS_DRIVER */
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
		} else {
			ull = NULL;
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
	struct event_done_extra *extra;
	uint8_t result;

	/* TODO: Pass from calling function */
	result = DONE_COMPLETED;

	lll_done_score(param, result);

	extra = ull_event_done_extra_get();
	LL_ASSERT(extra);

	/* Set result in done extra data - type was set by the role */
	extra->result = result;
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

bool lll_is_done(void *param, bool *is_resume)
{
	/* NOTE: Current radio event when preempted could put itself in resume
	 *       into the prepare pipeline in which case event.curr.param would
	 *       be set to NULL.
	 */
	*is_resume = (param != event.curr.param);

	return !event.curr.abort_cb;
}

int lll_is_abort_cb(void *next, void *curr, lll_prepare_cb_t *resume_cb)
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

uint32_t lll_event_offset_get(struct ull_hdr *ull)
{
	return HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
}

uint32_t lll_preempt_calc(struct ull_hdr *ull, uint8_t ticker_id,
		       uint32_t ticks_at_event)
{
	uint32_t ticks_now;
	uint32_t diff;

	ticks_now = ticker_ticks_now_get();
	diff = ticker_ticks_diff_get(ticks_now, ticks_at_event);
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
		return diff;
	}

	return 0U;
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

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}
}

void lll_isr_rx_status_reset(void)
{
	radio_status_reset();
	radio_tmr_status_reset();
	radio_rssi_status_reset();

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}
}

void lll_isr_tx_sub_status_reset(void)
{
	radio_status_reset();
	radio_tmr_tx_status_reset();

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}
}

void lll_isr_rx_sub_status_reset(void)
{
	radio_status_reset();
	radio_tmr_rx_status_reset();

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
		radio_gpio_pa_lna_disable();
	}
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

	if (IS_ENABLED(HAL_RADIO_GPIO_HAVE_PA_PIN) ||
	    IS_ENABLED(HAL_RADIO_GPIO_HAVE_LNA_PIN)) {
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
	radio_stop();

	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	lll_done(NULL);
}

void lll_isr_early_abort(void *param)
{
	int err;

	radio_isr_set(isr_race, param);
	if (!radio_is_idle()) {
		radio_disable();
	}

	err = lll_hfclock_off();
	LL_ASSERT(err >= 0);

	lll_done(NULL);
}

int lll_prepare_resolve(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
			lll_prepare_cb_t prepare_cb,
			struct lll_prepare_param *prepare_param,
			uint8_t is_resume, uint8_t is_dequeue)
{
	struct lll_event *ready_short = NULL;
	struct lll_event *ready;
	struct lll_event *next;
	uint8_t idx;
	int err;

	/* Find the ready prepare in the pipeline */
	idx = UINT8_MAX;
	ready = prepare_dequeue_iter_ready_get(&idx);

	/* Find any short prepare */
	if (ready) {
		uint32_t ticks_at_preempt_min = prepare_param->ticks_at_expire;
		uint32_t ticks_at_preempt_next;
		uint8_t idx_backup = idx;
		uint32_t diff;

		ticks_at_preempt_next = ready->prepare_param.ticks_at_expire;
		diff = ticker_ticks_diff_get(ticks_at_preempt_min,
					     ticks_at_preempt_next);
		if (is_resume || ((diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U)) {
			ticks_at_preempt_min = ticks_at_preempt_next;
			if (&ready->prepare_param != prepare_param) {
				ready_short = ready;
			}
		} else {
			ready = NULL;
			idx_backup = UINT8_MAX;
		}

		do {
			struct lll_event *ready_next;

			ready_next = prepare_dequeue_iter_ready_get(&idx);
			if (!ready_next) {
				break;
			}

			ticks_at_preempt_next = ready_next->prepare_param.ticks_at_expire;
			diff = ticker_ticks_diff_get(ticks_at_preempt_next,
						     ticks_at_preempt_min);
			if ((diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
				continue;
			}

			ready_short = ready_next;
			ticks_at_preempt_min = ticks_at_preempt_next;
		} while (true);

		idx = idx_backup;
	}

	/* Current event active or another prepare is ready in the pipeline */
	if ((!is_dequeue && !is_done_sync()) ||
	    event.curr.abort_cb || ready_short ||
	    (ready && is_resume)) {
#if defined(CONFIG_BT_CTLR_LOW_LAT)
		lll_prepare_cb_t resume_cb;
#endif /* CONFIG_BT_CTLR_LOW_LAT */

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

		/* Find any short prepare */
		if (ready_short) {
			ready = ready_short;
		}

		/* Always start preempt timeout for first prepare in pipeline */
		struct lll_event *first = ready ? ready : next;
		uint32_t ret;

		/* Start the preempt timeout */
		ret  = preempt_ticker_start(first, ready, next);
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));

#else /* CONFIG_BT_CTLR_LOW_LAT */
		next = NULL;
		while (ready) {
			if (!ready->is_aborted) {
				if (event.curr.param == ready->prepare_param.param) {
					ready->is_aborted = 1;
					ready->abort_cb(&ready->prepare_param,
							ready->prepare_param.param);
				} else {
					next = ready;
				}
			}

			ready = ull_prepare_dequeue_iter(&idx);
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

	LL_ASSERT(!ready || &ready->prepare_param == prepare_param);

	event.curr.param = prepare_param->param;
	event.curr.is_abort_cb = is_abort_cb;
	event.curr.abort_cb = abort_cb;

	err = prepare_cb(prepare_param);

	if (!IS_ENABLED(CONFIG_BT_CTLR_ASSERT_OVERHEAD_START) &&
	    (err == -ECANCELED)) {
		err = 0;
	}

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
	uint32_t ret;

	/* NOTE: preempt timeout started prior for the current event that has
	 *       its prepare that is now invoked is not explicitly stopped here.
	 *       If there is a next prepare event in pipeline, then the prior
	 *       preempt timeout if started will be stopped before starting
	 *       the new preempt timeout. Refer to implementation in
	 *       preempt_ticker_start().
	 */

	/* Find next prepare needing preempt timeout to be setup */
	next = prepare_dequeue_iter_ready_get(&idx);
	if (!next) {
		return err;
	}

	/* Start the preempt timeout */
	ret = preempt_ticker_start(next, NULL, next);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));
#endif /* !CONFIG_BT_CTLR_LOW_LAT */

	return err;
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

static inline struct lll_event *prepare_dequeue_iter_ready_get(uint8_t *idx)
{
	struct lll_event *ready;

	do {
		ready = ull_prepare_dequeue_iter(idx);
	} while (ready && (ready->is_aborted || ready->is_resume));

	return ready;
}

static inline struct lll_event *resume_enqueue(lll_prepare_cb_t resume_cb)
{
	struct lll_prepare_param prepare_param = {0};

	/* Enqueue into prepare pipeline as resume radio event, and remove
	 * parameter assignment from currently active radio event so that
	 * done event is not generated.
	 */
	prepare_param.param = event.curr.param;
	event.curr.param = NULL;

	return ull_prepare_enqueue(event.curr.is_abort_cb, event.curr.abort_cb,
				   &prepare_param, resume_cb, 1);
}

static void isr_race(void *param)
{
	/* NOTE: lll_disable could have a race with ... */
	radio_status_reset();
}

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
static uint8_t volatile preempt_start_req;
static uint8_t preempt_start_ack;
static uint8_t volatile preempt_stop_req;
static uint8_t preempt_stop_ack;
static uint8_t preempt_req;
static uint8_t volatile preempt_ack;

static void ticker_stop_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(preempt_stop_req != preempt_stop_ack);
	preempt_stop_ack = preempt_stop_req;

	/* We do not fail on status not being success because under scenarios
	 * where there is ticker_start then ticker_stop and then ticker_start,
	 * the call to ticker_stop will fail and this is acceptable.
	 * Also, the preempt_req and preempt_ack would not be update as the
	 * ticker_start was not processed before ticker_stop. Hence, it is
	 * safe to reset preempt_req and preempt_ack here.
	 */
	if (status == TICKER_STATUS_SUCCESS) {
		LL_ASSERT(preempt_req != preempt_ack);
	}

	preempt_req = preempt_ack;
}

static void ticker_start_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);
	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	/* Increase preempt requested count before acknowledging that the
	 * ticker start operation for the preempt timeout has been handled.
	 */
	LL_ASSERT(preempt_req == preempt_ack);
	preempt_req++;

	/* Increase preempt start ack count, to acknowledge that the ticker
	 * start operation has been handled.
	 */
	LL_ASSERT(preempt_start_req != preempt_start_ack);
	preempt_start_ack = preempt_start_req;
}

static uint32_t preempt_ticker_start(struct lll_event *first,
				     struct lll_event *prev,
				     struct lll_event *next)
{
	const struct lll_prepare_param *p;
	static uint32_t ticks_at_preempt;
	uint32_t ticks_at_preempt_new;
	uint32_t preempt_anchor;
	struct ull_hdr *ull;
	uint32_t preempt_to;
	uint32_t ret;

	/* Do not request to start preempt timeout if already requested.
	 *
	 * Check if there is pending preempt timeout start requested or if
	 * preempt timeout ticker has already been scheduled.
	 */
	if ((preempt_start_req != preempt_start_ack) ||
	    (preempt_req != preempt_ack)) {
		uint32_t diff;

		/* Calc the preempt timeout */
		p = &next->prepare_param;
		ull = HDR_LLL2ULL(p->param);
		preempt_anchor = p->ticks_at_expire;
		preempt_to = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US) -
			     HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);

		ticks_at_preempt_new = preempt_anchor + preempt_to;
		ticks_at_preempt_new &= HAL_TICKER_CNTR_MASK;

		/* Check for short preempt timeouts */
		diff = ticker_ticks_diff_get(ticks_at_preempt_new,
					     ticks_at_preempt);
		if ((diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
			return TICKER_STATUS_SUCCESS;
		}

		/* Stop any scheduled preempt ticker */
		ret = preempt_ticker_stop();
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));

		/* Schedule short preempt timeout */
		first = next;
	} else {
		/* Calc the preempt timeout */
		p = &first->prepare_param;
		ull = HDR_LLL2ULL(p->param);
		preempt_anchor = p->ticks_at_expire;
		preempt_to = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US) -
			     HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);

		ticks_at_preempt_new = preempt_anchor + preempt_to;
		ticks_at_preempt_new &= HAL_TICKER_CNTR_MASK;
	}

	preempt_start_req++;

	ticks_at_preempt = ticks_at_preempt_new;

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
			   preempt_ticker_cb, first->prepare_param.param,
			   ticker_start_op_cb, NULL);

	return ret;
}

static uint32_t preempt_ticker_stop(void)
{
	uint32_t ret;

	/* Do not request to stop preempt timeout if already requested or
	 * has expired
	 */
	if ((preempt_stop_req != preempt_stop_ack) ||
	    (preempt_req == preempt_ack)) {
		return TICKER_STATUS_SUCCESS;
	}

	preempt_stop_req++;

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR,
			  TICKER_USER_ID_LLL,
			  TICKER_ID_LLL_PREEMPT,
			  ticker_stop_op_cb, NULL);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));

	return ret;
}

static void preempt_ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			      uint32_t remainder, uint16_t lazy, uint8_t force,
			      void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, preempt};
	uint32_t ret;

	LL_ASSERT(preempt_ack != preempt_req);
	preempt_ack = preempt_req;

	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT(!ret);
}

static void preempt(void *param)
{
	lll_prepare_cb_t resume_cb;
	struct lll_event *ready;
	uint8_t idx;
	int err;

	/* No event to abort */
	if (!event.curr.abort_cb || !event.curr.param) {
		return;
	}

preempt_find_preemptor:
	/* Find a prepare that is ready and not a resume */
	idx = UINT8_MAX;
	ready = prepare_dequeue_iter_ready_get(&idx);
	if (!ready) {
		/* No ready prepare */
		return;
	}

	/* Preemptor not in pipeline */
	if (ready->prepare_param.param != param) {
		uint32_t ticks_at_preempt_min = ready->prepare_param.ticks_at_expire;
		struct lll_event *ready_short = NULL;
		struct lll_event *ready_next = NULL;
		struct lll_event *preemptor;

		/* Find if the short prepare request in the pipeline */
		do {
			uint32_t ticks_at_preempt_next;
			uint32_t diff;

			preemptor = prepare_dequeue_iter_ready_get(&idx);
			if (!preemptor) {
				break;
			}

			if (!ready_next) {
				ready_next = preemptor;
			}

			if (preemptor->prepare_param.param == param) {
				break;
			}

			ticks_at_preempt_next = preemptor->prepare_param.ticks_at_expire;
			diff = ticker_ticks_diff_get(ticks_at_preempt_next,
						     ticks_at_preempt_min);
			if ((diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U) {
				continue;
			}

			ready_short = preemptor;
			ticks_at_preempt_min = ticks_at_preempt_next;
		} while (true);

		/* "The" short prepare we were looking for is not in pipeline */
		if (!preemptor) {
			uint32_t ret;

			/* Find any short prepare */
			if (ready_short) {
				ready = ready_short;
			}

			/* Start the preempt timeout for (short) ready event */
			ret = preempt_ticker_start(ready, NULL, ready);
			LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
				  (ret == TICKER_STATUS_BUSY));

			return;
		}

		/* FIXME: Prepare pipeline is not a ordered list implementation,
		 *        and for short prepare being enqueued, ideally the
		 *        pipeline has to be implemented as ordered list.
		 *        Until then a workaround to abort a prepare present
		 *        before the short prepare being enqueued is implemented
		 *        below.
		 *        A proper solution will be to re-design the pipeline
		 *        as a ordered list, instead of the current FIFO.
		 */

		/* Abort the prepare that is present before the short prepare */
		ready->is_aborted = 1;
		ready->abort_cb(&ready->prepare_param, ready->prepare_param.param);

		/* Abort all events in pipeline before the short prepare */
		if (preemptor != ready_next) {
			goto preempt_find_preemptor;
		}

		/* As the prepare queue has been refreshed due to the call of
		 * abort_cb which invokes the lll_done, find the latest prepare
		 */
		idx = UINT8_MAX;
		ready = prepare_dequeue_iter_ready_get(&idx);
		if (!ready) {
			/* No ready prepare */
			return;
		}

		LL_ASSERT(ready->prepare_param.param == param);
	}

	/* Check if current event want to continue */
	err = event.curr.is_abort_cb(ready->prepare_param.param, event.curr.param, &resume_cb);
	if (!err || (err == -EBUSY)) {
		/* Returns -EBUSY when same curr and next state/role, do not
		 * abort same curr and next event.
		 */
		if (err != -EBUSY) {
			/* Let preemptor LLL know about the cancelled prepare */
			ready->is_aborted = 1;
			ready->abort_cb(&ready->prepare_param, ready->prepare_param.param);
		}

		return;
	}

	/* Abort the current event */
	event.curr.abort_cb(NULL, event.curr.param);

	/* Check if resume requested */
	if (err == -EAGAIN) {
		uint8_t is_resume_abort = 0U;
		struct lll_event *iter;
		uint8_t iter_idx;

preempt_abort_resume:
		/* Abort any duplicate non-resume, that they get dequeued */
		iter_idx = UINT8_MAX;
		iter = ull_prepare_dequeue_iter(&iter_idx);
		while (iter) {
			if (!iter->is_aborted &&
			    (is_resume_abort || !iter->is_resume) &&
			    event.curr.param == iter->prepare_param.param) {
				iter->is_aborted = 1;
				iter->abort_cb(&iter->prepare_param,
					       iter->prepare_param.param);

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
				/* NOTE: abort_cb called lll_done which modifies
				 *       the prepare pipeline hence re-iterate
				 *       through the prepare pipeline.
				 */
				iter_idx = UINT8_MAX;
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
			}

			iter = ull_prepare_dequeue_iter(&iter_idx);
		}

		if (!is_resume_abort) {
			is_resume_abort = 1U;

			goto preempt_abort_resume;
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
