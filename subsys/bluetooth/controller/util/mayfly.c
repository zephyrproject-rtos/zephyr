/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
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
	if (state != 0) {
		if (chain) {
			if (state != 1) {
				/* mark as ready in queue */
				m->_req = ack + 1;

				/* pend the callee for execution */
				mayfly_pend(caller_id, callee_id);

				return 0;
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

	/* pend the callee for execution */
	mayfly_pend(caller_id, callee_id);

	return 0;
}

void mayfly_run(u8_t callee_id)
{
	u8_t disable = 0;
	u8_t enable = 0;
	u8_t caller_id;

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
			u8_t req;

			/* execute work if ready */
			req = m->_req;
			state = (req - m->_ack) & 0x03;
			if (state == 1) {
				/* mark mayfly as ran */
				m->_ack--;

				/* call the mayfly function */
				m->fp(m->param);
			}

			/* dequeue if not re-pended */
			req = m->_req;
			if (((req - m->_ack) & 0x03) != 1) {
				memq_dequeue(mft[callee_id][caller_id].tail,
					     &mft[callee_id][caller_id].head,
					     0);

				/* release link into dequeued mayfly struct */
				m->_link = link;

				/* reset mayfly state to idle */
				m->_ack = req;
			}

			/* fetch next mayfly in callee queue, if any */
			link = memq_peek(mft[callee_id][caller_id].head,
					 mft[callee_id][caller_id].tail,
					 (void **)&m);

			/* yield out of mayfly_run if a mayfly function was
			 * called.
			 */
			if (state == 1) {
				/* pend callee (tailchain) if mayfly queue is
				 * not empty or all caller queues are not
				 * processed.
				 */
				if (caller_id || link) {
					mayfly_pend(callee_id, callee_id);

					return;
				}
			}
		}

		if (mft[callee_id][caller_id].disable_req !=
		    mft[callee_id][caller_id].disable_ack) {
			disable = 1;

			mft[callee_id][caller_id].disable_ack =
				mft[callee_id][caller_id].disable_req;
		}

		if (mft[callee_id][caller_id].enable_req !=
		    mft[callee_id][caller_id].enable_ack) {
			enable = 1;

			mft[callee_id][caller_id].enable_ack =
				mft[callee_id][caller_id].enable_req;
		}
	}

	if (disable && !enable) {
		mayfly_enable_cb(callee_id, callee_id, 0);
	}
}
