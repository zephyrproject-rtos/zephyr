#include <errno.h>

#include <zephyr/types.h>
#include <device.h>
#include <clock_control.h>

#if defined(CONFIG_SOC_FAMILY_NRF)
#include "hal/nrf5/ticker.h"
#define CLOCK_CONTROL_DRV_NAME CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME
#endif /* CONFIG_SOC_FAMILY_NRF */

#include "util/mem.h"
#include "util/memq.h"

#include "util/mayfly.h"
#include "ticker/ticker.h"

#include "lll.h"
#include "lll_internal.h"

#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

struct event_next {
	void			 *next;
	struct lll_prepare_param prepare_param;
	lll_prepare_cb_t         prepare_cb;
	lll_is_abort_cb_t        is_abort_cb;
	lll_abort_cb_t           abort_cb;
	int                      prio;
	u8_t                     is_resume:1;
};

static struct {
	struct {
		void              *param;
		lll_is_abort_cb_t is_abort_cb;
		lll_abort_cb_t    abort_cb;
	} curr;

	struct {
		struct {
			void *free;
			u8_t pool[sizeof(struct event_next) *
				  EVENT_PIPELINE_MAX];
		} mem;
		void *head;
		void *tail;
	} next;
} event;

static struct {
	struct device *clk_hf;
} lll;

static int _init_reset(void);
static int prepare_enqueue(struct lll_prepare_param *prepare_param,
			   lll_prepare_cb_t prepare_cb,
			   lll_is_abort_cb_t is_abort_cb,
			   lll_abort_cb_t abort_cb,
			   int prio);
static void *prepare_dequeue(struct event_next *next);
static void _preempt_ticker_cb(u32_t ticks_at_expire, u32_t remainder,
			       u16_t lazy, void *param);
static void _preempt(void *param);

int lll_init(void)
{
	int err;

	/* Initialise LLL internals */
	event.curr.abort_cb = NULL;
	mem_init(event.next.mem.pool, sizeof(struct event_next),
		 EVENT_PIPELINE_MAX, &event.next.mem.free);
	event.next.head = event.next.tail = NULL;

	/* Initialize HF CLK */
	lll.clk_hf = device_get_binding(CLOCK_CONTROL_DRV_NAME);
	if (!lll.clk_hf) {
		return -ENODEV;
	}

	err = _init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_reset(void)
{
	int err;

	err = _init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int lll_prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort_cb,
		lll_prepare_cb_t prepare_cb, int prio,
		struct lll_prepare_param *prepare_param)
{
	if (event.curr.abort_cb) {
		u32_t preempt_anchor;
		u32_t preempt_to;
		int ret;

		/* Store the next prepare for deferred call */
		ret = prepare_enqueue(prepare_param, prepare_cb,
				      is_abort_cb, abort_cb,
				      prio);
		LL_ASSERT(!ret);

		/* Calc the preempt timeout */
		preempt_anchor = prepare_param->ticks_at_expire;
		preempt_to = HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_US);

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
				   _preempt_ticker_cb, NULL,
				   NULL, NULL);
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (ret == TICKER_STATUS_BUSY));

		return -EINPROGRESS;
	}

	event.curr.param = prepare_param->param;
	event.curr.is_abort_cb = is_abort_cb;
	event.curr.abort_cb = abort_cb;

	return prepare_cb(prepare_param);
}

void lll_disable(void *param)
{
	if (!param || param == event.curr.param) {
		if (event.curr.abort_cb && event.curr.param) {
			event.curr.abort_cb(NULL, event.curr.param);
		} else {
			LL_ASSERT(!param);
		}
	} else {
		struct event_next *next = event.next.head;

		/* TODO:  search inside the pipeline */
		if (next && param == next->prepare_param.param) {
			next->abort_cb(&next->prepare_param,
				       next->prepare_param.param);
		} else {
			LL_ASSERT(0);
		}
	}
}

int lll_done(void *param)
{
	struct event_next *next = event.next.head;

	/* Assert if param supplied without a pending prepare to cancel. */
	LL_ASSERT(!param || next);

	/* check if current LLL event is done */
	if (!param) {
		/* Reset current event instance */
		LL_ASSERT(event.curr.abort_cb);
		event.curr.abort_cb = NULL;

		param = event.curr.param;
		event.curr.param = NULL;

		DEBUG_RADIO_CLOSE(0);
	} else {
		/* dequeue aborted next prepare */
		next = prepare_dequeue(next);
	}

	/* Let ULL know about LLL event done */
	ull_event_done(((struct lll_hdr *)param)->parent);

	/* Call the pending next prepare's bottom half */
	if (next) {
		lll_prepare_cb_t prepare_cb = next->prepare_cb;
		int ret;

		event.curr.is_abort_cb = next->is_abort_cb;
		event.curr.abort_cb = next->abort_cb;
		event.curr.param = next->prepare_param.param;

		ret = prepare_cb(&next->prepare_param);

		prepare_dequeue(next);

		return ret;
	}

	return 0;
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
	err = clock_control_on(lll.clk_hf, (void *)1);
	if (!err || err == -EINPROGRESS) {
		DEBUG_RADIO_XTAL(1);
	}

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

static int _init_reset(void)
{
	return 0;
}

static int prepare_enqueue(struct lll_prepare_param *prepare_param,
			   lll_prepare_cb_t prepare_cb,
			   lll_is_abort_cb_t is_abort_cb,
			   lll_abort_cb_t abort_cb,
			   int prio)
{
	struct event_next *next;

	/* allocate */
	next = mem_acquire(&event.next.mem.free);
	if (!next) {
		return -ENOBUFS;
	}

	/* setup */
	memcpy(&next->prepare_param, prepare_param,
	       sizeof(next->prepare_param));
	next->prepare_cb = prepare_cb;
	next->is_abort_cb = is_abort_cb;
	next->abort_cb = abort_cb;
	next->prio = prio;

	/* enqueue */
	next->next = NULL;
	if (!event.next.head) {
		event.next.head = event.next.tail = next;
	} else {
		((struct event_next *)event.next.tail)->next = next;
		event.next.tail = next;
	}

	return 0;
}

static void *prepare_dequeue(struct event_next *next)
{
	LL_ASSERT(event.next.head == next);

	event.next.head = next->next;
	if (!event.next.head) {
		event.next.tail = event.next.head;
	}

	/* release event_next struct */
	mem_release(next, &event.next.mem.free);

	return event.next.head;
}

static void _preempt_ticker_cb(u32_t ticks_at_expire, u32_t remainder,
			       u16_t lazy, void *param)
{
	static memq_link_t _link;
	static struct mayfly _mfy = {0, 0, &_link, NULL, _preempt};
	u32_t ret;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &_mfy);
	LL_ASSERT(!ret);
}

static void _preempt(void *param)
{
	struct event_next *next = event.next.head;

	if (!next || !event.curr.abort_cb || !event.curr.param) {
		return;
	}

	if (!event.curr.is_abort_cb(next->prepare_param.param, next->prio,
				    event.curr.param)) {
		/* Let LLL know about the cancelled prepare */
		next->abort_cb(&next->prepare_param, next->prepare_param.param);

		return;
	}

	event.curr.abort_cb(NULL, event.curr.param);
}
