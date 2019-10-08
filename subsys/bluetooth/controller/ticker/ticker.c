/*
 * Copyright (c) 2016-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/types.h>
#include <soc.h>

#include "hal/cntr.h"
#include "hal/ticker.h"

#include "ticker.h"

#define LOG_MODULE_NAME bt_ctlr_ticker
#include "common/log.h"
#include "hal/debug.h"

/*****************************************************************************
 * Defines
 ****************************************************************************/
#define DOUBLE_BUFFER_SIZE 2

/*****************************************************************************
 * Types
 ****************************************************************************/

struct ticker_node {
	u8_t  next;			 /* Next ticker node */

	u8_t  req;			 /* Request counter */
	u8_t  ack;			 /* Acknowledge counter. Imbalance
					  * between req and ack indicates
					  * ongoing operation
					  */
	u8_t  force;			 /* If non-zero, node timeout should
					  * be forced at next expiration
					  */
	u32_t ticks_periodic;		 /* If non-zero, interval
					  * between expirations
					  */
	u32_t ticks_to_expire;		 /* Ticks until expiration */
	ticker_timeout_func timeout_func;/* User timeout function */
	void  *context;			 /* Context delivered to timeout
					  * function
					  */
	u32_t ticks_to_expire_minus;	 /* Negative drift correction */
	u32_t ticks_slot;		 /* Air-time reservation for node */
	u16_t lazy_periodic;		 /* Number of timeouts to allow
					  * skipping
					  */
	u16_t lazy_current;		 /* Current number of timeouts
					  * skipped = slave latency
					  */
	u32_t remainder_periodic;	 /* Sub-microsecond tick remainder
					  * for each period
					  */
	u32_t remainder_current;	 /* Current sub-microsecond tick
					  * remainder
					  */
#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
	u8_t  must_expire;		 /* Node must expire, even if it
					  * collides with other nodes
					  */
	s8_t  priority;			 /* Ticker node priority. 0 is default.
					  * Lower value is higher priority
					  */
#endif /* CONFIG_BT_TICKER_COMPATIBILITY_MODE */
};

/* Operations to be performed in ticker_job.
 * Possible values for field "op" in struct ticker_user_op
 */
#define TICKER_USER_OP_TYPE_NONE         0
#define TICKER_USER_OP_TYPE_IDLE_GET     1
#define TICKER_USER_OP_TYPE_SLOT_GET     2
#define TICKER_USER_OP_TYPE_PRIORITY_SET 3
#define TICKER_USER_OP_TYPE_START        4
#define TICKER_USER_OP_TYPE_UPDATE       5
#define TICKER_USER_OP_TYPE_STOP         6

/* User operation data structure for start opcode. Used for passing start
 * requests to ticker_job
 */
struct ticker_user_op_start {
	u32_t ticks_at_start;		/* Anchor ticks (absolute) */
	u32_t ticks_first;		/* Initial timeout ticks */
	u32_t ticks_periodic;		/* Ticker period ticks */
	u32_t remainder_periodic;	/* Sub-microsecond tick remainder */
	u16_t lazy;			/* Periodic latency in number of
					 * periods
					 */
	u32_t ticks_slot;		/* Air-time reservation ticks */
	ticker_timeout_func fp_timeout_func; /* Timeout callback function */
	void  *context;			/* Context passed in timeout callback */
};

/* User operation data structure for update opcode. Used for passing update
 * requests to ticker_job
 */
struct ticker_user_op_update {
	u32_t ticks_drift_plus;		/* Requested positive drift in ticks */
	u32_t ticks_drift_minus;	/* Requested negatice drift in ticks */
	u32_t ticks_slot_plus;		/* Number of ticks to add to slot
					 * reservation (air-time)
					 */
	u32_t ticks_slot_minus;		/* Number of ticks to subtract from
					 * slot reservation (air-time)
					 */
	u16_t lazy;			/* Slave latency:
					 *  0: Do nothing
					 *  1: latency = 0
					 * >1: latency = lazy - 1
					 */
	u8_t  force;			/* Force update */
};

/* User operation data structure for slot_get opcode. Used for passing request
 * to get next ticker with slot ticks via ticker_job
 */
struct ticker_user_op_slot_get {
	u8_t  *ticker_id;
	u32_t *ticks_current;
	u32_t *ticks_to_expire;
};

/* User operation data structure for priority_set opcode. Used for passing
 * request to set ticker node priority via ticker_job
 */
struct ticker_user_op_priority_set {
	s8_t priority;		   /* Node priority. Defaults to 0 */
};

/* User operation top level data structure. Used for passing requests to
 * ticker_job
 */
struct ticker_user_op {
	u8_t op;		   /* User operation */
	u8_t id;		   /* Ticker node id */
	union {
		struct ticker_user_op_start start;
		struct ticker_user_op_update update;
		struct ticker_user_op_slot_get slot_get;
		struct ticker_user_op_priority_set priority_set;
	} params;		   /* User operation parameters */
	u32_t status;		   /* Operation result */
	ticker_op_func fp_op_func; /* Operation completion callback */
	void  *op_context;	   /* Context passed in completion callback */
};

/* User data structure for operations
 */
struct ticker_user {
	u8_t count_user_op;	   /* Number of user operation slots */
	u8_t first;		   /* Slot index of first user operation */
	u8_t middle;		   /* Slot index of last managed user op.
				    * Updated by ticker_job_list_manage
				    * for use in ticker_job_list_insert
				    */
	u8_t last;		   /* Slot index of last user operation */
	struct ticker_user_op *user_op; /* Pointer to user operation array */
};

/* Ticker instance
 */
struct ticker_instance {
	struct ticker_node *nodes; /* Pointer to ticker nodes */
	struct ticker_user *users; /* Pointer to user nodes */
	u8_t  count_node;	   /* Number of ticker nodes */
	u8_t  count_user;	   /* Number of user nodes */
	u8_t  ticks_elapsed_first; /* Index from which elapsed ticks count is
				    * pulled
				    */
	u8_t  ticks_elapsed_last;  /* Index to which elapsed ticks count is
				    * pushed
				    */
	u32_t ticks_elapsed[DOUBLE_BUFFER_SIZE]; /* Buffer for elapsed ticks */
	u32_t ticks_current;	   /* Absolute ticks elapsed at last
				    * ticker_job
				    */
	u32_t ticks_slot_previous; /* Number of ticks previously reserved by a
				    * ticker node (active air-time)
				    */
	u8_t  ticker_id_slot_previous; /* Id of previous slot reserving ticker
					* node
					*/
	u8_t  ticker_id_head;      /* Index of first ticker node (next to
				    * expire)
				    */
	u8_t  job_guard;	   /* Flag preventing ticker_worker from
				    * running if ticker_job is active
				    */
	u8_t  worker_trigger;	   /* Flag preventing ticker_job from starting
				    * if ticker_worker was requested, and to
				    * trigger ticker_worker at end of job, if
				    * requested
				    */

	ticker_caller_id_get_cb_t caller_id_get_cb; /* Function for retrieving
						     * the caller id from user
						     * id
						     */
	ticker_sched_cb_t         sched_cb;	    /* Function for scheduling
						     * ticker_worker and
						     * ticker_job
						     */
	ticker_trigger_set_cb_t   trigger_set_cb;   /* Function for setting
						     * the trigger (compare
						     * value)
						     */
};

BUILD_ASSERT(sizeof(struct ticker_node)    == TICKER_NODE_T_SIZE);
BUILD_ASSERT(sizeof(struct ticker_user)    == TICKER_USER_T_SIZE);
BUILD_ASSERT(sizeof(struct ticker_user_op) == TICKER_USER_OP_T_SIZE);

/*****************************************************************************
 * Global instances
 ****************************************************************************/
#define TICKER_INSTANCE_MAX 1
static struct ticker_instance _instance[TICKER_INSTANCE_MAX];

/*****************************************************************************
 * Static Functions
 ****************************************************************************/

/**
 * @brief Update elapsed index
 *
 * @param ticks_elapsed_index Pointer to current index
 *
 * @internal
 */
static inline void ticker_next_elapsed(u8_t *ticks_elapsed_index)
{
	u8_t idx = *ticks_elapsed_index + 1;

	if (idx == DOUBLE_BUFFER_SIZE) {
		idx = 0U;
	}
	*ticks_elapsed_index = idx;
}

#if defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
/**
 * @brief Get ticker expiring in a specific slot
 *
 * @details Searches for a ticker which expires in a specific slot starting
 * at 'ticks_slot'.
 *
 * @param node           Pointer to ticker node array
 * @param ticker_id_head Id of initial ticker node
 * @param ticks_slot     Ticks indicating slot to get
 *
 * @return Id of ticker expiring within slot or TICKER_NULL
 * @internal
 */
static u8_t ticker_by_slot_get(struct ticker_node *node, u8_t ticker_id_head,
			       u32_t ticks_slot)
{
	while (ticker_id_head != TICKER_NULL) {
		struct ticker_node *ticker;
		u32_t ticks_to_expire;

		ticker = &node[ticker_id_head];
		ticks_to_expire = ticker->ticks_to_expire;

		if (ticks_slot <= ticks_to_expire) {
			/* Next ticker expiration is outside the checked slot */
			return TICKER_NULL;
		}

		if (ticker->ticks_slot) {
			/* This ticker node has slot defined and expires within
			 * checked slot
			 */
			break;
		}

		ticks_slot -= ticks_to_expire;
		ticker_id_head = ticker->next;
	}

	return ticker_id_head;
}
#endif /* CONFIG_BT_TICKER_COMPATIBILITY_MODE */

/**
 * @brief Get next ticker with slot ticks
 *
 * @details Gets the next ticker which has slot ticks specified and
 * return the ticker id and accumulated ticks until expiration. If no
 * ticker nodes have slot ticks, the next ticker node is returned.
 * If no head id is provided (TICKER_NULL) the first node is returned.
 *
 * @param instance          Pointer to ticker instance
 * @param ticker_id_head    Pointer to id of first ticker node [in/out]
 * @param ticks_current     Pointer to current ticks count [in/out]
 * @param ticks_to_expire   Pointer to ticks to expire [in/out]
 *
 * @internal
 */
static void ticker_by_next_slot_get(struct ticker_instance *instance,
				    u8_t *ticker_id_head, u32_t *ticks_current,
				    u32_t *ticks_to_expire)
{
	struct ticker_node *ticker;
	struct ticker_node *node;
	u32_t _ticks_to_expire;
	u8_t _ticker_id_head;

	node = instance->nodes;

	_ticker_id_head = *ticker_id_head;
	_ticks_to_expire = *ticks_to_expire;
	if ((_ticker_id_head == TICKER_NULL) ||
	    (*ticks_current != instance->ticks_current)) {
		/* Initialize with instance head */
		_ticker_id_head = instance->ticker_id_head;
		*ticks_current = instance->ticks_current;
		_ticks_to_expire = 0U;
	} else {
		/* Get ticker id for next node */
		ticker = &node[_ticker_id_head];
		_ticker_id_head = ticker->next;
	}

	/* Find first ticker node with slot ticks */
	while ((_ticker_id_head != TICKER_NULL) &&
	       ((ticker = &node[_ticker_id_head])->ticks_slot == 0U)) {
		/* Accumulate expire ticks */
		_ticks_to_expire += ticker->ticks_to_expire;
		_ticker_id_head = ticker->next;
	}

	if (_ticker_id_head != TICKER_NULL) {
		/* Add ticks for found ticker */
		_ticks_to_expire += ticker->ticks_to_expire;
	}

	*ticker_id_head = _ticker_id_head;
	*ticks_to_expire = _ticks_to_expire;
}

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
/**
 * @brief Enqueue ticker node
 *
 * @details Finds insertion point for new ticker node and inserts the
 * node in the linked node list.
 *
 * @param instance Pointer to ticker instance
 * @param id       Ticker node id to enqueue
 *
 * @return Id of enqueued ticker node
 * @internal
 */
static u8_t ticker_enqueue(struct ticker_instance *instance, u8_t id)
{
	struct ticker_node *ticker_current;
	struct ticker_node *ticker_new;
	u32_t ticks_to_expire_current;
	struct ticker_node *node;
	u32_t ticks_to_expire;
	u8_t previous;
	u8_t current;

	node = &instance->nodes[0];
	ticker_new = &node[id];
	ticks_to_expire = ticker_new->ticks_to_expire;
	current = instance->ticker_id_head;

	/* Find insertion point for new ticker node and adjust ticks_to_expire
	 * relative to insertion point
	 */
	previous = TICKER_NULL;

	while ((current != TICKER_NULL) && (ticks_to_expire >=
		(ticks_to_expire_current =
		(ticker_current = &node[current])->ticks_to_expire))) {

		ticks_to_expire -= ticks_to_expire_current;

		/* Check for timeout in same tick - prioritize according to
		 * latency
		 */
		if (ticks_to_expire == 0 && (ticker_new->lazy_current >
					     ticker_current->lazy_current)) {
			ticks_to_expire = ticker_current->ticks_to_expire;
			break;
		}

		previous = current;
		current = ticker_current->next;
	}

	/* Link in new ticker node and adjust ticks_to_expire to relative value
	 */
	ticker_new->ticks_to_expire = ticks_to_expire;
	ticker_new->next = current;

	if (previous == TICKER_NULL) {
		instance->ticker_id_head = id;
	} else {
		node[previous].next = id;
	}

	if (current != TICKER_NULL) {
		node[current].ticks_to_expire -= ticks_to_expire;
	}

	return id;
}
#else /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */

/**
 * @brief Enqueue ticker node
 *
 * @details Finds insertion point for new ticker node and inserts the
 * node in the linked node list. However, if the new ticker node collides
 * with an existing node or the expiration is inside the previous slot,
 * the node is not inserted.
 *
 * @param instance Pointer to ticker instance
 * @param id       Ticker node id to enqueue
 *
 * @return Id of enqueued ticker node, or id of previous- or colliding
 * ticker node if new node was not enqueued
 * @internal
 */
static u8_t ticker_enqueue(struct ticker_instance *instance, u8_t id)
{
	struct ticker_node *ticker_current;
	struct ticker_node *ticker_new;
	u32_t ticks_to_expire_current;
	u8_t ticker_id_slot_previous;
	u32_t ticks_slot_previous;
	struct ticker_node *node;
	u32_t ticks_to_expire;
	u8_t previous;
	u8_t current;
	u8_t collide;

	node = &instance->nodes[0];
	ticker_new = &node[id];
	ticks_to_expire = ticker_new->ticks_to_expire;

	collide = ticker_id_slot_previous = TICKER_NULL;
	current = instance->ticker_id_head;
	previous = current;
	ticks_slot_previous = instance->ticks_slot_previous;

	/* Find insertion point for new ticker node and adjust ticks_to_expire
	 * relative to insertion point
	 */
	while ((current != TICKER_NULL) &&
	       (ticks_to_expire >
		(ticks_to_expire_current =
		 (ticker_current = &node[current])->ticks_to_expire))) {
		ticks_to_expire -= ticks_to_expire_current;

		if (ticker_current->ticks_slot != 0U) {
			ticks_slot_previous = ticker_current->ticks_slot;
			ticker_id_slot_previous = current;
		} else {
			if (ticks_slot_previous > ticks_to_expire_current) {
				ticks_slot_previous -= ticks_to_expire_current;
			} else {
				ticks_slot_previous = 0U;
			}
		}
		previous = current;
		current = ticker_current->next;
	}

	/* Check for collision for new ticker node at insertion point */
	collide = ticker_by_slot_get(&node[0], current,
				     ticks_to_expire + ticker_new->ticks_slot);

	if ((ticker_new->ticks_slot == 0U) ||
	    ((ticks_slot_previous <= ticks_to_expire) &&
	     (collide == TICKER_NULL))) {
		/* New ticker node has no slot ticks or there is no collision -
		 * link it in and adjust ticks_to_expire to relative value
		 */
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
		/* Collision - no ticker node insertion, set id to that of
		 * colliding node
		 */
		if (ticks_slot_previous > ticks_to_expire) {
			id = ticker_id_slot_previous;
		} else {
			id = collide;
		}
	}

	return id;
}
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */

/**
 * @brief Dequeue ticker node
 *
 * @details Finds extraction point for ticker node to be dequeued, unlinks
 * the node and adjusts the links and ticks_to_expire. Returns the ticks
 * until expiration for dequeued ticker node.
 *
 * @param instance Pointer to ticker instance
 * @param id       Ticker node id to dequeue
 *
 * @return Total ticks until expiration for dequeued ticker node, or 0 if
 * node was not found
 * @internal
 */
static u32_t ticker_dequeue(struct ticker_instance *instance, u8_t id)
{
	struct ticker_node *ticker_current;
	struct ticker_node *node;
	u8_t previous;
	u32_t timeout;
	u8_t current;
	u32_t total;

	/* Find the ticker's position in ticker node list while accumulating
	 * ticks_to_expire
	 */
	node = &instance->nodes[0];
	previous = instance->ticker_id_head;
	current = previous;
	total = 0U;
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

	if (current == TICKER_NULL) {
		/* Ticker not in active list */
		return 0;
	}

	if (previous == current) {
		/* Ticker is the first in the list */
		instance->ticker_id_head = ticker_current->next;
	}

	/* Remaining timeout between next timeout */
	timeout = ticker_current->ticks_to_expire;

	/* Link previous ticker with next of this ticker
	 * i.e. removing the ticker from list
	 */
	node[previous].next = ticker_current->next;

	/* If this is not the last ticker, increment the
	 * next ticker by this ticker timeout
	 */
	if (ticker_current->next != TICKER_NULL) {
		node[ticker_current->next].ticks_to_expire += timeout;
	}

	return (total + timeout);
}

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
/**
 * @brief Resolve ticker node collision
 *
 * @details Evaluates the provided ticker node against other queued nodes
 * and returns non-zero if the ticker node collides and should be skipped.
 * The following rules are checked:
 *   1) If the periodic latency is not yet exhausted, node is skipped
 *   2) If the node has highest possible priority, node is never skipped
 *   2) If the node will starve next node due to slot reservation
 *      overlap, node is skipped if:
 *      a) Next node has higher priority than current node
 *      b) Next node has more accumulated latency than the current node
 *      c) Next node is 'older' than current node and has same priority
 *      d) Next node has force flag set, and the current does not
 *
 * @param nodes         Pointer to ticker node array
 * @param ticker        Pointer to ticker to resolve
 *
 * @return 0 if no collision was detected. 1 if ticker node collides
 * with other ticker node of higher composite priority
 * @internal
 */
static u8_t ticker_resolve_collision(struct ticker_node *nodes,
				     struct ticker_node *ticker)
{
	u8_t  skipped = 0;
	s32_t lazy_current = ticker->lazy_current;

	if (ticker->lazy_periodic > lazy_current) {
		/* Programmed latency must be respected */
		skipped = 1;

	} else if ((ticker->priority != TICKER_PRIORITY_CRITICAL) &&
		   (ticker->next != TICKER_NULL)) {
		/* Check if this ticker node will starve next node which has
		 * latency or higher priority
		 */
		if (lazy_current >= ticker->lazy_periodic) {
			lazy_current -= ticker->lazy_periodic;
		}
		u8_t  id_head = ticker->next;
		u32_t acc_ticks_to_expire = 0;

		/* Age is time since last expiry */
		u32_t current_age = ticker->ticks_periodic +
				    (lazy_current * ticker->ticks_periodic);

		while (id_head != TICKER_NULL) {
			struct ticker_node *ticker_next = &nodes[id_head];

			/* We only care about nodes with slot reservation */
			if (ticker_next->ticks_slot == 0) {
				id_head = ticker_next->next;
				continue;
			}

			/* Accumulate ticks_to_expire for each node */
			acc_ticks_to_expire += ticker_next->ticks_to_expire;

			s32_t lazy_next = ticker_next->lazy_current;
			u8_t  lazy_next_periodic_skip =
				ticker_next->lazy_periodic > lazy_next;

			if (!lazy_next_periodic_skip) {
				lazy_next -= ticker_next->lazy_periodic;
			}

			/* Is the current and next node equal in priority? */
			u8_t equal_priority = ticker->priority ==
				ticker_next->priority;

			/* Age is time since last expiry */
			u32_t next_age = (ticker_next->ticks_periodic == 0U ?
					  0U :
					 (ticker_next->ticks_periodic -
					  ticker_next->ticks_to_expire)) +
					 (lazy_next *
					  ticker_next->ticks_periodic);

			/* Was the current node scheduled earlier? */
			u8_t current_is_older = current_age > next_age;
			/* Was next node scheduled earlier (legacy priority)? */
			u8_t next_is_older = next_age > current_age;

			/* Is force requested for next node (e.g. update) -
			 * more so than for current node?
			 */
			u8_t next_force = (ticker_next->force > ticker->force);

			/* Does next node have critical priority and should
			 * always be scheduled?
			 */
			u8_t next_is_critical = ticker_next->priority ==
				TICKER_PRIORITY_CRITICAL;

			/* Does next node have higher priority? */
			u8_t next_has_priority =
				(lazy_next - ticker_next->priority) >
				(lazy_current - ticker->priority);

			/* Check if next node is within this reservation slot
			 * and wins conflict resolution
			 */
			if (!lazy_next_periodic_skip &&
			    (acc_ticks_to_expire < ticker->ticks_slot) &&
			    (next_force ||
			     next_is_critical ||
			    (next_has_priority && !current_is_older) ||
			    (equal_priority && next_is_older))) {
				skipped = 1;
				break;
			}
			id_head = ticker_next->next;
		}
	}
	return skipped;
}
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */

/**
 * @brief Ticker worker
 *
 * @details Runs as upper half of ticker operation, triggered by a compare
 * match from the underlaying counter HAL, via the ticker_trigger function.
 * Traverses ticker nodes to find tickers expired since last job
 * execution. Expired (requested) ticker nodes have their timeout callback
 * functions called. Finally, a ticker job is enqueued. Invoked from the
 * ticker worker mayfly context (TICKER_MAYFLY_CALL_ID_WORKER)
 *
 * @param param Pointer to ticker instance
 *
 */
void ticker_worker(void *param)
{
	struct ticker_instance *instance = param;
	struct ticker_node *node;
	u32_t ticks_elapsed;
	u32_t ticks_expired;
	u8_t ticker_id_head;

	/* Defer worker if job running */
	instance->worker_trigger = 1U;
	if (instance->job_guard) {
		return;
	}

	/* If no tickers queued (active), do nothing */
	if (instance->ticker_id_head == TICKER_NULL) {
		instance->worker_trigger = 0U;
		return;
	}

	/* Get ticks elapsed since last job execution */
	ticks_elapsed = ticker_ticks_diff_get(cntr_cnt_get(),
					      instance->ticks_current);

	/* Initialize actual elapsed ticks being consumed */
	ticks_expired = 0U;

	/* Auto variable containing the head of tickers expiring */
	ticker_id_head = instance->ticker_id_head;

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
	/* Check if the previous ticker node which had air-time, is still
	 * active and has this time slot reserved
	 */
	u8_t slot_reserved = 0;

	if (instance->ticker_id_slot_previous != TICKER_NULL) {
		if (instance->ticks_slot_previous > ticks_elapsed) {
			/* This node intersects reserved slot */
			slot_reserved = 1;
		}
	}
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */

	/* Expire all tickers within ticks_elapsed and collect ticks_expired */
	node = &instance->nodes[0];

	while (ticker_id_head != TICKER_NULL) {
		struct ticker_node *ticker;
		u32_t ticks_to_expire;
		u8_t must_expire_skip;

		ticker = &node[ticker_id_head];

		/* Stop if ticker did not expire */
		ticks_to_expire = ticker->ticks_to_expire;
		if (ticks_elapsed < ticks_to_expire) {
			break;
		}

		/* Decrement ticks_elapsed and collect expired ticks */
		ticks_elapsed -= ticks_to_expire;
		ticks_expired += ticks_to_expire;

		/* Move to next ticker node */
		ticker_id_head = ticker->next;
		must_expire_skip = 0U;

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
		/* Check if node has slot reservation and resolve any collision
		 * with other ticker nodes
		 */
		if (ticker->ticks_slot != 0U &&
		   (slot_reserved || ticker_resolve_collision(node, ticker))) {
			ticker->lazy_current++;
			if ((ticker->must_expire == 0U) ||
			    (ticker->lazy_periodic >= ticker->lazy_current)) {
				/* Not a must-expire case or this is programmed
				 * latency. Skip this ticker node
				 */
				continue;
			}
			/* Continue but perform shallow expiry */
			must_expire_skip = 1U;
		}
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */
		/* Skip if not scheduled to execute */
		if (((ticker->req - ticker->ack) & 0xff) != 1U) {
			continue;
		}

		/* Scheduled timeout is acknowledged to be complete */
		ticker->ack--;

		if (ticker->timeout_func) {
			u32_t ticks_at_expire;

			ticks_at_expire = (instance->ticks_current +
					   ticks_expired -
					   ticker->ticks_to_expire_minus) &
					   HAL_TICKER_CNTR_MASK;

			DEBUG_TICKER_TASK(1);
			/* Invoke the timeout callback */
			ticker->timeout_func(ticks_at_expire,
					     ticker->remainder_current,
					     must_expire_skip ?
					     TICKER_LAZY_MUST_EXPIRE :
					     ticker->lazy_current,
					     ticker->context);
			DEBUG_TICKER_TASK(0);

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
			if (must_expire_skip == 0U) {
				/* Reset latency to periodic offset */
				ticker->lazy_current = 0U;
				ticker->force = 0U;
				if (ticker->ticks_slot != 0U) {
					/* Any further nodes will be skipped */
					slot_reserved = 1U;
				}
			}
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */
		}
	}

	/* Queue the elapsed ticks */
	if (instance->ticks_elapsed_first == instance->ticks_elapsed_last) {
		ticker_next_elapsed(&instance->ticks_elapsed_last);
	}
	instance->ticks_elapsed[instance->ticks_elapsed_last] = ticks_expired;

	instance->worker_trigger = 0U;

	/* Enqueue the ticker job with chain=1 (do not inline) */
	instance->sched_cb(TICKER_CALL_ID_WORKER, TICKER_CALL_ID_JOB, 1,
			   instance);
}

/**
 * @brief Prepare ticker node expiration
 *
 * @details Calculates the number of ticks until next expiration, taking
 * into consideration any negative drift correction.
 *
 * @param ticker         Pointer to ticker node
 * @param ticks_current  Current number of ticks (elapsed)
 * @param ticks_at_start Number of ticks at start (anchor)
 *
 * @internal
 */
static void ticks_to_expire_prep(struct ticker_node *ticker,
				 u32_t ticks_current, u32_t ticks_at_start)
{
	u32_t ticks_to_expire = ticker->ticks_to_expire;
	u32_t ticks_to_expire_minus = ticker->ticks_to_expire_minus;

	/* Calculate ticks to expire for this new node */
	if (!((ticks_at_start - ticks_current) & BIT(HAL_TICKER_CNTR_MSBIT))) {
		/* Most significant bit is 0 so ticks_at_start lies ahead of
		 * ticks_current: ticks_at_start >= ticks_current
		 */
		ticks_to_expire += ticker_ticks_diff_get(ticks_at_start,
							 ticks_current);
	} else {
		/* ticks_current > ticks_at_start
		 */
		u32_t delta_current_start;

		delta_current_start = ticker_ticks_diff_get(ticks_current,
							    ticks_at_start);
		if (ticks_to_expire > delta_current_start) {
			/* There's still time until expiration - subtract
			 * elapsed time
			 */
			ticks_to_expire -= delta_current_start;
		} else {
			/* Ticker node should have expired (we're late).
			 * Add 'lateness' to negative drift correction
			 * (ticks_to_expire_minus) and set ticks_to_expire
			 * to 0
			 */
			ticks_to_expire_minus +=
			    (delta_current_start - ticks_to_expire);
			ticks_to_expire = 0U;
		}
	}

	/* Handle negative drift correction */
	if (ticks_to_expire > ticks_to_expire_minus) {
		ticks_to_expire -= ticks_to_expire_minus;
		ticks_to_expire_minus = 0U;
	} else {
		ticks_to_expire_minus -= ticks_to_expire;
		ticks_to_expire = 0U;
	}

	/* Update ticker */
	ticker->ticks_to_expire = ticks_to_expire;
	ticker->ticks_to_expire_minus = ticks_to_expire_minus;
}

/**
 * @brief Increment remainder
 *
 * @details Calculates whether the remainder should increments expiration time
 * for above-microsecond precision counter HW. The remainder enables improved
 * ticker precision, but is disabled for for sub-microsecond precision
 * configurations.
 *
 * @param ticker Pointer to ticker node
 *
 * @return Returns 1 to indicate increment is due, otherwise 0
 * @internal
 */
static u8_t ticker_remainder_inc(struct ticker_node *ticker)
{
#ifdef HAL_TICKER_REMAINDER_RANGE
	ticker->remainder_current += ticker->remainder_periodic;
	if ((ticker->remainder_current < BIT(31)) &&
	    (ticker->remainder_current > (HAL_TICKER_REMAINDER_RANGE >> 1))) {
		ticker->remainder_current -= HAL_TICKER_REMAINDER_RANGE;
		return 1;
	}
	return 0;
#else
	return 0;
#endif
}

#if defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
/**
 * @brief Decrement remainder
 *
 * @details Calculates whether the remainder should decrements expiration time
 * for above-microsecond precision counter HW. The remainder enables improved
 * ticker precision, but is disabled for for sub-microsecond precision
 * configurations.
 *
 * @param ticker Pointer to ticker node
 *
 * @return Returns 1 to indicate decrement is due, otherwise 0
 * @internal
 */
static u8_t ticker_remainder_dec(struct ticker_node *ticker)
{
#ifdef HAL_TICKER_REMAINDER_RANGE
	u8_t decrement = 0U;

	if ((ticker->remainder_current >= BIT(31)) ||
	    (ticker->remainder_current <= (HAL_TICKER_REMAINDER_RANGE >> 1))) {
		decrement++;
		ticker->remainder_current += HAL_TICKER_REMAINDER_RANGE;
	}
	ticker->remainder_current -= ticker->remainder_periodic;
	return decrement;
#else
	return 0;
#endif
}
#endif /* CONFIG_BT_TICKER_COMPATIBILITY_MODE */

/**
 * @brief Invoke user operation callback
 *
 * @param user_op Pointer to user operation struct
 * @param status  User operation status to pass to callback
 *
 * @internal
 */
static void ticker_job_op_cb(struct ticker_user_op *user_op, u32_t status)
{
	user_op->op = TICKER_USER_OP_TYPE_NONE;
	user_op->status = status;
	if (user_op->fp_op_func) {
		user_op->fp_op_func(user_op->status, user_op->op_context);
	}
}

/**
 * @brief Update and insert ticker node
 *
 * @details Update ticker node with parameters passed in user operation.
 * After update, the ticker is inserted in front as new head.
 *
 * @param ticker	Pointer to ticker node
 * @param user_op	Pointer to user operation
 * @param ticks_current	Current ticker instance ticks
 * @param ticks_elapsed	Expired ticks at time of call
 * @param insert_head	Pointer to current head (id). Contains id
 *			from user operation upon exit
 * @internal
 */
static inline void ticker_job_node_update(struct ticker_node *ticker,
					  struct ticker_user_op *user_op,
					  u32_t ticks_current,
					  u32_t ticks_elapsed,
					  u8_t *insert_head)
{
	u32_t ticks_to_expire = ticker->ticks_to_expire;
	u32_t ticks_now;

	ticks_now = cntr_cnt_get();
	ticks_elapsed += ticker_ticks_diff_get(ticks_now, ticks_current);
	if (ticks_to_expire > ticks_elapsed) {
		ticks_to_expire -= ticks_elapsed;
	} else {
		ticker->ticks_to_expire_minus += ticks_elapsed -
						 ticks_to_expire;
		ticks_to_expire = 0U;
	}

	/* Update ticks_to_expire from latency (lazy) input */
	if ((ticker->ticks_periodic != 0U) &&
	    (user_op->params.update.lazy != 0U)) {
		user_op->params.update.lazy--;
#if defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
		while ((ticks_to_expire > ticker->ticks_periodic) &&
		       (ticker->lazy_current > user_op->params.update.lazy)) {
			ticks_to_expire -= ticker->ticks_periodic +
					   ticker_remainder_dec(ticker);
			ticker->lazy_current--;
		}

		while (ticker->lazy_current < user_op->params.update.lazy) {
			ticks_to_expire += ticker->ticks_periodic +
					   ticker_remainder_inc(ticker);
			ticker->lazy_current++;
		}
#endif /* CONFIG_BT_TICKER_COMPATIBILITY_MODE */
		ticker->lazy_periodic = user_op->params.update.lazy;
	}

	/* Update ticks_to_expire from drift input */
	ticker->ticks_to_expire = ticks_to_expire +
				  user_op->params.update.ticks_drift_plus;
	ticker->ticks_to_expire_minus +=
				user_op->params.update.ticks_drift_minus;

	ticks_to_expire_prep(ticker, ticks_current, ticks_now);

	/* Update ticks_slot parameter from plus/minus input */
	ticker->ticks_slot += user_op->params.update.ticks_slot_plus;
	if (ticker->ticks_slot > user_op->params.update.ticks_slot_minus) {
		ticker->ticks_slot -= user_op->params.update.ticks_slot_minus;
	} else {
		ticker->ticks_slot = 0U;
	}

	/* Update force parameter */
	if (user_op->params.update.force != 0U) {
		ticker->force = user_op->params.update.force;
	}

	ticker->next = *insert_head;
	*insert_head = user_op->id;
}

/**
 * @brief Manage user update operation
 *
 * @details Called by ticker_job to execute an update request, or set node
 * as done if request is not update. Invokes user operation callback before
 * exit.
 *
 * @param instance	Pointer to ticker instance
 * @param ticker	Pointer to ticker node
 * @param user_op	Pointer to user operation
 * @param ticks_elapsed Expired ticks at time of call
 * @param insert_head	Pointer to current head (id). For update operation,
 *			contains operation id upon exit
 * @internal
 */
static inline void ticker_job_node_manage(struct ticker_instance *instance,
					  struct ticker_node *ticker,
					  struct ticker_user_op *user_op,
					  u32_t ticks_elapsed,
					  u8_t *insert_head)
{
	/* Remove ticker node from list */
	ticker->ticks_to_expire = ticker_dequeue(instance, user_op->id);

	/* Handle update of ticker by re-inserting it back. */
	if (user_op->op == TICKER_USER_OP_TYPE_UPDATE) {
		ticker_job_node_update(ticker, user_op, instance->ticks_current,
				       ticks_elapsed, insert_head);

		/* Set schedule status of node
		 * as updating.
		 */
		ticker->req++;
	} else {
		/* Reset schedule status of node */
		ticker->req = ticker->ack;

		if (instance->ticker_id_slot_previous == user_op->id) {
			u32_t ticks_now = cntr_cnt_get();
			u32_t ticks_used;

			instance->ticker_id_slot_previous = TICKER_NULL;
			ticks_used = ticks_elapsed +
				ticker_ticks_diff_get(ticks_now,
						      instance->ticks_current);
			instance->ticks_slot_previous =	MIN(ticker->ticks_slot,
							    ticks_used);
		}
	}

	/* op success, @todo update may fail during
	 * actual insert! need to design that yet.
	 */
	ticker_job_op_cb(user_op, TICKER_STATUS_SUCCESS);
}

/**
 * @brief Manage user operations list
 *
 * @details Called by ticker_job to execute requested user operations. A
 * number of operation may be queued since last ticker_job. Only update and
 * stop operations are handled. Start is handled implicitly by inserting
 * the ticker node in ticker_job_list_insert.
 *
 * @param instance	Pointer to ticker instance
 * @param ticks_elapsed Expired ticks at time of call
 * @param insert_head	Pointer to current head (id). For update operation,
 *			contains operation id upon exit
 * @return Returns 1 if operations is pending, 0 if all operations are done.
 * @internal
 */
static inline u8_t ticker_job_list_manage(struct ticker_instance *instance,
					  u32_t ticks_elapsed,
					  u8_t *insert_head)
{
	u8_t pending;
	struct ticker_node *node;
	struct ticker_user *users;
	u8_t count_user;

	pending = 0U;
	node = &instance->nodes[0];
	users = &instance->users[0];
	count_user = instance->count_user;
	/* Traverse users - highest id first */
	while (count_user--) {
		struct ticker_user *user;
		struct ticker_user_op *user_ops;

		user = &users[count_user];
		user_ops = &user->user_op[0];
		/* Traverse user operation queue - middle to last (with wrap).
		 * This operation updates user->middle to be the past the last
		 * processed user operation. This is used later by
		 * ticker_job_list_insert, for handling user->first to middle.
		 */
		while (user->middle != user->last) {
			struct ticker_user_op *user_op;
			struct ticker_node *ticker;
			u8_t state;
			u8_t prev;
			u8_t middle;

			user_op = &user_ops[user->middle];

			/* Increment index and handle wrapping */
			prev = user->middle;
			middle = user->middle + 1;
			if (middle == user->count_user_op) {
				middle = 0U;
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
			if ((user_op->op > TICKER_USER_OP_TYPE_STOP) ||
			    (state == 0U) ||
			    ((user_op->op == TICKER_USER_OP_TYPE_UPDATE) &&
			     (user_op->params.update.ticks_drift_plus == 0U) &&
			     (user_op->params.update.ticks_drift_minus == 0U) &&
			     (user_op->params.update.ticks_slot_plus == 0U) &&
			     (user_op->params.update.ticks_slot_minus == 0U) &&
			     (user_op->params.update.lazy == 0U) &&
			     (user_op->params.update.force == 0U))) {
				ticker_job_op_cb(user_op,
						 TICKER_STATUS_FAILURE);
				continue;
			}

			/* Delete node, if not expired */
			if (state == 1U) {
				ticker_job_node_manage(instance, ticker,
						       user_op, ticks_elapsed,
						       insert_head);
			} else {
				/* Update on expired node requested, defering
				 * update until bottom half finishes.
				 */
				/* sched job to run after worker bottom half.
				 */
				instance->sched_cb(TICKER_CALL_ID_JOB,
						   TICKER_CALL_ID_JOB, 1,
						   instance);

				/* Update the index upto which management is
				 * complete.
				 */
				user->middle = prev;

				pending = 1U;
				break;
			}
		}
	}

	return pending;
}

/**
 * @brief Handle ticker node expirations
 *
 * @details Called by ticker_job to schedule next expirations. Expired ticker
 * nodes are removed from the active list, and re-inserted if periodic.
 *
 * @param instance	 Pointer to ticker instance
 * @param ticks_previous Absolute ticks at ticker_job start
 * @param ticks_elapsed  Expired ticks at time of call
 * @param insert_head	 Pointer to current head (id). Updated if nodes are
 *			 re-inserted
 * @internal
 */
static inline void ticker_job_worker_bh(struct ticker_instance *instance,
					u32_t ticks_previous,
					u32_t ticks_elapsed,
					u8_t *insert_head)
{
	struct ticker_node *node;
	u32_t ticks_expired;

	node = &instance->nodes[0];
	ticks_expired = 0U;
	while (instance->ticker_id_head != TICKER_NULL) {
		struct ticker_node *ticker;
		u32_t ticks_to_expire;
		u8_t id_expired;

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

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
		if (ticker->lazy_current != 0U) {
			instance->ticker_id_slot_previous = TICKER_NULL;
			instance->ticks_slot_previous = 0U;
		} else
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */
		{
			/* decrement ticks_slot_previous */
			if (instance->ticks_slot_previous > ticks_to_expire) {
				instance->ticks_slot_previous -=
				ticks_to_expire;
			} else {
				instance->ticker_id_slot_previous = TICKER_NULL;
				instance->ticks_slot_previous = 0U;
			}

			/* save current ticks_slot_previous */
			if (ticker->ticks_slot != 0U) {
				instance->ticker_id_slot_previous = id_expired;
				instance->ticks_slot_previous =
					ticker->ticks_slot;
			}
		}

		/* ticker expired, set ticks_to_expire zero */
		ticker->ticks_to_expire = 0U;

		/* remove the expired ticker from head */
		instance->ticker_id_head = ticker->next;

		/* ticker will be restarted if periodic */
		if (ticker->ticks_periodic != 0U) {
#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
			/* Reload ticks_to_expire with one period */
			ticker->ticks_to_expire  = ticker->ticks_periodic;
			ticker->ticks_to_expire += ticker_remainder_inc(ticker);

			ticks_to_expire_prep(ticker, instance->ticks_current,
					     (ticks_previous + ticks_expired));
#else /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */
			u32_t count;

			/* Prepare for next interval */
			ticks_to_expire = 0U;
			count = 1 + ticker->lazy_periodic;
			while (count--) {
				ticks_to_expire += ticker->ticks_periodic;
				ticks_to_expire += ticker_remainder_inc(ticker);
			}
			ticker->ticks_to_expire = ticks_to_expire;

			ticks_to_expire_prep(ticker, instance->ticks_current,
					     (ticks_previous + ticks_expired));

			/* Reset latency to periodic offset */
			ticker->lazy_current = ticker->lazy_periodic;
			ticker->force = 0U;
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */
			/* Add to insert list */
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

/**
 * @brief Prepare ticker node start
 *
 * @details Called by ticker_job to prepare ticker node start operation.
 *
 * @param ticker	Pointer to ticker node
 * @param user_op	Pointer to user operation
 * @param ticks_current Expired ticks at time of call
 *
 * @internal
 */
static inline void ticker_job_op_start(struct ticker_node *ticker,
				       struct ticker_user_op *user_op,
				       u32_t ticks_current)
{
	struct ticker_user_op_start *start = (void *)&user_op->params.start;

#if defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
	/* Must expire is not supported in compatibility mode */
	LL_ASSERT(start->lazy != TICKER_LAZY_MUST_EXPIRE);
#else
	ticker->must_expire = (start->lazy == TICKER_LAZY_MUST_EXPIRE) ? 1U :
			       0U;
#endif /* CONFIG_BT_TICKER_COMPATIBILITY_MODE */

	ticker->ticks_periodic = start->ticks_periodic;
	ticker->remainder_periodic = start->remainder_periodic;
	ticker->lazy_periodic = (start->lazy == TICKER_LAZY_MUST_EXPIRE) ? 0U :
				 start->lazy;
	ticker->ticks_slot = start->ticks_slot;
	ticker->timeout_func = start->fp_timeout_func;
	ticker->context = start->context;
	ticker->ticks_to_expire = start->ticks_first;
	ticker->ticks_to_expire_minus = 0U;
	ticks_to_expire_prep(ticker, ticks_current, start->ticks_at_start);
	ticker->remainder_current = 0U;
	ticker->lazy_current = 0U;
	ticker->force = 1U;
}

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
/**
 * @brief Insert new ticker node
 *
 * @details Called by ticker_job to insert a new ticker node. If node collides
 * with existing ticker nodes, either the new node is postponed, or colliding
 * node is un-scheduled. Decision is based on latency and the force-state of
 * individual nodes.
 *
 * @param instance    Pointer to ticker instance
 * @param id_insert   Id of ticker to insert
 * @param ticker      Pointer to ticker node to insert
 * @param insert_head Pointer to current head. Updated if colliding nodes
 *		      are un-scheduled
 * @internal
 */
static inline u32_t ticker_job_insert(struct ticker_instance *instance,
				      u8_t id_insert,
				      struct ticker_node *ticker,
				      u8_t *insert_head)
{
	ARG_UNUSED(insert_head);

	/* Prepare to insert */
	ticker->next = TICKER_NULL;

	/* Enqueue the ticker node */
	(void)ticker_enqueue(instance, id_insert);

	/* Inserted/Scheduled */
	ticker->req = ticker->ack + 1;

	return TICKER_STATUS_SUCCESS;
}
#else /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */

/**
 * @brief Insert new ticker node
 *
 * @details Called by ticker_job to insert a new ticker node. If node collides
 * with existing ticker nodes, either the new node is postponed, or colliding
 * node is un-scheduled. Decision is based on latency and the force-state of
 * individual nodes.
 *
 * @param instance    Pointer to ticker instance
 * @param id_insert   Id of ticker to insert
 * @param ticker      Pointer to ticker node to insert
 * @param insert_head Pointer to current head. Updated if colliding nodes
 *		      are un-scheduled
 * @internal
 */
static inline u32_t ticker_job_insert(struct ticker_instance *instance,
				      u8_t id_insert,
				      struct ticker_node *ticker,
				      u8_t *insert_head)
{
	struct ticker_node *node = &instance->nodes[0];
	u8_t id_collide;
	u16_t skip;

	/* Prepare to insert */
	ticker->next = TICKER_NULL;

	/* No. of times ticker has skipped its interval */
	if (ticker->lazy_current > ticker->lazy_periodic) {
		skip = ticker->lazy_current -
		       ticker->lazy_periodic;
	} else {
		skip = 0U;
	}

	/* If insert collides, remove colliding or advance to next interval */
	while (id_insert !=
	       (id_collide = ticker_enqueue(instance, id_insert))) {
		/* Check for collision */
		if (id_collide != TICKER_NULL) {
			struct ticker_node *ticker_collide = &node[id_collide];
			u16_t skip_collide;

			/* No. of times colliding ticker has skipped its
			 * interval.
			 */
			if (ticker_collide->lazy_current >
			    ticker_collide->lazy_periodic) {
				skip_collide = ticker_collide->lazy_current -
					       ticker_collide->lazy_periodic;
			} else {
				skip_collide = 0U;
			}

			/* Check if colliding node should be un-scheduled */
			if (ticker_collide->ticks_periodic &&
			    skip_collide <= skip &&
			    ticker_collide->force < ticker->force) {
				/* Dequeue and get the reminder of ticks
				 * to expire.
				 */
				ticker_collide->ticks_to_expire =
					ticker_dequeue(instance, id_collide);
				/* Unschedule node */
				ticker_collide->req = ticker_collide->ack;

				/* Enqueue for re-insertion */
				ticker_collide->next = *insert_head;
				*insert_head = id_collide;

				continue;
			}
		}

		/* occupied, try next interval */
		if (ticker->ticks_periodic != 0U) {
			ticker->ticks_to_expire += ticker->ticks_periodic +
						   ticker_remainder_inc(ticker);
			ticker->lazy_current++;

			/* Remove any accumulated drift (possibly added due to
			 * ticker job execution latencies).
			 */
			if (ticker->ticks_to_expire >
			    ticker->ticks_to_expire_minus) {
				ticker->ticks_to_expire -=
					ticker->ticks_to_expire_minus;
				ticker->ticks_to_expire_minus = 0U;
			} else {
				ticker->ticks_to_expire_minus -=
					ticker->ticks_to_expire;
				ticker->ticks_to_expire = 0U;
			}
		} else {
			return TICKER_STATUS_FAILURE;
		}
	}

	/* Inserted/Scheduled */
	ticker->req = ticker->ack + 1;

	return TICKER_STATUS_SUCCESS;
}
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */

/**
 * @brief Insert and start ticker nodes for all users
 *
 * @details Called by ticker_job to prepare, insert and start ticker nodes
 * for all users. Specifying insert_head to other than TICKER_NULL causes
 * that ticker node to be inserted first.
 *
 * @param instance    Pointer to ticker instance
 * @param insert_head Id of ticker node to insert, or TICKER_NULL if only
 *                    handle user operation inserts
 * @internal
 */
static inline void ticker_job_list_insert(struct ticker_instance *instance,
					  u8_t insert_head)
{
	struct ticker_node *node;
	struct ticker_user *users;
	u8_t count_user;

	node = &instance->nodes[0];
	users = &instance->users[0];
	count_user = instance->count_user;

	/* Iterate through all user ids */
	while (count_user--) {
		struct ticker_user_op *user_ops;
		struct ticker_user *user;
		u8_t user_ops_first;

		user = &users[count_user];
		user_ops = (void *)&user->user_op[0];
		user_ops_first = user->first;
		/* Traverse user operation queue - first to middle (wrap) */
		while ((insert_head != TICKER_NULL) ||
		       (user_ops_first != user->middle)) {
			struct ticker_user_op *user_op;
			struct ticker_node *ticker;
			u8_t id_insert;
			u32_t status;

			if (insert_head != TICKER_NULL) {
				/* Prepare insert of ticker node specified by
				 * insert_head
				 */
				id_insert = insert_head;
				ticker = &node[id_insert];
				insert_head = ticker->next;

				user_op = NULL;
			} else {
				/* Prepare insert of any ticker nodes requested
				 * via user operation TICKER_USER_OP_TYPE_START
				 */
				u8_t first;

				user_op = &user_ops[user_ops_first];
				first = user_ops_first + 1;
				if (first == user->count_user_op) {
					first = 0U;
				}
				user_ops_first = first;

				id_insert = user_op->id;
				ticker = &node[id_insert];
				if (user_op->op != TICKER_USER_OP_TYPE_START) {
					/* User operation is not start - skip
					 * to next operation
					 */
					continue;
				}

				if (((ticker->req -
				      ticker->ack) & 0xff) != 0U) {
					ticker_job_op_cb(user_op,
							 TICKER_STATUS_FAILURE);
					continue;
				}

				/* Prepare ticker for start */
				ticker_job_op_start(ticker, user_op,
						    instance->ticks_current);
			}

			/* Insert ticker node */
			status = ticker_job_insert(instance, id_insert, ticker,
						   &insert_head);

			if (user_op) {
				ticker_job_op_cb(user_op, status);
			}
		}
	}
}

/**
 * @brief Perform inquiry for specific user operation
 *
 * @param instance Pointer to ticker instance
 * @param uop	   Pointer to user operation
 *
 * @internal
 */
static inline void ticker_job_op_inquire(struct ticker_instance *instance,
					 struct ticker_user_op *uop)
{
	ticker_op_func fp_op_func;

	fp_op_func = NULL;
	switch (uop->op) {
	case TICKER_USER_OP_TYPE_SLOT_GET:
		ticker_by_next_slot_get(instance,
					uop->params.slot_get.ticker_id,
					uop->params.slot_get.ticks_current,
					uop->params.slot_get.ticks_to_expire);
		/* Fall-through */
	case TICKER_USER_OP_TYPE_IDLE_GET:
		uop->status = TICKER_STATUS_SUCCESS;
		fp_op_func = uop->fp_op_func;
		break;
#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
	case TICKER_USER_OP_TYPE_PRIORITY_SET:
		if (uop->id < instance->count_node) {
			struct ticker_node *node = instance->nodes;

			node[uop->id].priority =
				uop->params.priority_set.priority;
			uop->status = TICKER_STATUS_SUCCESS;
		} else {
			uop->status = TICKER_STATUS_FAILURE;
		}
		fp_op_func = uop->fp_op_func;
		break;
#endif
	default:
		/* do nothing for other ops */
		break;
	}

	if (fp_op_func) {
		fp_op_func(uop->status, uop->op_context);
	}
}

/**
 * @brief Check for pending inquiries for all users
 *
 * @details Run through all user operation lists, checking for pending
 * inquiries. Currently only two types of inquiries are supported:
 * TICKER_USER_OP_TYPE_SLOT_GET and TICKER_USER_OP_TYPE_IDLE_GET. The
 * function also supports user operation TICKER_USER_OP_TYPE_PRIORITY_SET.
 * This operation modifies the user->first index, indicating user operations
 * are complete.
 *
 * @param instance Pointer to ticker instance
 *
 * @internal
 */
static inline void ticker_job_list_inquire(struct ticker_instance *instance)
{
	struct ticker_user *users;
	u8_t count_user;

	users = &instance->users[0];
	count_user = instance->count_user;
	/* Traverse user operation queue - first to last (with wrap) */
	while (count_user--) {
		struct ticker_user_op *user_op;
		struct ticker_user *user;

		user = &users[count_user];
		user_op = &user->user_op[0];
		while (user->first != user->last) {
			u8_t first;

			ticker_job_op_inquire(instance, &user_op[user->first]);

			first = user->first + 1;
			if (first == user->count_user_op) {
				first = 0U;
			}
			user->first = first;
		}
	}
}

/**
 * @brief Update counter compare value (trigger)
 *
 * @details Updates trigger to the match next expiring ticker node. The
 * function takes into consideration that it may be preempted in the process,
 * and makes sure - by iteration - that compare value is set in the future
 * (with a margin).
 *
 * @param instance           Pointer to ticker instance
 * @param ticker_id_old_head Previous ticker_id_head
 *
 * @internal
 */
static inline void ticker_job_compare_update(struct ticker_instance *instance,
					     u8_t ticker_id_old_head)
{
	struct ticker_node *ticker;
	u32_t ticks_to_expire;
	u32_t ctr_post;
	u32_t ctr;
	u32_t cc;
	u32_t i;

	if (instance->ticker_id_head == TICKER_NULL) {
		if (cntr_stop() == 0) {
			instance->ticks_slot_previous = 0U;
		}
		return;
	}

	/* Check if this is the first update. If so, start the counter */
	if (ticker_id_old_head == TICKER_NULL) {
		u32_t ticks_current;

		ticks_current = cntr_cnt_get();

		if (cntr_start() == 0) {
			instance->ticks_current = ticks_current;
		}
	}

	ticker = &instance->nodes[instance->ticker_id_head];
	ticks_to_expire = ticker->ticks_to_expire;

	/* Iterate few times, if required, to ensure that compare is
	 * correctly set to a future value. This is required in case
	 * the operation is pre-empted and current h/w counter runs
	 * ahead of compare value to be set.
	 */
	i = 10U;
	do {
		u32_t ticks_elapsed;

		LL_ASSERT(i);
		i--;

		ctr = cntr_cnt_get();
		cc = instance->ticks_current;
		ticks_elapsed = ticker_ticks_diff_get(ctr, cc) +
				HAL_TICKER_CNTR_CMP_OFFSET_MIN +
				HAL_TICKER_CNTR_SET_LATENCY;
		cc += MAX(ticks_elapsed, ticks_to_expire);
		cc &= HAL_TICKER_CNTR_MASK;
		instance->trigger_set_cb(cc);

		ctr_post = cntr_cnt_get();
	} while ((ticker_ticks_diff_get(ctr_post, ctr) +
		  HAL_TICKER_CNTR_CMP_OFFSET_MIN) >
		  ticker_ticks_diff_get(cc, ctr));
}

/**
 * @brief Ticker job
 *
 * @details Runs the bottom half of the ticker, after ticker nodes have elapsed
 * or user operations requested. The ticker_job is responsible for removing and
 * re-inserting ticker nodes, based on next elapsing and periodicity of the
 * nodes. The ticker_job is also responsible for processing user operations,
 * i.e. requests for start, update, stop etc.
 * Invoked from the ticker job mayfly context (TICKER_MAYFLY_CALL_ID_JOB).
 *
 * @param param Pointer to ticker instance
 *
 * @internal
 */
void ticker_job(void *param)
{
	struct ticker_instance *instance = param;
	u8_t ticker_id_old_head;
	u8_t insert_head;
	u32_t ticks_elapsed;
	u32_t ticks_previous;
	u8_t flag_elapsed;
	u8_t pending;
	u8_t flag_compare_update;

	DEBUG_TICKER_JOB(1);

	/* Defer worker, as job is now running */
	if (instance->worker_trigger) {
		DEBUG_TICKER_JOB(0);
		return;
	}
	instance->job_guard = 1U;

	/* Back up the previous known tick */
	ticks_previous = instance->ticks_current;

	/* Update current tick with the elapsed value from queue, and dequeue */
	if (instance->ticks_elapsed_first != instance->ticks_elapsed_last) {
		ticker_next_elapsed(&instance->ticks_elapsed_first);

		ticks_elapsed =
		    instance->ticks_elapsed[instance->ticks_elapsed_first];

		instance->ticks_current += ticks_elapsed;
		instance->ticks_current &= HAL_TICKER_CNTR_MASK;

		flag_elapsed = 1U;
	} else {
		/* No elapsed value in queue */
		flag_elapsed = 0U;
		ticks_elapsed = 0U;
	}

	/* Initialise internal re-insert list */
	insert_head = TICKER_NULL;

	/* Initialise flag used to update next compare value */
	flag_compare_update = 0U;

	/* Remember the old head, so as to decide if new compare needs to be
	 * set.
	 */
	ticker_id_old_head = instance->ticker_id_head;

	/* Manage user operations (updates and deletions) in ticker list */
	pending = ticker_job_list_manage(instance, ticks_elapsed, &insert_head);

	/* Detect change in head of the list */
	if (instance->ticker_id_head != ticker_id_old_head) {
		flag_compare_update = 1U;
	}

	/* Handle expired tickers */
	if (flag_elapsed) {
		ticker_job_worker_bh(instance, ticks_previous, ticks_elapsed,
				     &insert_head);

		/* detect change in head of the list */
		if (instance->ticker_id_head != ticker_id_old_head) {
			flag_compare_update = 1U;
		}
	}

	/* Handle insertions */
	ticker_job_list_insert(instance, insert_head);

	/* detect change in head of the list */
	if (instance->ticker_id_head != ticker_id_old_head) {
		flag_compare_update = 1U;
	}

	/* Process any list inquiries */
	if (!pending) {
		/* Handle inquiries */
		ticker_job_list_inquire(instance);
	}

	/* Permit worker job to run */
	instance->job_guard = 0U;

	/* update compare if head changed */
	if (flag_compare_update) {
		ticker_job_compare_update(instance, ticker_id_old_head);
	}

	/* trigger worker if deferred */
	if (instance->worker_trigger) {
		instance->sched_cb(TICKER_CALL_ID_JOB, TICKER_CALL_ID_WORKER, 1,
				   instance);
	}

	DEBUG_TICKER_JOB(0);
}

/*****************************************************************************
 * Public Interface
 ****************************************************************************/

/**
 * @brief Initialize ticker instance
 *
 * @details Called by ticker instance client once to initialize the ticker.
 *
 * @param instance_index   Index of ticker instance
 * @param count_node	   Number of ticker nodes in node array
 * @param node		   Pointer to ticker node array
 * @param count_user	   Number of users in user array
 * @param user		   Pointer to user array of size count_user
 * @param count_op	   Number of user operations in user_op array
 * @param user_op	   Pointer to user operations array of size count_op
 * @param caller_id_get_cb Pointer to function for retrieving caller_id from
 *			   user id
 * @param sched_cb	   Pointer to function for scheduling ticker_worker
 *			   and ticker_job
 * @param trigger_set_cb   Pointer to function for setting the compare trigger
 *			   ticks value
 *
 * @return TICKER_STATUS_SUCCESS if initialization was successful, otherwise
 * TICKER_STATUS_FAILURE
 */
u32_t ticker_init(u8_t instance_index, u8_t count_node, void *node,
		  u8_t count_user, void *user, u8_t count_op, void *user_op,
		  ticker_caller_id_get_cb_t caller_id_get_cb,
		  ticker_sched_cb_t sched_cb,
		  ticker_trigger_set_cb_t trigger_set_cb)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op_ = (void *)user_op;
	struct ticker_user *users;

	if (instance_index >= TICKER_INSTANCE_MAX) {
		return TICKER_STATUS_FAILURE;
	}

	instance->count_node = count_node;
	instance->nodes = node;

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
	while (count_node--) {
		instance->nodes[count_node].priority = 0;
	}
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */

	instance->count_user = count_user;
	instance->users = user;

	/** @todo check if enough ticker_user_op supplied */

	users = &instance->users[0];
	while (count_user--) {
		users[count_user].user_op = user_op_;
		user_op_ += users[count_user].count_user_op;
		count_op -= users[count_user].count_user_op;
	}

	if (count_op) {
		return TICKER_STATUS_FAILURE;
	}

	instance->caller_id_get_cb = caller_id_get_cb;
	instance->sched_cb = sched_cb;
	instance->trigger_set_cb = trigger_set_cb;

	instance->ticker_id_head = TICKER_NULL;
	instance->ticker_id_slot_previous = TICKER_NULL;
	instance->ticks_slot_previous = 0U;
	instance->ticks_current = 0U;
	instance->ticks_elapsed_first = 0U;
	instance->ticks_elapsed_last = 0U;

	return TICKER_STATUS_SUCCESS;
}

/**
 * @brief Check if ticker instance is initialized
 *
 * @param instance_index Index of ticker instance
 *
 * @return true if ticker instance is initialized, false otherwise
 */
bool ticker_is_initialized(u8_t instance_index)
{
	return !!(_instance[instance_index].count_node);
}

/**
 * @brief Trigger the ticker worker
 *
 * @details Schedules the ticker_worker upper half by invoking the
 * corresponding mayfly.
 *
 * @param instance_index Index of ticker instance
 */
void ticker_trigger(u8_t instance_index)
{
	struct ticker_instance *instance;

	DEBUG_TICKER_ISR(1);

	instance = &_instance[instance_index];
	if (instance->sched_cb) {
		instance->sched_cb(TICKER_CALL_ID_TRIGGER,
				   TICKER_CALL_ID_WORKER, 1, instance);
	}

	DEBUG_TICKER_ISR(0);
}

/**
 * @brief Start a ticker node
 *
 * @details Creates a new user operation of type TICKER_USER_OP_TYPE_START and
 * schedules the ticker_job.
 *
 * @param instance_index     Index of ticker instance
 * @param user_id	     Ticker user id. Used for indexing user operations
 *			     and mapping to mayfly caller id
 * @param ticker_id	     Id of ticker node
 * @param ticks_anchor	     Absolute tick count as anchor point for
 *			     ticks_first
 * @param ticks_first	     Initial number of ticks before first timeout
 * @param ticks_periodic     Number of ticks for a peridic ticker node. If 0,
 *			     ticker node is treated as one-shot
 * @param remainder_periodic Periodic ticks fraction
 * @param lazy		     Number of periods to skip (latency). A value of 1
 *			     causes skipping every other timeout
 * @param ticks_slot	     Slot reservation ticks for node (air-time)
 * @param fp_timeout_func    Function pointer of function to call at timeout
 * @param context	     Context passed in timeout call
 * @param fp_op_func	     Function pointer of user operation completion
 *			     function
 * @param op_context	     Context passed in operation completion call
 *
 * @return TICKER_STATUS_BUSY if start was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to
 * run before exiting ticker_start
 */
u32_t ticker_start(u8_t instance_index, u8_t user_id, u8_t ticker_id,
		   u32_t ticks_anchor, u32_t ticks_first, u32_t ticks_periodic,
		   u32_t remainder_periodic, u16_t lazy, u32_t ticks_slot,
		   ticker_timeout_func fp_timeout_func, void *context,
		   ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	u8_t last;

	user = &instance->users[user_id];

	last = user->last + 1;
	if (last >= user->count_user_op) {
		last = 0U;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_START;
	user_op->id = ticker_id;
	user_op->params.start.ticks_at_start = ticks_anchor;
	user_op->params.start.ticks_first = ticks_first;
	user_op->params.start.ticks_periodic = ticks_periodic;
	user_op->params.start.remainder_periodic = remainder_periodic;
	user_op->params.start.ticks_slot = ticks_slot;
	user_op->params.start.lazy = lazy;
	user_op->params.start.fp_timeout_func = fp_timeout_func;
	user_op->params.start.context = context;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}

/**
 * @brief Update a ticker node
 *
 * @details Creates a new user operation of type TICKER_USER_OP_TYPE_UPDATE and
 * schedules the ticker_job.
 *
 * @param instance_index     Index of ticker instance
 * @param user_id	     Ticker user id. Used for indexing user operations
 *			     and mapping to mayfly caller id
 * @param ticker_id	     Id of ticker node
 * @param ticks_drift_plus   Number of ticks to add for drift compensation
 * @param ticks_drift_minus  Number of ticks to subtract for drift compensation
 * @param ticks_slot_plus    Number of ticks to add to slot reservation
 * @param ticks_slot_minus   Number of ticks to add subtract from slot
 *			     reservation
 * @param lazy		     Number of periods to skip (latency). A value of 0
 *			     means no action. 1 means no latency (normal). A
 *			     value >1 means latency = lazy - 1
 * @param force		     Force update to take effect immediately. With
 *			     force = 0, update is scheduled to take effect as
 *			     soon as possible
 * @param fp_op_func	     Function pointer of user operation completion
 *			     function
 * @param op_context	     Context passed in operation completion call
 *
 * @return TICKER_STATUS_BUSY if update was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to run
 * before exiting ticker_update
 */
u32_t ticker_update(u8_t instance_index, u8_t user_id, u8_t ticker_id,
		    u32_t ticks_drift_plus, u32_t ticks_drift_minus,
		    u32_t ticks_slot_plus, u32_t ticks_slot_minus, u16_t lazy,
		    u8_t force, ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	u8_t last;

	user = &instance->users[user_id];

	last = user->last + 1;
	if (last >= user->count_user_op) {
		last = 0U;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_UPDATE;
	user_op->id = ticker_id;
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

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}

/**
 * @brief Stop a ticker node
 *
 * @details Creates a new user operation of type TICKER_USER_OP_TYPE_STOP and
 * schedules the ticker_job.
 *
 * @param instance_index     Index of ticker instance
 * @param user_id	     Ticker user id. Used for indexing user operations
 *			     and mapping to mayfly caller id
 * @param fp_op_func	     Function pointer of user operation completion
 *			     function
 * @param op_context	     Context passed in operation completion call
 *
 * @return TICKER_STATUS_BUSY if stop was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to run
 * before exiting ticker_stop
 */
u32_t ticker_stop(u8_t instance_index, u8_t user_id, u8_t ticker_id,
		  ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	u8_t last;

	user = &instance->users[user_id];

	last = user->last + 1;
	if (last >= user->count_user_op) {
		last = 0U;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_STOP;
	user_op->id = ticker_id;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}

/**
 * @brief Get next ticker node slot
 *
 * @details Gets the next ticker which has slot ticks specified and
 * return the ticker id and accumulated ticks until expiration. If no
 * ticker nodes have slot ticks, the next ticker node is returned.
 * If no head id is provided (TICKER_NULL) the first node is returned.
 *
 * @param instance_index     Index of ticker instance
 * @param user_id	     Ticker user id. Used for indexing user operations
 *			     and mapping to mayfly caller id
 * @param ticker_id	     Pointer to id of ticker node
 * @param ticks_current	     Pointer to current ticks count
 * @param ticks_to_expire    Pointer to ticks to expire
 * @param fp_op_func	     Function pointer of user operation completion
 *			     function
 * @param op_context	     Context passed in operation completion call
 *
 * @return TICKER_STATUS_BUSY if request was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to run
 * before exiting ticker_next_slot_get
 */
u32_t ticker_next_slot_get(u8_t instance_index, u8_t user_id, u8_t *ticker_id,
			   u32_t *ticks_current, u32_t *ticks_to_expire,
			   ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	u8_t last;

	user = &instance->users[user_id];

	last = user->last + 1;
	if (last >= user->count_user_op) {
		last = 0U;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_SLOT_GET;
	user_op->id = TICKER_NULL;
	user_op->params.slot_get.ticker_id = ticker_id;
	user_op->params.slot_get.ticks_current = ticks_current;
	user_op->params.slot_get.ticks_to_expire = ticks_to_expire;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}

/**
 * @brief Get a callback at the end of ticker job execution
 *
 * @details Operation completion callback is called at the end of the
 * ticker_job execution. The user operation is immutable.
 *
 * @param instance_index     Index of ticker instance
 * @param user_id	     Ticker user id. Used for indexing user operations
 *			     and mapping to mayfly caller id
 * @param fp_op_func	     Function pointer of user operation completion
 *			     function
 * @param op_context	     Context passed in operation completion call
 *
 * @return TICKER_STATUS_BUSY if request was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to run
 * before exiting ticker_job_idle_get
 */
u32_t ticker_job_idle_get(u8_t instance_index, u8_t user_id,
			  ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	u8_t last;

	user = &instance->users[user_id];

	last = user->last + 1;
	if (last >= user->count_user_op) {
		last = 0U;
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

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}

#if !defined(CONFIG_BT_TICKER_COMPATIBILITY_MODE)
/**
 * @brief Set ticker node priority
 *
 * @param instance_index     Index of ticker instance
 * @param user_id	     Ticker user id. Used for indexing user operations
 *			     and mapping to mayfly caller id
 * @param ticker_id	     Id of ticker node to set priority on
 * @param priority	     Priority to set. Range [-128..127], default is 0.
 *			     Lover value equals higher priority. Setting
 *			     priority to -128 (TICKER_PRIORITY_CRITICAL) makes
 *			     the node win all collision challenges. Only one
 *			     node can have this priority assigned.
 * @param fp_op_func	     Function pointer of user operation completion
 *			     function
 * @param op_context	     Context passed in operation completion call
 *
 * @return TICKER_STATUS_BUSY if request was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to run
 * before exiting ticker_priority_set
 */
u32_t ticker_priority_set(u8_t instance_index, u8_t user_id, u8_t ticker_id,
			  s8_t priority, ticker_op_func fp_op_func,
			  void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	u8_t last;

	user = &instance->users[user_id];

	last = user->last + 1;
	if (last >= user->count_user_op) {
		last = 0U;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_PRIORITY_SET;
	user_op->id = ticker_id;
	user_op->params.priority_set.priority = priority;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}
#endif /* !CONFIG_BT_TICKER_COMPATIBILITY_MODE */

/**
 * @brief Schedule ticker job
 *
 * @param instance_index Index of ticker instance
 * @param user_id	 Ticker user id. Maps to mayfly caller id
 */
void ticker_job_sched(u8_t instance_index, u8_t user_id)
{
	struct ticker_instance *instance = &_instance[instance_index];

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);
}

/**
 * @brief Get current absolute tick count
 *
 * @return Absolute tick count
 */
u32_t ticker_ticks_now_get(void)
{
	return cntr_cnt_get();
}

/**
 * @brief Get diffence between two tick counts
 *
 * @details Subtract two counts and truncate to correct HW dependent counter
 * bit width
 *
 * @param ticks_now Highest tick count (now)
 * @param ticks_old Tick count to subtract from ticks_now
 */
u32_t ticker_ticks_diff_get(u32_t ticks_now, u32_t ticks_old)
{
	return ((ticks_now - ticks_old) & HAL_TICKER_CNTR_MASK);
}
