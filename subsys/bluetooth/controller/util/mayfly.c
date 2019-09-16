/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <sys/printk.h>
#include "memq.h"
#include "mayfly.h"

static struct {
	memq_link_t *head;
	memq_link_t *tail;
	u8_t        enable_req;
	u8_t        enable_ack;
	u8_t        disable_req;
	u8_t        disable_ack;
} mft[MAYFLY_CALLEE_COUNT][MAYFLY_CALLER_COUNT];

static memq_link_t mfl[MAYFLY_CALLEE_COUNT][MAYFLY_CALLER_COUNT];
static u8_t mfp[MAYFLY_CALLEE_COUNT];

#if defined(MAYFLY_UT)
static u8_t _state;
#endif /* MAYFLY_UT */

void mayfly_init(void)
{
	u8_t callee_id;

	callee_id = MAYFLY_CALLEE_COUNT;
	while (callee_id--) {
		u8_t caller_id;

		caller_id = MAYFLY_CALLER_COUNT;
		while (caller_id--) {
			memq_init(&mfl[callee_id][caller_id],
				  &mft[callee_id][caller_id].head,
				  &mft[callee_id][caller_id].tail);
		}
	}
}

void mayfly_enable(u8_t caller_id, u8_t callee_id, u8_t enable)
{
	if (enable) {
		if (mft[callee_id][caller_id].enable_req ==
		    mft[callee_id][caller_id].enable_ack) {
			mft[callee_id][caller_id].enable_req++;
		}

		mayfly_enable_cb(caller_id, callee_id, enable);
	} else {
		if (mft[callee_id][caller_id].disable_req ==
		    mft[callee_id][caller_id].disable_ack) {
			mft[callee_id][caller_id].disable_req++;

			/* set mayfly callee pending */
			mfp[callee_id] = 1U;

			/* pend the callee for execution */
			mayfly_pend(caller_id, callee_id);
		}
	}
}

u32_t mayfly_enqueue(u8_t caller_id, u8_t callee_id, u8_t chain,
			struct mayfly *m)
{
	u8_t state;
	u8_t ack;

	chain = chain || !mayfly_prio_is_equal(caller_id, callee_id) ||
		!mayfly_is_enabled(caller_id, callee_id) ||
		(mft[callee_id][caller_id].disable_req !=
		 mft[callee_id][caller_id].disable_ack);

	/* shadow the ack */
	ack = m->_ack;

	/* already in queue */
	state = (m->_req - ack) & 0x03;
	if (state != 0U) {
		if (chain) {
			if (state != 1U) {
				/* mark as ready in queue */
				m->_req = ack + 1;

				goto mayfly_enqueue_pend;
			}

			/* already ready */
			return 1;
		}

		/* mark as done in queue, and fall thru */
		m->_req = ack + 2;
	}

	/* handle mayfly(s) that can be inline */
	if (!chain) {
		/* call fp */
		m->fp(m->param);

		return 0;
	}

	/* new, add as ready in the queue */
	m->_req = ack + 1;
	memq_enqueue(m->_link, m, &mft[callee_id][caller_id].tail);

mayfly_enqueue_pend:
	/* set mayfly callee pending */
	mfp[callee_id] = 1U;

	/* pend the callee for execution */
	mayfly_pend(caller_id, callee_id);

	return 0;
}

static void dequeue(u8_t callee_id, u8_t caller_id, memq_link_t *link,
		    struct mayfly *m)
{
	u8_t req;

	req = m->_req;
	if (((req - m->_ack) & 0x03) != 1U) {
		u8_t ack;

#if defined(MAYFLY_UT)
		u32_t mayfly_ut_run_test(void);
		void mayfly_ut_mfy(void *param);

		if (_state && m->fp == mayfly_ut_mfy) {
			static u8_t single;

			if (!single) {
				single = 1U;
				mayfly_ut_run_test();
			}
		}
#endif /* MAYFLY_UT */

		/* dequeue mayfly struct */
		memq_dequeue(mft[callee_id][caller_id].tail,
			     &mft[callee_id][caller_id].head,
			     0);

		/* release link into dequeued mayfly struct */
		m->_link = link;

		/* reset mayfly state to idle */
		ack = m->_ack;
		m->_ack = req;

		/* re-insert, if re-pended by interrupt */
		if (((m->_req - ack) & 0x03) == 1U) {
#if defined(MAYFLY_UT)
			printk("%s: RACE\n", __func__);
#endif /* MAYFLY_UT */

			m->_ack = ack;
			memq_enqueue(link, m, &mft[callee_id][callee_id].tail);
		}
	}
}

void mayfly_run(u8_t callee_id)
{
	u8_t disable = 0U;
	u8_t enable = 0U;
	u8_t caller_id;

	if (!mfp[callee_id]) {
		return;
	}
	mfp[callee_id] = 0U;

	/* iterate through each caller queue to this callee_id */
	caller_id = MAYFLY_CALLER_COUNT;
	while (caller_id--) {
		memq_link_t *link;
		struct mayfly *m = 0;

		/* fetch mayfly in callee queue, if any */
		link = memq_peek(mft[callee_id][caller_id].head,
				 mft[callee_id][caller_id].tail,
				 (void **)&m);
		while (link) {
			u8_t state;

#if defined(MAYFLY_UT)
			_state = 0U;
#endif /* MAYFLY_UT */

			/* execute work if ready */
			state = (m->_req - m->_ack) & 0x03;
			if (state == 1U) {
#if defined(MAYFLY_UT)
				_state = 1U;
#endif /* MAYFLY_UT */

				/* mark mayfly as ran */
				m->_ack--;

				/* call the mayfly function */
				m->fp(m->param);
			}

			/* dequeue if not re-pended */
			dequeue(callee_id, caller_id, link, m);

			/* fetch next mayfly in callee queue, if any */
			link = memq_peek(mft[callee_id][caller_id].head,
					 mft[callee_id][caller_id].tail,
					 (void **)&m);

/**
 * When using cooperative thread implementation, an issue has been seen where
 * pended mayflies are never executed in certain scenarios.
 * This happens when mayflies with higher caller_id are constantly pended, in
 * which case lower value caller ids never get to be executed.
 * By allowing complete traversal of mayfly queues for all caller_ids, this
 * does not happen, however this means that more than one mayfly function is
 * potentially executed in a mayfly_run(), with added execution time as
 * consequence.
 */
#if defined(CONFIG_BT_MAYFLY_YIELD_AFTER_CALL)
			/* yield out of mayfly_run if a mayfly function was
			 * called.
			 */
			if (state == 1U) {
				/* pend callee (tailchain) if mayfly queue is
				 * not empty or all caller queues are not
				 * processed.
				 */
				if (caller_id || link) {
					/* set mayfly callee pending */
					mfp[callee_id] = 1U;

					/* pend the callee for execution */
					mayfly_pend(callee_id, callee_id);

					return;
				}
			}
#endif
		}

		if (mft[callee_id][caller_id].disable_req !=
		    mft[callee_id][caller_id].disable_ack) {
			disable = 1U;

			mft[callee_id][caller_id].disable_ack =
				mft[callee_id][caller_id].disable_req;
		}

		if (mft[callee_id][caller_id].enable_req !=
		    mft[callee_id][caller_id].enable_ack) {
			enable = 1U;

			mft[callee_id][caller_id].enable_ack =
				mft[callee_id][caller_id].enable_req;
		}
	}

	if (disable && !enable) {
		mayfly_enable_cb(callee_id, callee_id, 0);
	}
}

#if defined(MAYFLY_UT)
#define MAYFLY_CALL_ID_CALLER MAYFLY_CALL_ID_0
#define MAYFLY_CALL_ID_CALLEE MAYFLY_CALL_ID_2

void mayfly_ut_mfy(void *param)
{
	printk("%s: ran.\n", __func__);

	(*((u32_t *)param))++;
}

void mayfly_ut_test(void *param)
{
	static u32_t *count;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mayfly_ut_mfy};
	u32_t err;

	printk("%s: req= %u, ack= %u\n", __func__, mfy._req, mfy._ack);

	if (param) {
		count = param;
	}

	mfy.param = count;

	err = mayfly_enqueue(MAYFLY_CALL_ID_CALLER, MAYFLY_CALL_ID_CALLEE, 1,
			     &mfy);
	if (err) {
		printk("%s: FAILED (%u).\n", __func__, err);
	} else {
		printk("%s: SUCCESS.\n", __func__);
	}
}

u32_t mayfly_ut_run_test(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mayfly_ut_test};
	u32_t err;

	printk("%s: req= %u, ack= %u\n", __func__, mfy._req, mfy._ack);

	err = mayfly_enqueue(MAYFLY_CALL_ID_CALLEE, MAYFLY_CALL_ID_CALLER, 0,
			     &mfy);

	if (err) {
		printk("%s: FAILED.\n", __func__);
		return err;
	}

	printk("%s: SUCCESS.\n", __func__);

	return 0;
}

u32_t mayfly_ut(void)
{
	static u32_t count;
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, &count, mayfly_ut_test};
	u32_t err;

	printk("%s: req= %u, ack= %u\n", __func__, mfy._req, mfy._ack);

	err = mayfly_enqueue(MAYFLY_CALL_ID_PROGRAM, MAYFLY_CALL_ID_CALLER, 0,
			     &mfy);

	if (err) {
		printk("%s: FAILED.\n", __func__);
		return err;
	}

	printk("%s: count = %u.\n", __func__, count);
	printk("%s: SUCCESS.\n", __func__);
	return 0;
}
#endif /* MAYFLY_UT */
