/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The event prepare pipeline uses only the ULL prepare queue and the ticker,
 * so the LLL implementations share it rather than each keeping a copy.
 */

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

#include "hal/ticker.h"

#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_clock.h"
#include "lll/lll_internal.h"

#include "hal/debug.h"

static struct {
	struct {
		void              *param;
		lll_is_abort_cb_t is_abort_cb;
		lll_abort_cb_t    abort_cb;
		uint8_t           has_margin:1;
	} curr;

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
	struct {
		uint8_t volatile lll_count;
		uint8_t          ull_count;
	} done;
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
} event;

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
static inline void done_inc(void);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
static inline bool is_done_sync(void);
static inline struct lll_event *prepare_dequeue_iter_ready_get(uint8_t *idx);
static inline struct lll_event *resume_enqueue(lll_is_abort_cb_t is_abort_cb,
					       lll_abort_cb_t abort_cb, lll_prepare_cb_t resume_cb,
					       void *param);

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

void lll_prepare_pipeline_init(void)
{
	event.curr.abort_cb = NULL;
}

void lll_disable(void *param)
{
	/* LLL disable of current event, done is generated */
	if (!param || (param == event.curr.param)) {
		if (event.curr.abort_cb && event.curr.param) {
			event.curr.abort_cb(NULL, event.curr.param);
		} else {
			LL_ASSERT_ERR(!param);
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
	LL_ASSERT_ERR(!param || next);

	/* check if current LLL event is done */
	if (!param) {
		/* Reset current event instance */
		LL_ASSERT_ERR(event.curr.abort_cb);
		event.curr.abort_cb = NULL;

		param = event.curr.param;
		event.curr.param = NULL;

		/* Resume events will have set event.curr.param to NULL, these
		 * should not generate done events.
		 */
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
#else /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */
	done_inc();
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	struct event_done_extra *extra;
	uint8_t result;

	/* TODO: Pass from calling function */
	result = DONE_COMPLETED;

	lll_done_score(param, result);

	extra = ull_event_done_extra_get();
	LL_ASSERT_ERR(extra);

	/* Set result in done extra data - type was set by the role */
	extra->result = result;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

	/* Let ULL know about LLL event done */
	evdone = ull_event_done(ull);
	LL_ASSERT_ERR(evdone);

	return 0;
}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
void lll_done_ull_inc(void)
{
	LL_ASSERT_ERR(event.done.ull_count != event.done.lll_count);
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
		/* If the enqueued prepare is a resume or current ready prepare is shorter, then we
		 * should pick current ready prepare for setting up the prepare timeout.
		 */
		if (is_resume || ((diff & BIT(HAL_TICKER_CNTR_MSBIT)) == 0U)) {
			ticks_at_preempt_min = ticks_at_preempt_next;
			if (&ready->prepare_param != prepare_param) {
				/* There is a shorter prepare in the pipeline */
				ready_short = ready;
			} else {
				/* It is the same prepare in the pipeline being enqueued.
				 * This can happen executing `lll_done()`.
				 * Hence, we should ignore it being the `first` that setup the
				 * preempt timeout and also it has already setup the preempt
				 * timeout, refer to `preempt_ticker_start()` for details.
				 *
				 * We also set the `ready` to NULL as it is the same ready, the one
				 * being enqueued. This help short circuit a related assertion check
				 * later in this function.
				 */
				ready = NULL;
			}
		} else {
			ready = NULL;
			idx_backup = UINT8_MAX;
		}

		/* Loop and find any short prepare present out-of-order in the prepare pipeline.
		 *
		 * NOTE: This loop is O(n), where n is number of items in prepare pipeline present
		 *       before a short prepare was enqueued in to the FIFO.
		 *       Use of ordered linked list implementation has show improved lower latencies
		 *       and less CPU use.
		 * TODO: Replace use of FIFO for prepare pipeline with ordered linked list
		 *       implementation.
		 */
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
	if (((is_dequeue == 0U) && (is_done_sync() == 0U)) ||
	    (event.curr.abort_cb != NULL) ||
	    (ready_short != NULL) ||
	    ((ready != NULL) && (is_resume != 0U)) ||
	    (IS_ENABLED(CONFIG_BT_CTLR_LLL_PREPARE_AT_MARGIN) &&
	     (prepare_param->defer == 0U) &&
	     (event.curr.has_margin == 0U))) {
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
		LL_ASSERT_ERR(next);

#if !defined(CONFIG_BT_CTLR_LOW_LAT)
		if (is_resume || prepare_param->defer) {
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
		LL_ASSERT_ERR((ret == TICKER_STATUS_SUCCESS) ||
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
			LL_ASSERT_DBG(err);

			if (err == -EAGAIN) {
				void *curr_param;

				/* Remove parameter assignment from currently active radio event so
				 * that done event is not generated.
				 */
				curr_param = event.curr.param;
				event.curr.param = NULL;

				next = resume_enqueue(event.curr.is_abort_cb, event.curr.abort_cb,
						      resume_cb, curr_param);
				LL_ASSERT_ERR(next);
			} else {
				LL_ASSERT_ERR(err == -ECANCELED);
			}
		}
#endif /* CONFIG_BT_CTLR_LOW_LAT */

		return -EINPROGRESS;
	}

	LL_ASSERT_ERR(!ready || &ready->prepare_param == prepare_param);

	event.curr.param = prepare_param->param;
	event.curr.is_abort_cb = is_abort_cb;
	event.curr.abort_cb = abort_cb;

	if (IS_ENABLED(CONFIG_BT_CTLR_LLL_PREPARE_AT_MARGIN)) {
		event.curr.has_margin = 0U;
	}

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
	LL_ASSERT_ERR((ret == TICKER_STATUS_SUCCESS) ||
		      (ret == TICKER_STATUS_BUSY));
#endif /* !CONFIG_BT_CTLR_LOW_LAT */

	return err;
}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
static inline void done_inc(void)
{
	event.done.lll_count++;
	LL_ASSERT_ERR(event.done.lll_count != event.done.ull_count);
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
	} while ((ready != NULL) && ((ready->is_aborted != 0U) || (ready->is_resume != 0U) ||
				     (ready->prepare_param.defer != 0U)));

	return ready;
}

static inline struct lll_event *resume_enqueue(lll_is_abort_cb_t is_abort_cb,
					       lll_abort_cb_t abort_cb, lll_prepare_cb_t resume_cb,
					       void *param)
{
	struct lll_prepare_param prepare_param = {0};

	prepare_param.param = param;

	return ull_prepare_enqueue(is_abort_cb, abort_cb, &prepare_param, resume_cb, 1U);
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

	LL_ASSERT_ERR(preempt_stop_req != preempt_stop_ack);
	preempt_stop_ack = preempt_stop_req;

	/* We do not fail on status not being success because under scenarios
	 * where there is ticker_start then ticker_stop and then ticker_start,
	 * the call to ticker_stop will fail and this is acceptable.
	 * Also, the preempt_req and preempt_ack would not be update as the
	 * ticker_start was not processed before ticker_stop. Hence, it is
	 * safe to reset preempt_req and preempt_ack here.
	 */
	if (status == TICKER_STATUS_SUCCESS) {
		LL_ASSERT_ERR(preempt_req != preempt_ack);
	}

	preempt_req = preempt_ack;
}

static void ticker_start_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);
	LL_ASSERT_ERR(status == TICKER_STATUS_SUCCESS);

	/* Increase preempt requested count before acknowledging that the
	 * ticker start operation for the preempt timeout has been handled.
	 */
	LL_ASSERT_ERR(preempt_req == preempt_ack);
	preempt_req++;

	/* Increase preempt start ack count, to acknowledge that the ticker
	 * start operation has been handled.
	 */
	LL_ASSERT_ERR(preempt_start_req != preempt_start_ack);
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
		LL_ASSERT_ERR((ret == TICKER_STATUS_SUCCESS) ||
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
	LL_ASSERT_ERR((ret == TICKER_STATUS_SUCCESS) ||
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

	LL_ASSERT_ERR(preempt_ack != preempt_req);
	preempt_ack = preempt_req;

	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL,
			     0, &mfy);
	LL_ASSERT_ERR(!ret);
}

static void preempt(void *param)
{
	lll_prepare_cb_t resume_cb;
	struct lll_event *ready;
	uint8_t idx;
	int err;

	/* No event to abort */
	if (!event.curr.abort_cb || !event.curr.param) {
		/* When a radio event is placed back in the prepare pipeline as
		 * resume prepare and a done event is not to be generated; in
		 * these cases, event.curr.abort_cb is not NULL, but
		 * event.curr.param is NULL. Let us setup the preempt timeout to
		 * ensure the margin for certain.
		 */
		if (IS_ENABLED(CONFIG_BT_CTLR_LLL_PREPARE_AT_MARGIN) &&
		    (event.curr.abort_cb == NULL)) {
			/* Previous event is done before the prepare margin for
			 * the event ready in the pipeline when we are here now.
			 */
			event.curr.has_margin = 1U;

			/* Execute the enqueued ready LLL prepare callbacks */
			ull_prepare_dequeue(TICKER_USER_ID_LLL);
		}

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
			LL_ASSERT_ERR((ret == TICKER_STATUS_SUCCESS) ||
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

		LL_ASSERT_ERR(ready->prepare_param.param == param);
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_LLL_PREPARE_AT_MARGIN)) {
		/* Here prepare margin has expired while a previous event is
		 * active, set the flag and proceed with abort.
		 */
		event.curr.has_margin = 1U;
	}

	/* Check if current event want to continue */
	err = event.curr.is_abort_cb(ready->prepare_param.param, event.curr.param, &resume_cb);
	if (!err || (err == -EBUSY)) {
		if (err == -EBUSY) {
			uint32_t ret;

			/* Returns -EBUSY when same curr and next ready state/role, do not abort
			 * same curr and next ready event.
			 */
			ready->prepare_param.defer = 1U;

			/* Find next prepare that is ready and not a resume */
			ready = prepare_dequeue_iter_ready_get(&idx);
			if (ready == NULL) {
				/* No ready prepare */
				return;
			}

			/* Start the preempt timeout for next ready prepare */
			ret = preempt_ticker_start(ready, NULL, ready);
			LL_ASSERT_ERR((ret == TICKER_STATUS_SUCCESS) ||
				      (ret == TICKER_STATUS_BUSY));

		} else {
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
		lll_is_abort_cb_t is_abort_cb;
		lll_abort_cb_t abort_cb;
		uint8_t is_resume_abort;
		struct lll_event *iter;
		uint8_t iter_idx;
		void *curr_param;

		/* Remove parameter assignment from currently active radio event so that done event
		 * is not generated.
		 */
		curr_param = event.curr.param;
		event.curr.param = NULL;

		/* backup is_abort_cb and abort_cb */
		is_abort_cb = event.curr.is_abort_cb;
		abort_cb = event.curr.abort_cb;

		/* Iterate twice to ensure preempt timeout is setup after all duplicate resume
		 * events are aborted.
		 */
		is_resume_abort = 0U;

preempt_abort_resume:
		/* Abort any duplicate non-resume, that they get dequeued */
		iter_idx = UINT8_MAX;
		iter = ull_prepare_dequeue_iter(&iter_idx);
		while (iter) {
			if (!iter->is_aborted &&
			    (is_resume_abort || !iter->is_resume) &&
			    (curr_param == iter->prepare_param.param)) {
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
		iter = resume_enqueue(is_abort_cb, abort_cb, resume_cb, curr_param);
		LL_ASSERT_ERR(iter);
	} else {
		LL_ASSERT_ERR(err == -ECANCELED);
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
	LL_ASSERT_ERR((ret == TICKER_STATUS_SUCCESS) ||
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
