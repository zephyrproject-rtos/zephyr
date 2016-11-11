/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

#include "hal_rtc.h"
#include "hal_work.h"
#include "work.h"

#include "ticker.h"

#include "debug.h"

/*****************************************************************************
 * Defines
 ****************************************************************************/
#define TICKER_DOUBLE_BUFFER_SIZE 2
#define TICKER_RTC_CC_OFFSET_MIN  3

/*****************************************************************************
 * Types
 ****************************************************************************/
typedef void (*ticker_fp_sched) (uint8_t chain);
typedef void (*ticker_fp_compare_set) (uint32_t cc);

struct ticker_node {
	uint8_t next;

	uint8_t req;
	uint8_t ack;
	uint8_t force;
	uint32_t ticks_periodic;
	uint32_t ticks_to_expire;
	ticker_timeout_func timeout_func;
	void *context;

	uint16_t ticks_to_expire_minus;
	uint16_t ticks_slot;
	uint16_t lazy_periodic;
	uint16_t lazy_current;
	uint32_t remainder_periodic;
	uint32_t remainder_current;
};

enum ticker_user_op_type {
	TICKER_USER_OP_TYPE_NONE,
	TICKER_USER_OP_TYPE_IDLE_GET,
	TICKER_USER_OP_TYPE_SLOT_GET,
	TICKER_USER_OP_TYPE_START,
	TICKER_USER_OP_TYPE_UPDATE,
	TICKER_USER_OP_TYPE_STOP,
};

struct ticker_user_op_start {
	uint32_t ticks_at_start;
	uint32_t ticks_first;
	uint32_t ticks_periodic;
	uint32_t remainder_periodic;
	uint16_t lazy;
	uint16_t ticks_slot;
	ticker_timeout_func fp_timeout_func;
	void *context;
};

struct ticker_user_op_update {
	uint16_t ticks_drift_plus;
	uint16_t ticks_drift_minus;
	uint16_t ticks_slot_plus;
	uint16_t ticks_slot_minus;
	uint16_t lazy;
	uint8_t force;
};

struct ticker_user_op_slot_get {
	uint8_t *ticker_id;
	uint32_t *ticks_current;
	uint32_t *ticks_to_expire;
};

struct ticker_user_op {
	enum ticker_user_op_type op;
	uint8_t id;
	union {
		struct ticker_user_op_start start;
		struct ticker_user_op_update update;
		struct ticker_user_op_slot_get slot_get;
	} params;
	uint32_t status;
	ticker_op_func fp_op_func;
	void *op_context;
};

struct ticker_user {
	uint8_t count_user_op;
	uint8_t first;
	uint8_t middle;
	uint8_t last;
	struct ticker_user_op *user_op;
};

struct ticker_instance {
	struct ticker_node *node;
	struct ticker_user *user;
	uint8_t count_node;
	uint8_t count_user;
	uint8_t ticks_elapsed_first;
	uint8_t ticks_elapsed_last;
	uint32_t ticks_elapsed[TICKER_DOUBLE_BUFFER_SIZE];
	uint32_t ticks_current;
	uint8_t ticker_id_head;
	uint8_t ticker_id_slot_previous;
	uint16_t ticks_slot_previous;
	uint8_t job_guard;
	uint8_t worker_trigger;
	ticker_fp_sched fp_worker_sched;
	ticker_fp_sched fp_job_sched;
	ticker_fp_compare_set fp_compare_set;
};

/*****************************************************************************
 * Global instances
 ****************************************************************************/
static struct ticker_instance _instance[2];

/*****************************************************************************
 * Static Functions
 ****************************************************************************/
static uint8_t ticker_by_slot_get(struct ticker_node *node,
					uint8_t ticker_id_head,
					uint32_t ticks_slot)
{
	while (ticker_id_head != TICKER_NULL) {
		struct ticker_node *ticker;
		uint32_t ticks_to_expire;

		ticker = &node[ticker_id_head];
		ticks_to_expire = ticker->ticks_to_expire;

		if (ticks_slot <= ticks_to_expire) {
			return TICKER_NULL;
		}

		if (ticker->ticks_slot) {
			break;
		}

		ticks_slot -= ticks_to_expire;
		ticker_id_head = ticker->next;
	}

	return ticker_id_head;
}

static void ticker_by_next_slot_get(struct ticker_instance *instance,
				       uint8_t *ticker_id_head,
				       uint32_t *ticks_current,
				       uint32_t *ticks_to_expire)
{
	struct ticker_node *node;
	uint8_t _ticker_id_head;
	struct ticker_node *ticker;
	uint32_t _ticks_to_expire;

	node = instance->node;

	_ticker_id_head = *ticker_id_head;
	_ticks_to_expire = *ticks_to_expire;
	if ((_ticker_id_head == TICKER_NULL)
	    || (*ticks_current != instance->ticks_current)) {
		_ticker_id_head = instance->ticker_id_head;
		*ticks_current = instance->ticks_current;
		_ticks_to_expire = 0;
	} else {
		ticker = &node[_ticker_id_head];
		_ticker_id_head = ticker->next;
	}

	while ((_ticker_id_head != TICKER_NULL)
	       && ((ticker = &node[_ticker_id_head])->ticks_slot == 0)
	    ) {
		_ticks_to_expire += ticker->ticks_to_expire;
		_ticker_id_head = ticker->next;
	}

	if (_ticker_id_head != TICKER_NULL) {
		_ticks_to_expire += ticker->ticks_to_expire;
	}

	*ticker_id_head = _ticker_id_head;
	*ticks_to_expire = _ticks_to_expire;
}

static uint8_t ticker_enqueue(struct ticker_instance *instance,
				    uint8_t id)
{
	struct ticker_node *node;
	struct ticker_node *ticker_new;
	struct ticker_node *ticker_current;
	uint8_t previous;
	uint8_t current;
	uint8_t ticker_id_slot_previous;
	uint8_t collide;
	uint32_t ticks_to_expire;
	uint32_t ticks_to_expire_current;
	uint32_t ticks_slot_previous;

	node = &instance->node[0];
	ticker_new = &node[id];
	ticks_to_expire = ticker_new->ticks_to_expire;

	collide = ticker_id_slot_previous = TICKER_NULL;
	current = instance->ticker_id_head;
	previous = current;
	ticks_slot_previous = instance->ticks_slot_previous;
	while ((current != TICKER_NULL)
	       &&
	       (ticks_to_expire >
		(ticks_to_expire_current =
		 (ticker_current = &node[current])->ticks_to_expire))
	    ) {
		ticks_to_expire -= ticks_to_expire_current;

		if (ticker_current->ticks_slot != 0) {
			ticks_slot_previous = ticker_current->ticks_slot;
			ticker_id_slot_previous = current;
		} else {
			if (ticks_slot_previous > ticks_to_expire_current) {
				ticks_slot_previous -= ticks_to_expire_current;
			} else {
				ticks_slot_previous = 0;
			}
		}
		previous = current;
		current = ticker_current->next;
	}

	collide = ticker_by_slot_get(&node[0], current, ticks_to_expire +
				ticker_new->ticks_slot);

	if ((ticker_new->ticks_slot == 0)
	    || ((ticks_slot_previous <= ticks_to_expire)
		&& (collide == TICKER_NULL))
	    ) {
		ticker_new->ticks_to_expire = ticks_to_expire;
		ticker_new->next = current;

		if (previous == current) {
			instance->ticker_id_head = id;
		} else {
			node[previous].next = id;
		}

		if (current != TICKER_NULL) {
			node[current].ticks_to_expire -= ticks_to_expire;
		}
	} else {
		if (ticks_slot_previous > ticks_to_expire) {
			id = ticker_id_slot_previous;
		} else {
			id = collide;
		}
	}

	return id;
}

static uint32_t ticker_dequeue(struct ticker_instance *instance,
				uint8_t id)
{
	struct ticker_node *ticker_current;
	struct ticker_node *node;
	uint8_t previous;
	uint8_t current;
	uint32_t timeout;
	uint32_t total;

	/* find the ticker's position in ticker list */
	node = &instance->node[0];
	previous = instance->ticker_id_head;
	current = previous;
	total = 0;
	ticker_current = 0;
	while (current != TICKER_NULL) {

		ticker_current = &node[current];

		if (current == id) {
			break;
		}

		total += ticker_current->ticks_to_expire;
		previous = current;
		current = ticker_current->next;
	}

	/* ticker not in active list */
	if (current == TICKER_NULL) {
		return 0;
	}

	/* ticker is the first in the list */
	if (previous == current) {
		instance->ticker_id_head = ticker_current->next;
	}

	/* remaining timeout between next timeout */
	timeout = ticker_current->ticks_to_expire;

	/* link previous ticker with next of this ticker
	 * i.e. removing the ticker from list
	 */
	node[previous].next = ticker_current->next;

	/* if this is not the last ticker, increment the
	 * next ticker by this ticker timeout
	 */
	if (ticker_current->next != TICKER_NULL) {
		node[ticker_current->next].ticks_to_expire += timeout;
	}

	return (total + timeout);
}

static inline void ticker_worker(struct ticker_instance *instance)
{
	struct ticker_node *node;
	uint32_t ticks_elapsed;
	uint32_t ticks_expired;
	uint8_t ticker_id_head;

	/* Defer worker if job running */
	instance->worker_trigger = 1;
	if (instance->job_guard) {
		return;
	}

	/* If no tickers to expire, do nothing */
	if (instance->ticker_id_head == TICKER_NULL) {
		instance->worker_trigger = 0;

		return;
	}

	/* ticks_elapsed is collected here, job will use it */
	ticks_elapsed =
	    ticker_ticks_diff_get(rtc_tick_get(), instance->ticks_current);

	/* initialise actual elapsed ticks being consumed */
	ticks_expired = 0;

	/* auto variable containing the head of tickers expiring */
	ticker_id_head = instance->ticker_id_head;

	/* expire all tickers within ticks_elapsed and collect ticks_expired */
	node = &instance->node[0];
	while (ticker_id_head != TICKER_NULL) {
		struct ticker_node *ticker;
		uint32_t ticks_to_expire;

		/* auto variable for current ticker node */
		ticker = &node[ticker_id_head];

		/* Do nothing if ticker did not expire */
		ticks_to_expire = ticker->ticks_to_expire;
		if (ticks_elapsed < ticks_to_expire) {
			break;
		}

		/* decrement ticks_elapsed and collect expired ticks */
		ticks_elapsed -= ticks_to_expire;
		ticks_expired += ticks_to_expire;

		/* move to next ticker */
		ticker_id_head = ticker->next;

		/* skip if not scheduled to execute */
		if (((ticker->req - ticker->ack) & 0xff) != 1) {
			continue;
		}

		/* scheduled timeout is acknowledged to be complete */
		ticker->ack--;

		if (ticker->timeout_func) {
			DEBUG_TICKER_TASK(1);
			ticker->timeout_func(((instance->ticks_current +
					       ticks_expired -
					       ticker->ticks_to_expire_minus) &
					       0x00FFFFFF),
					     ticker->remainder_current,
					     ticker->lazy_current,
					     ticker->context);
			DEBUG_TICKER_TASK(0);
		}
	}

	/* queue the elapsed value */
	if (instance->ticks_elapsed_first == instance->ticks_elapsed_last) {
		uint8_t last;

		last = instance->ticks_elapsed_last + 1;
		if (last == TICKER_DOUBLE_BUFFER_SIZE) {
			last = 0;
		}
		instance->ticks_elapsed_last = last;
	}
	instance->ticks_elapsed[instance->ticks_elapsed_last] =
	    ticks_expired;

	instance->worker_trigger = 0;

	instance->fp_job_sched(1);
}

static void prepare_ticks_to_expire(struct ticker_node *ticker,
					uint32_t ticks_current,
					uint32_t ticks_at_start)
{
	uint32_t ticks_to_expire = ticker->ticks_to_expire;
	uint16_t ticks_to_expire_minus = ticker->ticks_to_expire_minus;

	/* Calculate ticks to expire for this new node */
	if (((ticks_at_start - ticks_current) & 0x00800000) == 0) {
		ticks_to_expire +=
		    ticker_ticks_diff_get(ticks_at_start, ticks_current);
	} else {
		uint32_t delta_current_start;

		delta_current_start =
		    ticker_ticks_diff_get(ticks_current, ticks_at_start);
		if (ticks_to_expire > delta_current_start) {
			ticks_to_expire -= delta_current_start;
		} else {
			ticks_to_expire_minus +=
			    (delta_current_start - ticks_to_expire);
			ticks_to_expire = 0;
		}
	}

	/* Handle any drifts requested */
	if (ticks_to_expire > ticks_to_expire_minus) {
		ticks_to_expire -= ticks_to_expire_minus;
		ticks_to_expire_minus = 0;
	} else {
		ticks_to_expire_minus -= ticks_to_expire;
		ticks_to_expire = 0;
	}

	ticker->ticks_to_expire = ticks_to_expire;
	ticker->ticks_to_expire_minus = ticks_to_expire_minus;
}

static uint8_t ticker_remainder_increment(struct ticker_node *ticker)
{
	ticker->remainder_current += ticker->remainder_periodic;
	if ((ticker->remainder_current < 0x80000000)
	    && (ticker->remainder_current > (30517578UL / 2))) {
		ticker->remainder_current -= 30517578UL;
		return 1;
	}
	return 0;
}

static uint8_t ticker_remainder_decrement(struct ticker_node *ticker)
{
	uint8_t decrement = 0;

	if ((ticker->remainder_current >= 0x80000000)
	    || (ticker->remainder_current <= (30517578UL / 2))) {
		decrement++;
		ticker->remainder_current += 30517578UL;
	}
	ticker->remainder_current -= ticker->remainder_periodic;

	return decrement;
}

static inline void ticker_job_node_update(struct ticker_node *ticker,
					struct ticker_user_op *user_op,
					uint32_t ticks_current,
					uint32_t ticks_elapsed,
					uint8_t *insert_head)
{
	uint32_t ticks_now;
	uint32_t ticks_to_expire = ticker->ticks_to_expire;

	ticks_now = rtc_tick_get();
	ticks_elapsed += ticker_ticks_diff_get(ticks_now, ticks_current);
	if (ticks_to_expire > ticks_elapsed) {
		ticks_to_expire -= ticks_elapsed;
	} else {
		ticker->ticks_to_expire_minus +=
		    (ticks_elapsed - ticks_to_expire);
		ticks_to_expire = 0;
	}

	if ((ticker->ticks_periodic != 0)
	    && (user_op->params.update.lazy != 0)
	    ) {
		user_op->params.update.lazy--;

		while ((ticks_to_expire > ticker->ticks_periodic)
		       && (ticker->lazy_current >
			   user_op->params.update.lazy)) {
			ticks_to_expire -=
			    (ticker->ticks_periodic +
			     ticker_remainder_decrement(ticker));
			ticker->lazy_current--;
		}

		while (ticker->lazy_current < user_op->params.update.lazy) {
			ticks_to_expire +=
			    ticker->ticks_periodic +
			    ticker_remainder_increment(ticker);
			ticker->lazy_current++;
		}

		ticker->lazy_periodic = user_op->params.update.lazy;
	}

	ticker->ticks_to_expire =
	    ticks_to_expire + user_op->params.update.ticks_drift_plus;
	ticker->ticks_to_expire_minus +=
	    user_op->params.update.ticks_drift_minus;
	prepare_ticks_to_expire(ticker, ticks_current, ticks_now);

	ticker->ticks_slot += user_op->params.update.ticks_slot_plus;
	if (ticker->ticks_slot > user_op->params.update.ticks_slot_minus) {
		ticker->ticks_slot -=
		    user_op->params.update.ticks_slot_minus;
	} else {
		ticker->ticks_slot = 0;
	}

	if (user_op->params.update.force != 0) {
		ticker->force = user_op->params.update.force;
	}

	ticker->next = *insert_head;
	*insert_head = user_op->id;
}

static inline uint8_t ticker_job_list_manage(
					struct ticker_instance *instance,
					uint32_t ticks_elapsed,
					uint8_t *insert_head)
{
	uint8_t pending;
	struct ticker_node *node;
	struct ticker_user *users;
	uint8_t count_user;

	pending = 0;
	node = &instance->node[0];
	users = &instance->user[0];
	count_user = instance->count_user;
	while (count_user--) {
		struct ticker_user *user;
		struct ticker_user_op *user_ops;

		user = &users[count_user];
		user_ops = &user->user_op[0];
		while (user->middle != user->last) {
			struct ticker_user_op *user_op;
			struct ticker_node *ticker;
			uint8_t state;
			uint8_t prev;
			uint8_t middle;

			user_op = &user_ops[user->middle];

			/* Traverse queue (no dequeue) */
			prev = user->middle;
			middle = user->middle + 1;
			if (middle == user->count_user_op) {
				middle = 0;
			}
			user->middle = middle;

			ticker = &node[user_op->id];

			/* if op is start, then skip update and stop ops */
			if (user_op->op < TICKER_USER_OP_TYPE_UPDATE) {
				continue;
			}

			/* determine the ticker state */
			state = (ticker->req - ticker->ack) & 0xff;

			/* if not started or update not required,
			 * set status and continue.
			 */
			if ((user_op->op > TICKER_USER_OP_TYPE_STOP)
			    || (state == 0)
			    || ((user_op->op == TICKER_USER_OP_TYPE_UPDATE)
				&&
				(user_op->params.update.ticks_drift_plus == 0)
				&&
				(user_op->params.update.ticks_drift_minus ==
				 0)
				&& (user_op->params.update.ticks_slot_plus ==
				    0)
				&& (user_op->params.update.ticks_slot_minus ==
				    0)
				&& (user_op->params.update.lazy == 0)
				&& (user_op->params.update.force == 0)
			    )
			    ) {
				user_op->op = TICKER_USER_OP_TYPE_NONE;
				user_op->status = TICKER_STATUS_FAILURE;
				if (user_op->fp_op_func) {
					user_op->fp_op_func(user_op->status,
							      user_op->
							      op_context);
				}

				continue;
			}

			/* Delete node, if not expired */
			if (state == 1) {
				ticker->ticks_to_expire =
				    ticker_dequeue(instance,
						      user_op->id);

				/* Handle drift of ticker by re-inserting
				 * it back.
				 */
				if (user_op->op ==
					TICKER_USER_OP_TYPE_UPDATE) {
					ticker_job_node_update(ticker,
							  user_op,
							  instance->
							  ticks_current,
							  ticks_elapsed,
							  insert_head);

					/* set schedule status of node
					 * as updating.
					 */
					ticker->req++;
				} else {
					/* reset schedule status of node */
					ticker->req = ticker->ack;

					if (instance->
					    ticker_id_slot_previous ==
					    user_op->id) {
						instance->
						    ticker_id_slot_previous =
						    TICKER_NULL;
						instance->
						    ticks_slot_previous = 0;
					}
				}

				/* op success, @todo update may fail during
				 * actual insert! need to design that yet.
				 */
				user_op->op = TICKER_USER_OP_TYPE_NONE;
				user_op->status = TICKER_STATUS_SUCCESS;
				if (user_op->fp_op_func) {
					user_op->fp_op_func(
							user_op->status,
							user_op->
							op_context);
				}
			} else {
				/* update on expired node requested, deferi
				 * update until bottom half finishes.
				 */
				/* sched job to run after worker bottom half.
				 */
				instance->fp_job_sched(1);

				/* Update the index upto which management is
				 * complete.
				 */
				user->middle = prev;

				pending = 1;

				break;
			}
		}
	}

	return pending;
}

static inline void ticker_job_worker_bottom_half(
					struct ticker_instance *instance,
					uint32_t ticks_previous,
					uint32_t ticks_elapsed,
					uint8_t *insert_head)
{
	struct ticker_node *node;
	uint32_t ticks_expired;

	node = &instance->node[0];
	ticks_expired = 0;
	while (instance->ticker_id_head != TICKER_NULL) {
		struct ticker_node *ticker;
		uint8_t id_expired;
		uint32_t ticks_to_expire;

		/* auto variable for current ticker node */
		id_expired = instance->ticker_id_head;
		ticker = &node[id_expired];

		/* Do nothing if ticker did not expire */
		ticks_to_expire = ticker->ticks_to_expire;
		if (ticks_elapsed < ticks_to_expire) {
			ticker->ticks_to_expire -= ticks_elapsed;
			break;
		}

		/* decrement ticks_elapsed and collect expired ticks */
		ticks_elapsed -= ticks_to_expire;
		ticks_expired += ticks_to_expire;

		/* decrement ticks_slot_previous */
		if (instance->ticks_slot_previous > ticks_to_expire) {
			instance->ticks_slot_previous -= ticks_to_expire;
		} else {
			instance->ticker_id_slot_previous = TICKER_NULL;
			instance->ticks_slot_previous = 0;
		}

		/* save current ticks_slot_previous */
		if (ticker->ticks_slot != 0) {
			instance->ticker_id_slot_previous = id_expired;
			instance->ticks_slot_previous = ticker->ticks_slot;
		}

		/* ticker expired, set ticks_to_expire zero */
		ticker->ticks_to_expire = 0;

		/* remove the expired ticker from head */
		instance->ticker_id_head = ticker->next;

		/* ticker will be restarted if periodic */
		if (ticker->ticks_periodic != 0) {
			uint32_t count;

			count = 1 + ticker->lazy_periodic;

			ticks_to_expire = 0;
			while (count--) {
				ticks_to_expire +=
				    ticker->ticks_periodic +
				    ticker_remainder_increment(ticker);
			}

			ticker->ticks_to_expire = ticks_to_expire;
			prepare_ticks_to_expire(ticker,
						    instance->ticks_current,
						    (ticks_previous +
						     ticks_expired));
			ticker->lazy_current = ticker->lazy_periodic;
			ticker->force = 0;

			ticker->next = *insert_head;
			*insert_head = id_expired;

			/* set schedule status of node as restarting. */
			ticker->req++;
		} else {
			/* reset schedule status of node */
			ticker->req = ticker->ack;
		}
	}
}

static inline void ticker_job_list_insert(
					struct ticker_instance *instance,
					uint8_t insert_head)
{
	struct ticker_node *node;
	struct ticker_user *users;
	uint8_t count_user;

	node = &instance->node[0];
	users = &instance->user[0];
	count_user = instance->count_user;
	while (count_user--) {
		struct ticker_user *user;
		uint8_t user_ops_first;

		user = &users[count_user];
		user_ops_first = user->first;
		while ((insert_head != TICKER_NULL)
		       || (user_ops_first != user->middle)
		    ) {
			uint8_t id_insert;
			uint8_t id_collide;
			struct ticker_user_op *user_op;
			enum ticker_user_op_type _user_op;
			struct ticker_node *ticker;
			uint32_t status;

			if (insert_head != TICKER_NULL) {
				id_insert = insert_head;
				ticker = &node[id_insert];
				insert_head = ticker->next;

				user_op = 0;
				_user_op = TICKER_USER_OP_TYPE_START;
			} else {
				uint8_t first;

				user_op = &user->user_op[user_ops_first];
				first = user_ops_first + 1;
				if (first == user->count_user_op) {
					first = 0;
				}
				user_ops_first = first;

				_user_op = user_op->op;
				id_insert = user_op->id;
				ticker = &node[id_insert];
				if (_user_op != TICKER_USER_OP_TYPE_START) {
					continue;
				}

				if (((ticker->req - ticker->ack) & 0xff) != 0) {
					user_op->op = TICKER_USER_OP_TYPE_NONE;
					user_op->status =
					    TICKER_STATUS_FAILURE;

					if (user_op->fp_op_func) {
						user_op->
						    fp_op_func(user_op->
							       status,
							       user_op->
							       op_context);
					}

					continue;
				}

				ticker->ticks_periodic =
				    user_op->params.start.ticks_periodic;
				ticker->remainder_periodic =
				    user_op->params.start.remainder_periodic;
				ticker->lazy_periodic =
				    user_op->params.start.lazy;
				ticker->ticks_slot =
				    user_op->params.start.ticks_slot;
				ticker->timeout_func =
				    user_op->params.start.fp_timeout_func;
				ticker->context =
				    user_op->params.start.context;

				ticker->ticks_to_expire =
				    user_op->params.start.ticks_first;
				ticker->ticks_to_expire_minus = 0;
				prepare_ticks_to_expire(ticker,
							    instance->
							    ticks_current,
							    user_op->params.
							    start.
							    ticks_at_start);

				ticker->remainder_current = 0;
				ticker->lazy_current = 0;
				ticker->force = 1;
			}

			/* Prepare to insert */
			ticker->next = TICKER_NULL;

			/* If insert collides advance to next interval */
			while (id_insert !=
			       (id_collide =
				ticker_enqueue(instance, id_insert))) {
				struct ticker_node *ticker_preempt;

				ticker_preempt = (id_collide != TICKER_NULL) ?
					&node[id_collide] : 0;

				if (ticker_preempt
				    && (ticker->force > ticker_preempt->force)
				    ) {
					/* dequeue and get the reminder of ticks
					 * to expire.
					 */
					ticker_preempt->ticks_to_expire =
					    ticker_dequeue(instance,
							      id_collide);

					/* unschedule node */
					ticker_preempt->req =
					    ticker_preempt->ack;

					/* enqueue for re-insertion */
					ticker_preempt->next = insert_head;
					insert_head = id_collide;
				} else if (ticker->ticks_periodic != 0) {
					ticker->ticks_to_expire +=
					    ticker->ticks_periodic +
					    ticker_remainder_increment
					    (ticker);
					ticker->lazy_current++;
				} else {
					break;
				}
			}

			/* Update flags */
			if (id_insert == id_collide) {
				ticker->req = ticker->ack + 1;

				status = TICKER_STATUS_SUCCESS;
			} else {
				status = TICKER_STATUS_FAILURE;
			}

			if (user_op) {
				user_op->op = TICKER_USER_OP_TYPE_NONE;
				user_op->status = status;

				if (user_op->fp_op_func) {
					user_op->fp_op_func(user_op->status,
							      user_op->
							      op_context);
				}
			}
		}
	}
}

static inline void ticker_job_list_inquire(
					struct ticker_instance *instance)
{
	struct ticker_user *users;
	uint8_t count_user;

	users = &instance->user[0];
	count_user = instance->count_user;
	while (count_user--) {
		struct ticker_user *user;

		user = &users[count_user];
		while (user->first != user->last) {
			struct ticker_user_op *user_op;
			ticker_op_func fp_op_func;
			uint8_t first;

			user_op = &user->user_op[user->first];
			fp_op_func = 0;

			switch (user_op->op) {
			case TICKER_USER_OP_TYPE_IDLE_GET:
				user_op->status =
				    TICKER_STATUS_SUCCESS;
				fp_op_func = user_op->fp_op_func;
				break;

			case TICKER_USER_OP_TYPE_SLOT_GET:
				ticker_by_next_slot_get(instance,
							   user_op->
							   params.
							   slot_get.
							   ticker_id,
							   user_op->
							   params.
							   slot_get.
							   ticks_current,
							   user_op->
							   params.
							   slot_get.
							   ticks_to_expire);

				user_op->status =
				    TICKER_STATUS_SUCCESS;
				fp_op_func = user_op->fp_op_func;
				break;

			default:
				/* do nothing for other ops */
				break;
			}

			if (fp_op_func) {
				fp_op_func(user_op->status,
					   user_op->op_context);
			}

			first = user->first + 1;
			if (first == user->count_user_op) {
				first = 0;
			}
			user->first = first;
		}
	}
}

static inline void ticker_job_compare_update(
					struct ticker_instance *instance,
					uint8_t ticker_id_old_head)
{
	struct ticker_node *ticker;
	struct ticker_node *node;
	uint32_t ticks_to_expire;
	uint32_t ctr_post;
	uint32_t ctr;
	uint32_t cc;
	uint32_t i;

	if (instance->ticker_id_head == TICKER_NULL) {
		if (rtc_stop() == 0) {
			instance->ticks_slot_previous = 0;
		}

		return;
	}

	if (ticker_id_old_head == TICKER_NULL) {
		uint32_t ticks_current;

		ticks_current = rtc_tick_get();

		if (rtc_start() == 0) {
			instance->ticks_current = ticks_current;
		}
	}

	node = &instance->node[0];
	ticker = &node[instance->ticker_id_head];
	ticks_to_expire = ticker->ticks_to_expire;

	/* Iterate few times, if required, to ensure that compare is
	 * correctly set to a future value. This is required in case
	 * the operation is pre-empted and current h/w counter runs
	 * ahead of compare value to be set.
	 */
	i = 10;
	do {
		uint32_t ticks_elapsed;

		LL_ASSERT(i);
		i--;

		ctr = rtc_tick_get();
		cc = instance->ticks_current;
		ticks_elapsed = ticker_ticks_diff_get(ctr, cc) +
				TICKER_RTC_CC_OFFSET_MIN;
		cc += ((ticks_elapsed < ticks_to_expire) ?
		       ticks_to_expire : ticks_elapsed);
		cc &= 0x00FFFFFF;

		instance->fp_compare_set(cc);

		ctr_post = rtc_tick_get();
	} while ((ticker_ticks_diff_get(ctr_post, ctr) +
		  TICKER_RTC_CC_OFFSET_MIN) > ticker_ticks_diff_get(cc, ctr));
}

static inline void ticker_job(struct ticker_instance *instance)
{
	uint8_t ticker_id_old_head;
	uint8_t insert_head;
	uint32_t ticks_elapsed;
	uint32_t ticks_previous;
	uint8_t flag_elapsed;
	uint8_t pending;
	uint8_t flag_compare_update;

	DEBUG_TICKER_JOB(1);

	/* Defer worker, as job is now running */
	if (instance->worker_trigger) {
		DEBUG_TICKER_JOB(0);

		return;
	}
	instance->job_guard = 1;

	/* Back up the previous known tick */
	ticks_previous = instance->ticks_current;

	/* Update current tick with the elapsed value from queue, and dequeue */
	if (instance->ticks_elapsed_first != instance->ticks_elapsed_last) {
		uint8_t first;

		first = instance->ticks_elapsed_first + 1;
		if (first == TICKER_DOUBLE_BUFFER_SIZE) {
			first = 0;
		}
		instance->ticks_elapsed_first = first;

		ticks_elapsed =
		    instance->ticks_elapsed[instance->ticks_elapsed_first];

		instance->ticks_current += ticks_elapsed;
		instance->ticks_current &= 0x00FFFFFF;

		flag_elapsed = 1;
	} else {
		/* No elapsed value in queue */
		flag_elapsed = 0;
		ticks_elapsed = 0;
	}

	/* Initialise internal re-insert list */
	insert_head = TICKER_NULL;

	/* Initialise flag used to update next compare value */
	flag_compare_update = 0;

	/* Remember the old head, so as to decide if new compare needs to be
	 * set.
	 */
	ticker_id_old_head = instance->ticker_id_head;

	/* Manage updates and deletions in ticker list */
	pending =
	    ticker_job_list_manage(instance, ticks_elapsed, &insert_head);

	/* Detect change in head of the list */
	if (instance->ticker_id_head != ticker_id_old_head) {
		flag_compare_update = 1;
	}

	/* Handle expired tickers */
	if (flag_elapsed) {
		ticker_job_worker_bottom_half(instance, ticks_previous,
						 ticks_elapsed, &insert_head);

		/* detect change in head of the list */
		if (instance->ticker_id_head != ticker_id_old_head) {
			flag_compare_update = 1;
		}
	}

	/* Handle insertions */
	ticker_job_list_insert(instance, insert_head);

	/* detect change in head of the list */
	if (instance->ticker_id_head != ticker_id_old_head) {
		flag_compare_update = 1;
	}

	/* Processing any list inquiries */
	if (!pending) {
		/* Handle inquiries */
		ticker_job_list_inquire(instance);
	}

	/* Permit worker job to run */
	instance->job_guard = 0;

	/* update compare if head changed */
	if (flag_compare_update) {
		ticker_job_compare_update(instance, ticker_id_old_head);
	}

	/* trigger worker if deferred */
	if (instance->worker_trigger) {
		instance->fp_worker_sched(1);
	}

	DEBUG_TICKER_JOB(0);
}

/*****************************************************************************
 * Instances Work Helpers
 ****************************************************************************/
static void ticker_instance0_worker_sched(uint8_t chain)
{
	static struct work instance0_worker_irq = {
		0, 0, 0, WORK_TICKER_WORKER0_IRQ, (work_fp) ticker_worker,
		&_instance[0]
	};

	/* return value not checked as we allow multiple calls to schedule
	 * before being actually needing the work to complete before new
	 * schedule.
	 */
	work_schedule(&instance0_worker_irq, chain);
}

static void ticker_instance0_job_sched(uint8_t chain)
{
	static struct work instance0_job_irq = {
		0, 0, 0, WORK_TICKER_JOB0_IRQ, (work_fp) ticker_job,
		&_instance[0]
	};

	/* return value not checked as we allow multiple calls to schedule
	 * before being actually needing the work to complete before new
	 * schedule.
	 */
	work_schedule(&instance0_job_irq, chain);
}

static void ticker_instance0_rtc_compare_set(uint32_t value)
{
	rtc_compare_set(0, value);
}

static void ticker_instance1_worker_sched(uint8_t chain)
{
	static struct work instance1_worker_irq = {
		0, 0, 0, WORK_TICKER_WORKER1_IRQ, (work_fp) ticker_worker,
		&_instance[1]
	};

	/* return value not checked as we allow multiple calls to schedule
	 * before being actually needing the work to complete before new
	 * schedule.
	 */
	work_schedule(&instance1_worker_irq, chain);
}

static void ticker_instance1_job_sched(uint8_t chain)
{
	static struct work instance1_job_irq = {
		0, 0, 0, WORK_TICKER_JOB1_IRQ, (work_fp) ticker_job,
		&_instance[1]
	};

	/* return value not checked as we allow multiple calls to schedule
	 * before being actually needing the work to complete before new
	 * schedule.
	 */
	work_schedule(&instance1_job_irq, chain);
}

static void ticker_instance1_rtc_compare_set(uint32_t value)
{
	rtc_compare_set(1, value);
}

/*****************************************************************************
 * Public Interface
 ****************************************************************************/
uint32_t ticker_init(uint8_t instance_index, uint8_t count_node, void *node,
		    uint8_t count_user, void *user, uint8_t count_op,
		    void *user_op)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user *users;
	struct ticker_user_op *user_op_ =
				(struct ticker_user_op *)user_op;

	if ((sizeof(struct ticker_node) != TICKER_NODE_T_SIZE)
	    || (sizeof(struct ticker_user) != TICKER_USER_T_SIZE)
	    || (sizeof(struct ticker_user_op) != TICKER_USER_OP_T_SIZE)
	    ) {
		return TICKER_STATUS_FAILURE;
	}

	switch (instance_index) {
	case 0:
		instance->fp_worker_sched =
		    ticker_instance0_worker_sched;
		instance->fp_job_sched =
		    ticker_instance0_job_sched;
		instance->fp_compare_set =
		    ticker_instance0_rtc_compare_set;
		break;

	case 1:
		instance->fp_worker_sched =
		    ticker_instance1_worker_sched;
		instance->fp_job_sched =
		    ticker_instance1_job_sched;
		instance->fp_compare_set =
		    ticker_instance1_rtc_compare_set;
		break;

	default:
		return TICKER_STATUS_FAILURE;
	}

	instance->count_node = count_node;
	instance->node = node;

	instance->count_user = count_user;
	instance->user = user;

	/** @todo check if enough ticker_user_op supplied */

	users = &instance->user[0];
	while (count_user--) {
		users[count_user].user_op = user_op_;
		user_op_ += users[count_user].count_user_op;
		count_op -= users[count_user].count_user_op;
	}

	if (count_op) {
		return TICKER_STATUS_FAILURE;
	}

	instance->ticker_id_head = TICKER_NULL;
	instance->ticker_id_slot_previous = TICKER_NULL;
	instance->ticks_slot_previous = 0;
	instance->ticks_current = 0;
	instance->ticks_elapsed_first = 0;
	instance->ticks_elapsed_last = 0;

	rtc_init();

	return TICKER_STATUS_SUCCESS;
}

void ticker_trigger(uint8_t instance_index)
{
	if (_instance[instance_index].fp_worker_sched) {
		_instance[instance_index].fp_worker_sched(1);
	}
}

uint32_t ticker_start(uint8_t instance_index, uint8_t user_id,
		     uint8_t _ticker_id, uint32_t ticks_anchor,
		     uint32_t ticks_first, uint32_t ticks_periodic,
		     uint32_t remainder_periodic, uint16_t lazy,
		     uint16_t ticks_slot,
		     ticker_timeout_func ticker_timeout_func, void *context,
		     ticker_op_func fp_op_func, void *op_context)
{
	uint8_t last;
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user *user;
	struct ticker_user_op *user_op;

	user = &instance->user[user_id];

	last = user->last + 1;
	if (last == user->count_user_op) {
		last = 0;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_START;
	user_op->id = _ticker_id;
	user_op->params.start.ticks_at_start = ticks_anchor;
	user_op->params.start.ticks_first = ticks_first;
	user_op->params.start.ticks_periodic = ticks_periodic;
	user_op->params.start.remainder_periodic = remainder_periodic;
	user_op->params.start.ticks_slot = ticks_slot;
	user_op->params.start.lazy = lazy;
	user_op->params.start.fp_timeout_func = ticker_timeout_func;
	user_op->params.start.context = context;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->fp_job_sched(0);

	return user_op->status;
}

uint32_t ticker_update(uint8_t instance_index, uint8_t user_id,
		      uint8_t _ticker_id, uint16_t ticks_drift_plus,
		      uint16_t ticks_drift_minus, uint16_t ticks_slot_plus,
		      uint16_t ticks_slot_minus, uint16_t lazy, uint8_t force,
		      ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	uint8_t last;
	struct ticker_user *user;
	struct ticker_user_op *user_op;

	user = &instance->user[user_id];

	last = user->last + 1;
	if (last == user->count_user_op) {
		last = 0;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_UPDATE;
	user_op->id = _ticker_id;
	user_op->params.update.ticks_drift_plus = ticks_drift_plus;
	user_op->params.update.ticks_drift_minus = ticks_drift_minus;
	user_op->params.update.ticks_slot_plus = ticks_slot_plus;
	user_op->params.update.ticks_slot_minus = ticks_slot_minus;
	user_op->params.update.lazy = lazy;
	user_op->params.update.force = force;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->fp_job_sched(0);

	return user_op->status;
}

uint32_t ticker_stop(uint8_t instance_index, uint8_t user_id,
		    uint8_t _ticker_id, ticker_op_func fp_op_func,
		    void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	uint8_t last;
	struct ticker_user *user;
	struct ticker_user_op *user_op;

	user = &instance->user[user_id];

	last = user->last + 1;
	if (last == user->count_user_op) {
		last = 0;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_STOP;
	user_op->id = _ticker_id;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->fp_job_sched(0);

	return user_op->status;
}

uint32_t ticker_next_slot_get(uint8_t instance_index, uint8_t user_id,
			     uint8_t *_ticker_id,
			     uint32_t *ticks_current,
			     uint32_t *ticks_to_expire,
			     ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	uint8_t last;
	struct ticker_user *user;
	struct ticker_user_op *user_op;

	user = &instance->user[user_id];

	last = user->last + 1;
	if (last == user->count_user_op) {
		last = 0;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_SLOT_GET;
	user_op->id = TICKER_NULL;
	user_op->params.slot_get.ticker_id = _ticker_id;
	user_op->params.slot_get.ticks_current = ticks_current;
	user_op->params.slot_get.ticks_to_expire = ticks_to_expire;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->fp_job_sched(0);

	return user_op->status;
}

uint32_t ticker_job_idle_get(uint8_t instance_index, uint8_t user_id,
			    ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	uint8_t last;
	struct ticker_user *user;
	struct ticker_user_op *user_op;

	user = &instance->user[user_id];

	last = user->last + 1;
	if (last == user->count_user_op) {
		last = 0;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_IDLE_GET;
	user_op->id = TICKER_NULL;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->fp_job_sched(0);

	return user_op->status;
}

void ticker_job_sched(uint8_t instance_index)
{
	struct ticker_instance *instance = &_instance[instance_index];

	instance->fp_job_sched(0);
}

uint32_t ticker_ticks_now_get(void)
{
	return rtc_tick_get();
}

uint32_t ticker_ticks_diff_get(uint32_t ticks_now, uint32_t ticks_old)
{
	return ((ticks_now - ticks_old) & 0x00FFFFFF);
}
