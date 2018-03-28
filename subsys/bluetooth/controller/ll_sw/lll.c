#include <errno.h>

#include <zephyr/types.h>
#include <device.h>
#include <clock_control.h>

#if defined(CONFIG_SOC_FAMILY_NRF)
#include "hal/nrf5/ticker.h"
#define CLOCK_CONTROL_DRV_NAME CONFIG_CLOCK_CONTROL_NRF5_M16SRC_DRV_NAME
#endif /* CONFIG_SOC_FAMILY_NRF */

#include "util/memq.h"

#include "util/mayfly.h"
#include "ticker/ticker.h"

#include "lll.h"
#include "lll_internal.h"

#include "common/log.h"
#include <soc.h>
#include "hal/debug.h"

static struct {
	struct {
		void              *param;
		lll_is_abort_cb_t is_abort_cb;
		lll_abort_cb_t    abort_cb;
	} curr;

	struct {
		int                      prio;
		struct lll_prepare_param prepare_param;
		lll_prepare_cb_t         prepare_cb;
		lll_is_abort_cb_t        is_abort_cb;
		lll_abort_cb_t           abort_cb;
	} next;
} event;

static struct {
	struct device *clk_hf;
} lll;

static int _init_reset(void);
static void _preempt_ticker_cb(u32_t ticks_at_expire, u32_t remainder,
			       u16_t lazy, void *param);
static void _preempt(void *param);

int lll_init(void)
{
	int err;

	/* Initialise LLL internals */
	event.curr.abort_cb = NULL;
	event.next.prepare_cb = NULL;

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
	/* Assert if a deferred prepare is pending */
	LL_ASSERT(!event.next.prepare_cb);

	if (event.curr.abort_cb) {
		u32_t preempt_anchor;
		u32_t preempt_to;
		int ret;

		/* Store the next prepare for deferred call */
		event.next.prio = prio;
		memcpy(&event.next.prepare_param, prepare_param,
		       sizeof(event.next.prepare_param));
		event.next.prepare_cb = prepare_cb;
		event.next.is_abort_cb = is_abort_cb;
		event.next.abort_cb = abort_cb;

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
	if (param == event.curr.param) {
		event.curr.abort_cb(NULL, event.curr.param);
	} else if (param == event.next.prepare_param.param) {
		/* TODO: implement prepare pipeline and search in it */
		event.next.abort_cb(&event.next.prepare_param,
				    event.next.prepare_param.param);
	} else {
		LL_ASSERT(0);
	}
}

int lll_done(void *param)
{
	/* Assert if param supplied without a pending prepare to cancel. */
	LL_ASSERT(!param || event.next.abort_cb);

	/* check if current LLL event is done */
	if (!param) {
		LL_ASSERT(event.curr.abort_cb);
		event.curr.abort_cb = NULL;

		param = event.curr.param;
		event.curr.param = NULL;

		DEBUG_RADIO_CLOSE(0);
	}

	/* Let ULL know about LLL event done */
	ull_event_done(((struct lll_hdr *)param)->parent);

	/* Call the pending next prepare's bottom half */
	if (event.next.prepare_cb) {
		lll_prepare_cb_t prepare_cb = event.next.prepare_cb;

		event.next.prepare_cb = NULL;

		event.curr.is_abort_cb = event.next.is_abort_cb;
		event.curr.abort_cb = event.next.abort_cb;
		event.curr.param = event.next.prepare_param.param;

		return prepare_cb(&event.next.prepare_param);
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
	if (!event.curr.is_abort_cb(event.next.prepare_param.param,
				    event.next.prio, event.curr.param)) {
		/* Cancel next prepare */
		event.next.prepare_cb = NULL;

		/* Let LLL know about the cancelled prepare */
		event.next.abort_cb(&event.next.prepare_param,
				    event.next.prepare_param.param);

		return;
	}

	event.curr.abort_cb(NULL, event.curr.param);
}
