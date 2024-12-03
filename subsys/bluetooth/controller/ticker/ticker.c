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
#include "hal/cpu.h"

#include "ticker.h"

#include "hal/debug.h"

/*****************************************************************************
 * Defines
 ****************************************************************************/
#define DOUBLE_BUFFER_SIZE 2

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
#if !defined(CONFIG_BT_CTLR_ADV_AUX_SET)
#define BT_CTLR_ADV_AUX_SET 0
#else
#define BT_CTLR_ADV_AUX_SET CONFIG_BT_CTLR_ADV_AUX_SET
#endif
#if !defined(CONFIG_BT_CTLR_ADV_SYNC_SET)
#define BT_CTLR_ADV_SYNC_SET 0
#else
#define BT_CTLR_ADV_SYNC_SET CONFIG_BT_CTLR_ADV_SYNC_SET
#endif
#if defined(CONFIG_BT_CTLR_ADV_ISO)
#define TICKER_EXPIRE_INFO_MAX (BT_CTLR_ADV_AUX_SET + BT_CTLR_ADV_SYNC_SET*2)
#else
#define TICKER_EXPIRE_INFO_MAX (BT_CTLR_ADV_AUX_SET + BT_CTLR_ADV_SYNC_SET)
#endif /* !CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

/*****************************************************************************
 * Types
 ****************************************************************************/

struct ticker_node {
	uint8_t  next;			    /* Next ticker node */

	uint8_t  req;			    /* Request counter */
	uint8_t  ack;			    /* Acknowledge counter. Imbalance
					     * between req and ack indicates
					     * ongoing operation
					     */
	uint8_t  force:1;		    /* If non-zero, node timeout should
					     * be forced at next expiration
					     */
#if defined(CONFIG_BT_TICKER_PREFER_START_BEFORE_STOP)
	uint8_t  start_pending:1;	    /* If non-zero, start is pending for
					     * bottom half of ticker_job.
					     */
#endif /* CONFIG_BT_TICKER_PREFER_START_BEFORE_STOP */
	uint32_t ticks_periodic;	    /* If non-zero, interval
					     * between expirations
					     */
	uint32_t ticks_to_expire;	    /* Ticks until expiration */
	ticker_timeout_func timeout_func;   /* User timeout function */
	void  *context;			    /* Context delivered to timeout
					     * function
					     */
	uint32_t ticks_to_expire_minus;	    /* Negative drift correction */
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	uint32_t ticks_slot;		    /* Air-time reservation for node */
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */
	uint16_t lazy_periodic;		    /* Number of timeouts to allow
					     * skipping
					     */
	uint16_t lazy_current;		    /* Current number of timeouts
					     * skipped = peripheral latency
					     */
	union {
#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
		uint32_t remainder_periodic;/* Sub-microsecond tick remainder
					     * for each period
					     */
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

		ticker_op_func fp_op_func;  /* Operation completion callback */
	};

	union {
#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
		uint32_t remainder_current; /* Current sub-microsecond tick
					     * remainder
					     */
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

		void  *op_context;	    /* Context passed in completion
					     * callback
					     */
	};

#if  defined(CONFIG_BT_TICKER_EXT)
	struct ticker_ext *ext_data;	    /* Ticker extension data */
#endif /* CONFIG_BT_TICKER_EXT */
#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	uint8_t  must_expire;		    /* Node must expire, even if it
					     * collides with other nodes
					     */
#if defined(CONFIG_BT_TICKER_PRIORITY_SET)
	int8_t  priority;		    /* Ticker node priority. 0 is
					     * default. Lower value is higher
					     * priority
					     */
#endif /* CONFIG_BT_TICKER_PRIORITY_SET */
#endif /* !CONFIG_BT_TICKER_LOW_LAT &&
	* !CONFIG_BT_TICKER_SLOT_AGNOSTIC
	*/
};

struct ticker_expire_info_internal {
	uint32_t ticks_to_expire;
	uint32_t remainder;
	uint16_t lazy;
	uint8_t ticker_id;
	uint8_t outdated:1;
	uint8_t found:1;
	uint8_t last:1;
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
#define TICKER_USER_OP_TYPE_YIELD_ABS    6
#define TICKER_USER_OP_TYPE_STOP         7
#define TICKER_USER_OP_TYPE_STOP_ABS     8

/* Slot window re-schedule states */
#define TICKER_RESCHEDULE_STATE_NONE     0
#define TICKER_RESCHEDULE_STATE_PENDING  1
#define TICKER_RESCHEDULE_STATE_DONE     2

#if defined(CONFIG_BT_TICKER_EXT) && !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
#define TICKER_HAS_SLOT_WINDOW(_ticker) \
	((_ticker)->ext_data && ((_ticker)->ext_data->ticks_slot_window != 0U))
#define TICKER_RESCHEDULE_PENDING(_ticker) \
	(_ticker->ext_data && (_ticker->ext_data->reschedule_state == \
		TICKER_RESCHEDULE_STATE_PENDING))
#else
#define TICKER_HAS_SLOT_WINDOW(_ticker) 0
#define TICKER_RESCHEDULE_PENDING(_ticker) 0
#endif

/* User operation data structure for start opcode. Used for passing start
 * requests to ticker_job
 */
struct ticker_user_op_start {
	uint32_t ticks_at_start;	/* Anchor ticks (absolute) */
	uint32_t ticks_first;		/* Initial timeout ticks */
	uint32_t ticks_periodic;	/* Ticker period ticks */

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
	uint32_t remainder_periodic;	/* Sub-microsecond tick remainder */

#if defined(CONFIG_BT_TICKER_START_REMAINDER)
	uint32_t remainder_first;       /* Sub-microsecond tick remainder */
#endif /* CONFIG_BT_TICKER_START_REMAINDER */
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

	uint16_t lazy;			/* Periodic latency in number of
					 * periods
					 */

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	uint32_t ticks_slot;		/* Air-time reservation ticks */
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */

	ticker_timeout_func fp_timeout_func; /* Timeout callback function */
	void  *context;			/* Context passed in timeout callback */

#if defined(CONFIG_BT_TICKER_EXT)
	struct ticker_ext *ext_data;	/* Ticker extension data instance */
#endif /* CONFIG_BT_TICKER_EXT */
};

/* User operation data structure for update opcode. Used for passing update
 * requests to ticker_job
 */
struct ticker_user_op_update {
	uint32_t ticks_drift_plus;	/* Requested positive drift in ticks */
	uint32_t ticks_drift_minus;	/* Requested negative drift in ticks */
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	uint32_t ticks_slot_plus;	/* Number of ticks to add to slot
					 * reservation (air-time)
					 */
	uint32_t ticks_slot_minus;	/* Number of ticks to subtract from
					 * slot reservation (air-time)
					 */
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */
	uint16_t lazy;			/* Peripheral latency:
					 *  0: Do nothing
					 *  1: latency = 0
					 * >1: latency = lazy - 1
					 */
	uint8_t  force;			/* Force update */
#if defined(CONFIG_BT_TICKER_EXT) && !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC) &&\
	!defined(CONFIG_BT_TICKER_LOW_LAT)
	uint8_t must_expire;		/* Node must expire, even if it
					 * collides with other nodes:
					 *  0x00: Do nothing
					 *  0x01: Disable must_expire
					 *  0x02: Enable must_expire
					 */
#endif
#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	uint8_t expire_info_id;
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
};

/* User operation data structure for yield/stop opcode. Used for passing yield/
 * stop requests with absolute tick to ticker_job
 */
struct ticker_user_op_yield {
	uint32_t ticks_at_yield;        /* Anchor ticks (absolute) */
};

/* User operation data structure for slot_get opcode. Used for passing request
 * to get next ticker with slot ticks via ticker_job
 */
struct ticker_user_op_slot_get {
	uint8_t  *ticker_id;
	uint32_t *ticks_current;
	uint32_t *ticks_to_expire;
#if defined(CONFIG_BT_TICKER_REMAINDER_GET)
	uint32_t *remainder;
#endif /* CONFIG_BT_TICKER_REMAINDER_GET */
#if defined(CONFIG_BT_TICKER_LAZY_GET)
	uint16_t *lazy;
#endif /* CONFIG_BT_TICKER_LAZY_GET */
#if defined(CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH)
	ticker_op_match_func fp_match_op_func;
	void *match_op_context;
#endif /* CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH */
};

/* User operation data structure for priority_set opcode. Used for passing
 * request to set ticker node priority via ticker_job
 */
struct ticker_user_op_priority_set {
	int8_t priority;		/* Node priority. Defaults to 0 */
};

/* User operation top level data structure. Used for passing requests to
 * ticker_job
 */
struct ticker_user_op {
	uint8_t op;			/* User operation */
	uint8_t id;			/* Ticker node id */
	uint8_t status;			/* Operation result */
	union {
		struct ticker_user_op_start        start;
		struct ticker_user_op_update       update;
		struct ticker_user_op_yield        yield;
		struct ticker_user_op_slot_get     slot_get;
		struct ticker_user_op_priority_set priority_set;
	} params;			/* User operation parameters */
	ticker_op_func fp_op_func;	/* Operation completion callback */
	void  *op_context;		/* Context passed in completion callback */
};

/* User data structure for operations
 */
struct ticker_user {
	uint8_t count_user_op;		/* Number of user operation slots */
	uint8_t first;			/* Slot index of first user operation */
	uint8_t middle;			/* Slot index of last managed user op.
					 * Updated by ticker_job_list_manage
					 * for use in ticker_job_list_insert
					 */
	uint8_t last;			/* Slot index of last user operation */
	struct ticker_user_op *user_op; /* Pointer to user operation array */
};

/* Ticker instance
 */
struct ticker_instance {
	struct ticker_node *nodes;	/* Pointer to ticker nodes */
	struct ticker_user *users;	/* Pointer to user nodes */
	uint8_t  count_node;		/* Number of ticker nodes */
	uint8_t  count_user;		/* Number of user nodes */
	uint8_t  ticks_elapsed_first;	/* Index from which elapsed ticks count
					 * is pulled
					 */
	uint8_t  ticks_elapsed_last;	/* Index to which elapsed ticks count
					 * is pushed
					 */
	uint32_t ticks_elapsed[DOUBLE_BUFFER_SIZE]; /* Buffer for elapsed
						     * ticks
						     */
	uint32_t ticks_current;		/* Absolute ticks elapsed at last
					 * ticker_job
					 */
	uint8_t  ticker_id_head;	/* Index of first ticker node (next to
					 * expire)
					 */
	uint8_t  job_guard;		/* Flag preventing ticker_worker from
					 * running if ticker_job is active
					 */
	uint8_t  worker_trigger;	/* Flag preventing ticker_job from
					 * starting if ticker_worker was
					 * requested, and to trigger
					 * ticker_worker at end of job, if
					 * requested
					 */

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	uint8_t  ticker_id_slot_previous; /* Id of previous slot reserving
					   * ticker node
					   */
	uint32_t ticks_slot_previous;	/* Number of ticks previously reserved
					 * by a ticker node (active air-time)
					 */
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	struct ticker_expire_info_internal expire_infos[TICKER_EXPIRE_INFO_MAX];
	bool expire_infos_outdated;
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

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

static inline uint8_t ticker_add_to_remainder(uint32_t *remainder, uint32_t to_add);

/**
 * @brief Update elapsed index
 *
 * @param ticks_elapsed_index Pointer to current index
 *
 * @internal
 */
static inline void ticker_next_elapsed(uint8_t *ticks_elapsed_index)
{
	uint8_t idx = *ticks_elapsed_index + 1;

	if (idx == DOUBLE_BUFFER_SIZE) {
		idx = 0U;
	}
	*ticks_elapsed_index = idx;
}

#if defined(CONFIG_BT_TICKER_LOW_LAT)
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
static uint8_t ticker_by_slot_get(struct ticker_node *node, uint8_t ticker_id_head,
			       uint32_t ticks_slot)
{
	while (ticker_id_head != TICKER_NULL) {
		struct ticker_node *ticker;
		uint32_t ticks_to_expire;

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
#endif /* CONFIG_BT_TICKER_LOW_LAT */

#if defined(CONFIG_BT_TICKER_NEXT_SLOT_GET)
/**
 * @brief Get next ticker with slot ticks or match
 *
 * @details Iterates ticker nodes from ticker_id_head. If no head id is provided
 * (TICKER_NULL), iteration starts from the first node.
 * Operation details:
 *
 * NORMAL MODE (!CONFIG_BT_TICKER_SLOT_AGNOSTIC)
 * - Gets the next ticker which has slot ticks specified and return the ticker
 *   id and accumulated ticks until expiration.
 * - If a matching function is provided, this function is called and node iteration
 *   continues until match function returns true.
 *
 * SLOT AGNOSTIC MODE (CONFIG_BT_TICKER_SLOT_AGNOSTIC)
 * - Gets the next ticker node.
 * - If a matching function is provided, this function is called and node iteration
 *   continues until match function returns true.
 *
 * @param instance          Pointer to ticker instance
 * @param ticker_id_head    Pointer to id of first ticker node [in/out]
 * @param ticks_current     Pointer to current ticks count [in/out]
 * @param ticks_to_expire   Pointer to ticks to expire [in/out]
 * @param fp_match_op_func  Pointer to match function or NULL if unused
 * @param match_op_context  Pointer to operation context passed to match
 *                          function or NULL if unused
 * @param lazy              Pointer to lazy variable to receive lazy_current
 *                          of found ticker node
 * @internal
 */
static void ticker_by_next_slot_get(struct ticker_instance *instance,
				    uint8_t *ticker_id_head,
				    uint32_t *ticks_current,
				    uint32_t *ticks_to_expire,
				    ticker_op_match_func fp_match_op_func,
				    void *match_op_context, uint32_t *remainder,
				    uint16_t *lazy)
{
	struct ticker_node *ticker;
	struct ticker_node *node;
	uint32_t _ticks_to_expire;
	uint8_t _ticker_id_head;

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

	/* Find first ticker node with match or slot ticks */
	while (_ticker_id_head != TICKER_NULL) {
		ticker = &node[_ticker_id_head];

#if defined(CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH)
		if (fp_match_op_func) {
			uint32_t ticks_slot = 0;

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
			ticks_slot += ticker->ticks_slot;
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

			/* Match node id */
			if (fp_match_op_func(_ticker_id_head, ticks_slot,
					     _ticks_to_expire +
					     ticker->ticks_to_expire,
					     match_op_context)) {
				/* Match found */
				break;
			}
		} else
#else /* !CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH */
	ARG_UNUSED(fp_match_op_func);
	ARG_UNUSED(match_op_context);
#endif /* !CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH */

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
			if (ticker->ticks_slot) {
				/* Matching not used and node has slot ticks */
				break;
#else
			{
				/* Matching not used and slot agnostic */
				break;
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */
			}

		/* Accumulate expire ticks */
		_ticks_to_expire += ticker->ticks_to_expire;
		_ticker_id_head = ticker->next;
	}

	if (_ticker_id_head != TICKER_NULL) {
		/* Add ticks for found ticker */
		_ticks_to_expire += ticker->ticks_to_expire;

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
#if defined(CONFIG_BT_TICKER_REMAINDER_GET)
		if (remainder) {
			*remainder = ticker->remainder_current;
		}
#else /* !CONFIG_BT_TICKER_REMAINDER_GET */
		ARG_UNUSED(remainder);
#endif /* !CONFIG_BT_TICKER_REMAINDER_GET */
#else /* !CONFIG_BT_TICKER_REMAINDER_SUPPORT */
		ARG_UNUSED(remainder);
#endif /* !CONFIG_BT_TICKER_REMAINDER_SUPPORT */

#if defined(CONFIG_BT_TICKER_LAZY_GET)
		if (lazy) {
			*lazy = ticker->lazy_current;
		}
#else /* !CONFIG_BT_TICKER_LAZY_GET */
	ARG_UNUSED(lazy);
#endif /* !CONFIG_BT_TICKER_LAZY_GET */
	}

	*ticker_id_head = _ticker_id_head;
	*ticks_to_expire = _ticks_to_expire;
}
#endif /* CONFIG_BT_TICKER_NEXT_SLOT_GET */

#if !defined(CONFIG_BT_TICKER_LOW_LAT)
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
static uint8_t ticker_enqueue(struct ticker_instance *instance, uint8_t id)
{
	struct ticker_node *ticker_current;
	struct ticker_node *ticker_new;
	uint32_t ticks_to_expire_current;
	struct ticker_node *node;
	uint32_t ticks_to_expire;
	uint8_t previous;
	uint8_t current;

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
			ticks_to_expire = ticks_to_expire_current;
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
#else /* CONFIG_BT_TICKER_LOW_LAT */

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
static uint8_t ticker_enqueue(struct ticker_instance *instance, uint8_t id)
{
	struct ticker_node *ticker_current;
	struct ticker_node *ticker_new;
	uint32_t ticks_to_expire_current;
	uint8_t ticker_id_slot_previous;
	uint32_t ticks_slot_previous;
	struct ticker_node *node;
	uint32_t ticks_to_expire;
	uint8_t previous;
	uint8_t current;
	uint8_t collide;

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
#endif /* CONFIG_BT_TICKER_LOW_LAT */

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
static uint32_t ticker_dequeue(struct ticker_instance *instance, uint8_t id)
{
	struct ticker_node *ticker_current;
	struct ticker_node *node;
	uint8_t previous;
	uint32_t timeout;
	uint8_t current;
	uint32_t total;

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

#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
/**
 * @brief Resolve ticker node collision
 *
 * @details Evaluates the provided ticker node against other queued nodes
 * and returns non-zero if the ticker node collides and should be skipped.
 * The following rules are checked:
 *   1) If the periodic latency is not yet exhausted, node is skipped
 *   2) If the node has highest possible priority, node is never skipped
 *   3) If the node will starve next node due to slot reservation
 *      overlap, node is skipped if:
 *      a) Next node has higher priority than current node
 *      b) Next node has more accumulated latency than the current node
 *      c) Next node is 'older' than current node and has same priority
 *      d) Next node has force flag set, and the current does not
 *   4) If using ticks slot window,
 *      a) current node can be rescheduled later in the ticks slot window
 *   5) If using ticks slot window under yield (build time configuration),
 *      a) Current node can be rescheduled later in the ticks slot window when
 *         next node can not be rescheduled later in its ticks slot window
 *
 * @param nodes         Pointer to ticker node array
 * @param ticker        Pointer to ticker to resolve
 *
 * @return 0 if no collision was detected. 1 if ticker node collides
 * with other ticker node of higher composite priority
 * @internal
 */
static uint8_t ticker_resolve_collision(struct ticker_node *nodes,
				     struct ticker_node *ticker)
{
#if defined(CONFIG_BT_TICKER_PRIORITY_SET)
	if ((ticker->priority != TICKER_PRIORITY_CRITICAL) &&
	    (ticker->next != TICKER_NULL)) {

#else /* !CONFIG_BT_TICKER_PRIORITY_SET */
	if (ticker->next != TICKER_NULL) {

#endif /* !CONFIG_BT_TICKER_PRIORITY_SET */

		uint16_t lazy_current = ticker->lazy_current;
		uint32_t ticker_ticks_slot;

		if (TICKER_HAS_SLOT_WINDOW(ticker) && !ticker->ticks_slot) {
			ticker_ticks_slot = HAL_TICKER_RESCHEDULE_MARGIN;
		} else {
			ticker_ticks_slot = ticker->ticks_slot;
		}

		/* Check if this ticker node will starve next node which has
		 * latency or higher priority
		 */
		if (lazy_current >= ticker->lazy_periodic) {
			lazy_current -= ticker->lazy_periodic;
		}
		uint8_t  id_head = ticker->next;
		uint32_t acc_ticks_to_expire = 0U;

		/* Age is time since last expiry */
		uint32_t current_age = ticker->ticks_periodic +
				    (lazy_current * ticker->ticks_periodic);

		while (id_head != TICKER_NULL) {
			struct ticker_node *ticker_next = &nodes[id_head];
			uint32_t ticker_next_ticks_slot;

			/* Accumulate ticks_to_expire for each node */
			acc_ticks_to_expire += ticker_next->ticks_to_expire;
			if (acc_ticks_to_expire > ticker_ticks_slot) {
				break;
			}

			if (TICKER_HAS_SLOT_WINDOW(ticker_next) &&
			    (ticker_next->ticks_slot == 0U)) {
				ticker_next_ticks_slot =
					HAL_TICKER_RESCHEDULE_MARGIN;
			} else {
				ticker_next_ticks_slot =
					ticker_next->ticks_slot;
			}

			/* We only care about nodes with slot reservation */
			if (ticker_next_ticks_slot == 0U) {
				id_head = ticker_next->next;
				continue;
			}

			uint16_t lazy_next = ticker_next->lazy_current;
			uint8_t  lazy_next_periodic_skip =
				ticker_next->lazy_periodic > lazy_next;

			if (!lazy_next_periodic_skip) {
				lazy_next -= ticker_next->lazy_periodic;
			}

			/* Age is time since last expiry */
			uint32_t next_age = (ticker_next->ticks_periodic == 0U ?
					  0U :
					 (ticker_next->ticks_periodic -
					  ticker_next->ticks_to_expire)) +
					 (lazy_next *
					  ticker_next->ticks_periodic);

			/* Was the current node scheduled earlier? */
			uint8_t current_is_older =
				(ticker->ticks_periodic == 0U) ||
				(current_age > next_age);
			/* Was next node scheduled earlier (legacy priority)? */
			uint8_t next_is_older =
				(ticker->ticks_periodic != 0U) &&
				(next_age > current_age);

			/* Is the current and next node equal in force? */
			uint8_t equal_force =
				(ticker->force == ticker_next->force);
			/* Is force requested for next node (e.g. update) -
			 * more so than for current node?
			 */
			uint8_t next_force =
				(ticker_next->force > ticker->force);

#if defined(CONFIG_BT_TICKER_PRIORITY_SET)
			/* Does next node have critical priority and should
			 * always be scheduled?
			 */
			uint8_t next_is_critical =
				(ticker_next->priority ==
				 TICKER_PRIORITY_CRITICAL);

			/* Is the current and next node equal in priority? */
			uint8_t equal_priority =
				(ticker->priority == ticker_next->priority);

#else /* !CONFIG_BT_TICKER_PRIORITY_SET */
			uint8_t next_is_critical = 0U;
			uint8_t equal_priority = 1U;
			uint8_t next_has_priority = 0U;

#endif /* !CONFIG_BT_TICKER_PRIORITY_SET */

#if defined(CONFIG_BT_TICKER_EXT)
#if defined(CONFIG_BT_TICKER_EXT_SLOT_WINDOW_YIELD)
#if defined(CONFIG_BT_TICKER_PRIORITY_SET)
			/* Does next node have higher priority? */
			uint8_t next_has_priority =
				(!TICKER_HAS_SLOT_WINDOW(ticker_next) &&
				((lazy_next - ticker_next->priority) >
				 (lazy_current - ticker->priority));
#endif /* CONFIG_BT_TICKER_PRIORITY_SET */

			/* Colliding next ticker does not use ticks_slot_window
			 * or it does not fit after the current ticker within
			 * the ticks_slot_window.
			 */
			uint8_t next_not_ticks_slot_window =
					!TICKER_HAS_SLOT_WINDOW(ticker_next) ||
					(ticker_next->ext_data->is_drift_in_window &&
					 TICKER_HAS_SLOT_WINDOW(ticker)) ||
					((acc_ticks_to_expire +
					  ticker_next->ext_data->ticks_slot_window -
					  ticker_next->ticks_slot) <
					 ticker->ticks_slot);

			/* Can the current ticker with ticks_slot_window be
			 * scheduled after the colliding ticker?
			 */
			uint8_t curr_has_ticks_slot_window =
					TICKER_HAS_SLOT_WINDOW(ticker) &&
					((acc_ticks_to_expire +
					  ticker_next->ticks_slot) <=
					 (ticker->ext_data->ticks_slot_window -
					  ticker->ticks_slot));

#else /* !CONFIG_BT_TICKER_EXT_SLOT_WINDOW_YIELD */
#if defined(CONFIG_BT_TICKER_PRIORITY_SET)
			/* Does next node have higher priority? */
			uint8_t next_has_priority =
				(lazy_next - ticker_next->priority) >
				(lazy_current - ticker->priority);

#endif /* CONFIG_BT_TICKER_PRIORITY_SET */
			uint8_t next_not_ticks_slot_window = 1U;

			/* Can the current ticker with ticks_slot_window be
			 * scheduled after the colliding ticker?
			 * NOTE: Tickers with ticks_slot_window and no
			 *       ticks_slot (unreserved) be always scheduled
			 *       after the colliding ticker.
			 */
			uint8_t curr_has_ticks_slot_window =
				(TICKER_HAS_SLOT_WINDOW(ticker) &&
				 !ticker->ticks_slot &&
				 ((acc_ticks_to_expire +
				   ticker_next->ticks_slot) <=
				  (ticker->ext_data->ticks_slot_window)));

#endif /* !CONFIG_BT_TICKER_EXT_SLOT_WINDOW_YIELD */
#else /* !CONFIG_BT_TICKER_EXT */
#if defined(CONFIG_BT_TICKER_PRIORITY_SET)
			/* Does next node have higher priority? */
			uint8_t next_has_priority =
				(lazy_next - ticker_next->priority) >
				(lazy_current - ticker->priority);

#endif /* CONFIG_BT_TICKER_PRIORITY_SET */
			uint8_t next_not_ticks_slot_window = 1U;
			uint8_t curr_has_ticks_slot_window = 0U;

#endif /* !CONFIG_BT_TICKER_EXT */

			/* Check if next node is within this reservation slot
			 * and wins conflict resolution
			 */
			if ((curr_has_ticks_slot_window &&
			     next_not_ticks_slot_window) ||
			    (!lazy_next_periodic_skip &&
			     (next_is_critical ||
			      next_force ||
			      (next_has_priority && !current_is_older) ||
			      (equal_priority && equal_force && next_is_older &&
			       next_not_ticks_slot_window)))) {
				/* This node must be skipped - check window */
				return 1U;
			}
			id_head = ticker_next->next;
		}
	}

	return 0U;
}
#endif /* !CONFIG_BT_TICKER_LOW_LAT &&
	* !CONFIG_BT_TICKER_SLOT_AGNOSTIC
	*/

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
/**
 * @brief Get expiration delta from one ticker id to another ticker id
 *
 * @details Helper function to get expiration info between two tickers
 *
 * @param instance            Ticker instance
 * @param to_ticker_id        Target ticker id
 * @param from_ticker_id      Ticker id to compare with
 * @param expire_info         Pointer to ticker_expire_info that will
 *                            get populated with the result
 *
 * @internal
 */
static void ticker_get_expire_info(struct ticker_instance *instance, uint8_t to_ticker_id,
					  uint8_t from_ticker_id,
					  struct ticker_expire_info_internal *expire_info)
{
	struct ticker_node *current_node;
	uint32_t acc_ticks_to_expire = 0;
	uint8_t current_ticker_id;
	uint32_t from_ticks = 0;
	bool from_found = false;
	uint32_t to_ticks = 0;
	bool to_found = false;

	current_ticker_id = instance->ticker_id_head;
	current_node = &instance->nodes[instance->ticker_id_head];
	while (current_ticker_id != TICKER_NULL && (!to_found || !from_found)) {
		/* Accumulate expire ticks */
		acc_ticks_to_expire += current_node->ticks_to_expire;

		if (current_ticker_id == from_ticker_id) {
			from_ticks = acc_ticks_to_expire;
			from_found = true;
		} else if (current_ticker_id == to_ticker_id) {
			to_ticks = acc_ticks_to_expire;
			to_found = true;
		}

		current_ticker_id = current_node->next;
		current_node = &instance->nodes[current_ticker_id];
	}

	if (to_found && from_found) {
		struct ticker_node *to_ticker = &instance->nodes[to_ticker_id];

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
		uint32_t to_remainder = to_ticker->remainder_current;
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

		if (from_ticks > to_ticks) {
			/* from ticker is scheduled after the to ticker - use period
			 * to give an result
			 */
			if (to_ticker->ticks_periodic == 0) {
				/* single shot ticker */
				expire_info->found = 0;
				return;
			}
			while (to_ticks < from_ticks) {
				to_ticks += to_ticker->ticks_periodic;

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
				to_ticks += ticker_add_to_remainder(&to_remainder,
								    to_ticker->remainder_periodic);
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */
			}
		}

		expire_info->ticks_to_expire = to_ticks - from_ticks;
		expire_info->remainder = to_remainder;
		expire_info->lazy = to_ticker->lazy_current;
		expire_info->found = 1;
	} else {
		expire_info->found = 0;
	}
}

/**
 * @brief Allocate an expire info for the given ticker ID
 *
 * @param instance            Ticker instance
 * @param ticker_id           Ticker ID to allocate for
 *
 * @return Returns TICKER_STATUS_SUCCESS if the allocation succeeded,
 *         TICKER_STATUS_FAILURE otherwise
 *
 * @internal
 */
static uint32_t ticker_alloc_expire_info(struct ticker_instance *instance, uint8_t ticker_id)
{
	uint32_t status = TICKER_STATUS_FAILURE;
	uint8_t is_last = 0;

	for (int i = 0; i < TICKER_EXPIRE_INFO_MAX; i++) {
		if (instance->expire_infos[i].ticker_id == TICKER_NULL) {
			struct ticker_node *ticker = &instance->nodes[ticker_id];

			instance->expire_infos[i].ticker_id = ticker_id;
			instance->expire_infos[i].outdated = true;
			instance->expire_infos[i].last = is_last;
			ticker->ext_data->other_expire_info = &instance->expire_infos[i];
			instance->expire_infos_outdated = true;
			status = TICKER_STATUS_SUCCESS;
			break;
		} else if (instance->expire_infos[i].last && i < TICKER_EXPIRE_INFO_MAX - 1) {
			instance->expire_infos[i].last = 0;
			is_last = 1;
		}
	}

	return status;
}

/**
 * @brief Free a previously allocated expire info for the given ticker ID
 *
 * @param instance            Ticker instance
 * @param ticker_id           Ticker ID to free up the allocation for
 *
 * @internal
 */
static void ticker_free_expire_info(struct ticker_instance *instance, uint8_t ticker_id)
{
	uint8_t is_last = 0;
	uint8_t index;

	for (index = 0; index < TICKER_EXPIRE_INFO_MAX; index++) {
		if (instance->expire_infos[index].ticker_id == ticker_id) {
			instance->expire_infos[index].ticker_id = TICKER_NULL;
			is_last = instance->expire_infos[index].last;
			instance->expire_infos[index].last = 0;
			break;
		}
	}

	if (is_last) {
		/* Find new last used element and mark it */
		for (; index >= 0; index--) {
			if (instance->expire_infos[index].ticker_id != TICKER_NULL || index == 0) {
				instance->expire_infos[index].last = 1;
				break;
			}
		}
	}
}

/**
 * @brief Mark all expire infos involving a ticker ID as outdated
 *
 * @details If a ticker moves this function should be called to mark all expiration
 *          infos (if any) that involve that ticker as outdated and in need of re-calculation.
 *          If any expiration infos involving the ticker_id is found, the ticker instances
 *          expire_infos_outdated flag is also set.
 *
 * @param instance            Ticker instance
 * @param ticker_id           ID of ticker that has moved
 *
 * @internal
 */
static void ticker_mark_expire_info_outdated(struct ticker_instance *instance, uint8_t ticker_id)
{
	for (int i = 0; i < TICKER_EXPIRE_INFO_MAX; i++) {
		if (instance->expire_infos[i].ticker_id != TICKER_NULL) {
			uint8_t current_id = instance->expire_infos[i].ticker_id;
			struct ticker_node *ticker = &instance->nodes[current_id];

			if (current_id == ticker_id ||
			    ticker->ext_data->expire_info_id == ticker_id) {
				instance->expire_infos[i].outdated = true;
				instance->expire_infos_outdated = true;
			}
		}
		if (instance->expire_infos[i].last) {
			break;
		}
	}
}

/**
 * @brief Run through all expire infos and update them if needed
 *
 * @details Runs through all expire_infos and runs ticker_get_expire_info()
 *          for any that are marked as outdated. Clears the expire_infos_outdated
 *          flag when done
 *
 * @param param Pointer to ticker instance
 *
 * @internal
 */
static void ticker_job_update_expire_infos(struct ticker_instance *instance)
{
	for (int i = 0; i < TICKER_EXPIRE_INFO_MAX; i++) {
		struct ticker_expire_info_internal *info = &instance->expire_infos[i];

		if (info->ticker_id != TICKER_NULL && info->outdated) {
			struct ticker_node *ticker = &instance->nodes[info->ticker_id];

			ticker_get_expire_info(instance, ticker->ext_data->expire_info_id,
						info->ticker_id, info);
			info->outdated = false;
		}

		if (info->last) {
			break;
		}
	}

	instance->expire_infos_outdated = false;
}

#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

/**
 * @brief Ticker worker
 *
 * @details Runs as upper half of ticker operation, triggered by a compare
 * match from the underlying counter HAL, via the ticker_trigger function.
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
	uint32_t ticks_elapsed;
	uint32_t ticks_expired;
	uint8_t ticker_id_head;
	uint32_t ticks_now;

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

	ticks_now = cntr_cnt_get();

	/* Get ticks elapsed since last job execution */
	ticks_elapsed = ticker_ticks_diff_get(ticks_now,
					      instance->ticks_current);

	/* Initialize actual elapsed ticks being consumed */
	ticks_expired = 0U;

	/* Auto variable containing the head of tickers expiring */
	ticker_id_head = instance->ticker_id_head;

#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	/* Check if the previous ticker node which had air-time, is still
	 * active and has this time slot reserved
	 */
	uint8_t slot_reserved = 0;

	if (instance->ticks_slot_previous > ticks_elapsed) {
		/* This node intersects reserved slot */
		slot_reserved = 1;
	}
#endif /* !CONFIG_BT_TICKER_LOW_LAT &&
	* !CONFIG_BT_TICKER_SLOT_AGNOSTIC
	*/

	/* Expire all tickers within ticks_elapsed and collect ticks_expired */
	node = &instance->nodes[0];

	while (ticker_id_head != TICKER_NULL) {
		struct ticker_node *ticker;
		uint32_t ticks_to_expire;
		uint8_t must_expire_skip;
		uint32_t ticks_drift;

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

		/* Skip if not scheduled to execute */
		if (((ticker->req - ticker->ack) & 0xff) != 1U) {
			continue;
		}

#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
		uint32_t ticker_ticks_slot;

		if (TICKER_HAS_SLOT_WINDOW(ticker) &&
		    (ticker->ticks_slot == 0U)) {
			ticker_ticks_slot = HAL_TICKER_RESCHEDULE_MARGIN;
		} else {
			ticker_ticks_slot = ticker->ticks_slot;
		}

		/* Check if node has slot reservation and resolve any collision
		 * with other ticker nodes
		 */
		if ((ticker_ticks_slot != 0U) &&
		    (slot_reserved ||
		     (instance->ticks_slot_previous > ticks_expired) ||
		     ticker_resolve_collision(node, ticker))) {
#if defined(CONFIG_BT_TICKER_EXT)
			struct ticker_ext *ext_data = ticker->ext_data;

			if (ext_data &&
			    ext_data->ticks_slot_window != 0U &&
			    ext_data->reschedule_state ==
			    TICKER_RESCHEDULE_STATE_NONE &&
			   (ticker->lazy_periodic <= ticker->lazy_current)) {
				/* Mark node for re-scheduling in ticker_job */
				ext_data->reschedule_state =
					TICKER_RESCHEDULE_STATE_PENDING;
			} else if (ext_data) {
				/* Mark node as not re-scheduling */
				ext_data->reschedule_state =
					TICKER_RESCHEDULE_STATE_NONE;
			}
#endif /* CONFIG_BT_TICKER_EXT */
			/* Increment lazy_current to indicate skipped event. In case
			 * of re-scheduled node, the lazy count will be decremented in
			 * ticker_job_reschedule_in_window when completed.
			 */
			ticker->lazy_current++;

			if ((ticker->must_expire == 0U) ||
			    (ticker->lazy_periodic >= ticker->lazy_current) ||
			    TICKER_RESCHEDULE_PENDING(ticker)) {
				/* Not a must-expire node or this is periodic
				 * latency or pending re-schedule. Skip this
				 * ticker node. Mark it as elapsed.
				 */
				ticker->ack--;
				continue;
			}

			/* Continue but perform shallow expiry */
			must_expire_skip = 1U;
		}

#if defined(CONFIG_BT_TICKER_EXT)
		if (ticker->ext_data) {
			ticks_drift = ticker->ext_data->ticks_drift;
			ticker->ext_data->ticks_drift = 0U;
			/* Mark node as not re-scheduling */
			ticker->ext_data->reschedule_state =
				TICKER_RESCHEDULE_STATE_NONE;
		} else {
			ticks_drift = 0U;
		}

#else  /* !CONFIG_BT_TICKER_EXT */
		ticks_drift = 0U;
#endif /* !CONFIG_BT_TICKER_EXT */

#else  /* CONFIG_BT_TICKER_LOW_LAT ||
	* CONFIG_BT_TICKER_SLOT_AGNOSTIC
	*/
		ticks_drift = 0U;
#endif /* CONFIG_BT_TICKER_LOW_LAT ||
	* CONFIG_BT_TICKER_SLOT_AGNOSTIC
	*/

		/* Scheduled timeout is acknowledged to be complete */
		ticker->ack--;

		if (ticker->timeout_func) {
			uint32_t remainder_current;
			uint32_t ticks_at_expire;

			ticks_at_expire = (instance->ticks_current +
					   ticks_expired -
					   ticker->ticks_to_expire_minus) &
					   HAL_TICKER_CNTR_MASK;

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
			remainder_current = ticker->remainder_current;
#else /* !CONFIG_BT_TICKER_REMAINDER_SUPPORT */
			remainder_current = 0U;
#endif /* !CONFIG_BT_TICKER_REMAINDER_SUPPORT */

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
			if (ticker->ext_data &&
			    ticker->ext_data->ext_timeout_func) {
				struct ticker_expire_info_internal *expire_info;
				struct ticker_ext_context ext_context;
				ticker_timeout_func timeout_func;

				timeout_func = ticker->ext_data->ext_timeout_func;
				expire_info = ticker->ext_data->other_expire_info;
				if (ticker->ext_data->expire_info_id != TICKER_NULL) {
					LL_ASSERT(expire_info && !expire_info->outdated);
				}

				ext_context.context = ticker->context;
				if (expire_info && expire_info->found) {
					ext_context.other_expire_info = (void *)expire_info;
				} else {
					ext_context.other_expire_info = NULL;
				}

				DEBUG_TICKER_TASK(1);

				/* Invoke the timeout callback */
				timeout_func(ticks_at_expire,
					     ticks_drift,
					     remainder_current,
					     must_expire_skip ?
					     TICKER_LAZY_MUST_EXPIRE :
					     ticker->lazy_current,
					     ticker->force,
					     &ext_context);
			} else
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
			{
				DEBUG_TICKER_TASK(1);

				/* Invoke the timeout callback */
				ticker->timeout_func(ticks_at_expire,
					     ticks_drift,
					     remainder_current,
					     must_expire_skip ?
					     TICKER_LAZY_MUST_EXPIRE :
					     ticker->lazy_current,
					     ticker->force,
					     ticker->context);
				DEBUG_TICKER_TASK(0);
			}

			if (!IS_ENABLED(CONFIG_BT_TICKER_LOW_LAT) &&
			   (must_expire_skip == 0U)) {
				/* Reset latency to periodic offset */
				ticker->lazy_current = 0U;
				ticker->force = 0U;

#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
				if (ticker_ticks_slot != 0U) {
					/* Any further nodes will be skipped */
					slot_reserved = 1U;
				}
#endif /* !CONFIG_BT_TICKER_LOW_LAT &&
	* !CONFIG_BT_TICKER_SLOT_AGNOSTIC
	*/

			}
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
				 uint32_t ticks_current, uint32_t ticks_at_start)
{
	uint32_t ticks_to_expire = ticker->ticks_to_expire;
	uint32_t ticks_to_expire_minus = ticker->ticks_to_expire_minus;

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
		uint32_t delta_current_start;

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
 * @brief Add to remainder
 *
 * @details Calculates whether the remainder should increments expiration time
 * for above-microsecond precision counter HW. The remainder enables improved
 * ticker precision, but is disabled for sub-microsecond precision
 * configurations.
 * Note: This is the same functionality as ticker_remainder_inc(), except this
 * function allows doing the calculation without modifying any tickers
 *
 * @param remainder Pointer to remainder to add to
 * @param to_add    Remainder value to add
 *
 * @return Returns 1 to indicate ticks increment is due, otherwise 0
 * @internal
 */
static inline uint8_t ticker_add_to_remainder(uint32_t *remainder, uint32_t to_add)
{
	*remainder += to_add;
	if ((*remainder < BIT(31)) &&
	    (*remainder > (HAL_TICKER_REMAINDER_RANGE >> 1))) {
		*remainder -= HAL_TICKER_REMAINDER_RANGE;

		return 1;
	}

	return 0;
}

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
/**
 * @brief Increment remainder
 *
 * @details Calculates whether the remainder should increments expiration time
 * for above-microsecond precision counter HW. The remainder enables improved
 * ticker precision, but is disabled for sub-microsecond precision
 * configurations.
 *
 * @param ticker Pointer to ticker node
 *
 * @return Returns 1 to indicate increment is due, otherwise 0
 * @internal
 */
static uint8_t ticker_remainder_inc(struct ticker_node *ticker)
{
	return ticker_add_to_remainder(&ticker->remainder_current, ticker->remainder_periodic);
}

/**
 * @brief Decrement remainder
 *
 * @details Calculates whether the remainder should decrements expiration time
 * for above-microsecond precision counter HW. The remainder enables improved
 * ticker precision, but is disabled for sub-microsecond precision
 * configurations.
 *
 * @param ticker Pointer to ticker node
 *
 * @return Returns 1 to indicate decrement is due, otherwise 0
 * @internal
 */
static uint8_t ticker_remainder_dec(struct ticker_node *ticker)
{
	uint8_t decrement = 0U;

	if ((ticker->remainder_current >= BIT(31)) ||
	    (ticker->remainder_current <= (HAL_TICKER_REMAINDER_RANGE >> 1))) {
		decrement++;
		ticker->remainder_current += HAL_TICKER_REMAINDER_RANGE;
	}

	ticker->remainder_current -= ticker->remainder_periodic;

	return decrement;
}
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

/**
 * @brief Invoke user operation callback
 *
 * @param user_op Pointer to user operation struct
 * @param status  User operation status to pass to callback
 *
 * @internal
 */
static void ticker_job_op_cb(struct ticker_user_op *user_op, uint8_t status)
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
static inline uint32_t ticker_job_node_update(struct ticker_instance *instance,
					  struct ticker_node *ticker,
					  struct ticker_user_op *user_op,
					  uint32_t ticks_now,
					  uint32_t ticks_current,
					  uint32_t ticks_elapsed,
					  uint8_t *insert_head)
{
	uint32_t ticks_to_expire = ticker->ticks_to_expire;

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
		while ((ticks_to_expire > ticker->ticks_periodic) &&
		       (ticker->lazy_current > user_op->params.update.lazy)) {
			ticks_to_expire -= ticker->ticks_periodic;

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
			ticks_to_expire -= ticker_remainder_dec(ticker);
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

			ticker->lazy_current--;
		}

		while (ticker->lazy_current < user_op->params.update.lazy) {
			ticks_to_expire += ticker->ticks_periodic;

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
			ticks_to_expire += ticker_remainder_inc(ticker);
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

			ticker->lazy_current++;
		}
		ticker->lazy_periodic = user_op->params.update.lazy;
	}

	/* Update ticks_to_expire from drift input */
	ticker->ticks_to_expire = ticks_to_expire +
				  user_op->params.update.ticks_drift_plus;
	ticker->ticks_to_expire_minus +=
				user_op->params.update.ticks_drift_minus;

#if defined(CONFIG_BT_TICKER_EXT) && !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	/* TODO: An improvement on this could be to only consider the drift
	 * (ADV => randomization) if re-sceduling fails. We would still store
	 * the drift ticks here, but not actually update the node. That would
	 * allow the ticker to use the full window for re-scheduling.
	 */
	struct ticker_ext *ext_data = ticker->ext_data;

	if (ext_data && ext_data->ticks_slot_window != 0U) {
		ext_data->ticks_drift =
			user_op->params.update.ticks_drift_plus -
			user_op->params.update.ticks_drift_minus;
	}
#endif /* CONFIG_BT_TICKER_EXT && !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

	ticks_to_expire_prep(ticker, ticks_current, ticks_now);

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	/* Update ticks_slot parameter from plus/minus input */
	ticker->ticks_slot += user_op->params.update.ticks_slot_plus;
	if (ticker->ticks_slot > user_op->params.update.ticks_slot_minus) {
		ticker->ticks_slot -= user_op->params.update.ticks_slot_minus;
	} else {
		ticker->ticks_slot = 0U;
	}
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */

	/* Update force parameter */
	if (user_op->params.update.force != 0U) {
		ticker->force = user_op->params.update.force;
	}

#if defined(CONFIG_BT_TICKER_EXT) && !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC) &&\
	!defined(CONFIG_BT_TICKER_LOW_LAT)
	/* Update must_expire parameter */
	if (user_op->params.update.must_expire) {
		/* 1: disable, 2: enable */
		ticker->must_expire = (user_op->params.update.must_expire - 1);
	}
#endif /* CONFIG_BT_TICKER_EXT */

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	if (ticker->ext_data && user_op->params.update.expire_info_id != user_op->id) {
		if (user_op->params.update.expire_info_id != TICKER_NULL &&
		    !ticker->ext_data->other_expire_info) {
			uint32_t status;

			status = ticker_alloc_expire_info(instance, user_op->id);
			if (status) {
				return status;
			}
		} else if (user_op->params.update.expire_info_id == TICKER_NULL &&
			 ticker->ext_data->other_expire_info) {
			ticker_free_expire_info(instance, user_op->id);
			ticker->ext_data->other_expire_info = NULL;
		}

		ticker->ext_data->expire_info_id = user_op->params.update.expire_info_id;
		if (ticker->ext_data->expire_info_id != TICKER_NULL) {
			ticker_mark_expire_info_outdated(instance, user_op->id);
		}
	}
#else /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	ARG_UNUSED(instance);
#endif /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	ticker->next = *insert_head;
	*insert_head = user_op->id;

	return TICKER_STATUS_SUCCESS;
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
					  uint32_t ticks_now,
					  uint32_t ticks_elapsed,
					  uint8_t *insert_head)
{
	/* Handle update of ticker by re-inserting it back. */
	if (IS_ENABLED(CONFIG_BT_TICKER_UPDATE) &&
	    (user_op->op == TICKER_USER_OP_TYPE_UPDATE)) {
		/* Remove ticker node from list */
		ticker->ticks_to_expire = ticker_dequeue(instance, user_op->id);

		/* Update node and insert back */
		ticker_job_node_update(instance, ticker, user_op, ticks_now,
				       instance->ticks_current, ticks_elapsed,
				       insert_head);

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
		ticker_mark_expire_info_outdated(instance, user_op->id);
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

		/* Set schedule status of node
		 * as updating.
		 */
		ticker->req++;
	} else {
		/* If stop/stop_abs requested, then dequeue node */
		if (user_op->op != TICKER_USER_OP_TYPE_YIELD_ABS) {
			/* Remove ticker node from list */
			ticker->ticks_to_expire = ticker_dequeue(instance,
								 user_op->id);

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
			if (ticker->ext_data && ticker->ext_data->expire_info_id != TICKER_NULL) {
				ticker_free_expire_info(instance, user_op->id);
				ticker->ext_data->other_expire_info = NULL;
			}

			ticker_mark_expire_info_outdated(instance, user_op->id);
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

			/* Reset schedule status of node */
			ticker->req = ticker->ack;
		}

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
		/* If yield_abs/stop/stop_abs then adjust ticks_slot_previous */
		if (instance->ticker_id_slot_previous == user_op->id) {
			uint32_t ticks_current;
			uint32_t ticks_at_yield;
			uint32_t ticks_used;

			if (user_op->op != TICKER_USER_OP_TYPE_YIELD_ABS) {
				instance->ticker_id_slot_previous = TICKER_NULL;
			}

			if ((user_op->op == TICKER_USER_OP_TYPE_YIELD_ABS) ||
			    (user_op->op == TICKER_USER_OP_TYPE_STOP_ABS)) {
				ticks_at_yield =
					user_op->params.yield.ticks_at_yield;
			} else {
				ticks_at_yield = ticks_now;
			}

			ticks_current = instance->ticks_current;
			if (!((ticks_at_yield - ticks_current) &
			      BIT(HAL_TICKER_CNTR_MSBIT))) {
				ticks_used = ticks_elapsed +
					ticker_ticks_diff_get(ticks_at_yield,
							      ticks_current);
			} else {
				ticks_used =
					ticker_ticks_diff_get(ticks_current,
							      ticks_at_yield);
				if (ticks_elapsed > ticks_used) {
					ticks_used = ticks_elapsed -
						     ticks_used;
				} else {
					ticks_used = 0;
				}
			}

			if (instance->ticks_slot_previous > ticks_used) {
				instance->ticks_slot_previous = ticks_used;
			}
		}
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

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
static inline uint8_t ticker_job_list_manage(struct ticker_instance *instance,
					     uint32_t ticks_now,
					     uint32_t ticks_elapsed,
					     uint8_t *insert_head)
{
	uint8_t pending;
	struct ticker_node *node;
	struct ticker_user *users;
	uint8_t count_user;

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
			uint8_t state;
			uint8_t prev;
			uint8_t middle;

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
#if defined(CONFIG_BT_TICKER_PREFER_START_BEFORE_STOP)
				if (user_op->op == TICKER_USER_OP_TYPE_START) {
					/* Set start pending to validate a
					 * successive, inline stop operation.
					 */
					ticker->start_pending = 1U;
				}
#endif /* CONFIG_BT_TICKER_PREFER_START_BEFORE_STOP */

				continue;
			}

			/* determine the ticker state */
			state = (ticker->req - ticker->ack) & 0xff;

			/* if not started or update not required,
			 * set status and continue.
			 */
			if ((user_op->op > TICKER_USER_OP_TYPE_STOP_ABS) ||
			    ((state == 0U) &&
#if defined(CONFIG_BT_TICKER_PREFER_START_BEFORE_STOP)
			     !ticker->start_pending &&
#endif /* CONFIG_BT_TICKER_PREFER_START_BEFORE_STOP */
			     (user_op->op != TICKER_USER_OP_TYPE_YIELD_ABS)) ||
			    ((user_op->op == TICKER_USER_OP_TYPE_UPDATE) &&
			     (user_op->params.update.ticks_drift_plus == 0U) &&
			     (user_op->params.update.ticks_drift_minus == 0U) &&
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
			     (user_op->params.update.ticks_slot_plus == 0U) &&
			     (user_op->params.update.ticks_slot_minus == 0U) &&
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */
#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
			     (!ticker->ext_data ||
				  user_op->params.update.expire_info_id == user_op->id) &&
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
			     (user_op->params.update.lazy == 0U) &&
			     (user_op->params.update.force == 0U))) {
				ticker_job_op_cb(user_op,
						 TICKER_STATUS_FAILURE);
				continue;
			}

			/* Delete or yield node, if not expired */
			if ((state == 1U) ||
			    (user_op->op == TICKER_USER_OP_TYPE_YIELD_ABS)) {
				ticker_job_node_manage(instance, ticker,
						       user_op, ticks_now,
						       ticks_elapsed,
						       insert_head);
			} else {
				/* Update on expired node requested, deferring
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
					uint32_t ticks_now,
					uint32_t ticks_previous,
					uint32_t ticks_elapsed,
					uint8_t *insert_head)
{
	struct ticker_node *node;
	uint32_t ticks_expired;
	uint32_t ticks_latency;

	ticks_latency = ticker_ticks_diff_get(ticks_now, ticks_previous);

	node = &instance->nodes[0];
	ticks_expired = 0U;
	while (instance->ticker_id_head != TICKER_NULL) {
		uint8_t skip_collision = 0U;
		struct ticker_node *ticker;
		uint32_t ticks_to_expire;
		uint8_t id_expired;
		uint8_t state;

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
		ticks_latency -= ticks_to_expire;
		ticks_expired += ticks_to_expire;

		state = (ticker->req - ticker->ack) & 0xff;

#if !defined(CONFIG_BT_TICKER_LOW_LAT)
		/* Node with lazy count did not expire with callback, but
		 * was either a collision or re-scheduled. This node should
		 * not define the active slot reservation (slot_previous).
		 */
		skip_collision = (ticker->lazy_current != 0U);
#endif /* !CONFIG_BT_TICKER_LOW_LAT */

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
		/* decrement ticks_slot_previous */
		if (instance->ticks_slot_previous > ticks_to_expire) {
			instance->ticks_slot_previous -= ticks_to_expire;
		} else {
			instance->ticker_id_slot_previous = TICKER_NULL;
			instance->ticks_slot_previous = 0U;
		}

		uint32_t ticker_ticks_slot;

		if (TICKER_HAS_SLOT_WINDOW(ticker) && !ticker->ticks_slot) {
			ticker_ticks_slot = HAL_TICKER_RESCHEDULE_MARGIN;
		} else {
			ticker_ticks_slot = ticker->ticks_slot;
		}

		/* If a reschedule is set pending, we will need to keep
		 * the slot_previous information
		 */
		if (ticker_ticks_slot && (state == 2U) && !skip_collision &&
		    !TICKER_RESCHEDULE_PENDING(ticker)) {
			instance->ticker_id_slot_previous = id_expired;
			instance->ticks_slot_previous = ticker_ticks_slot;
		}
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */

		/* ticker expired, set ticks_to_expire zero */
		ticker->ticks_to_expire = 0U;

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
		ticker_mark_expire_info_outdated(instance, instance->ticker_id_head);
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

		/* remove the expired ticker from head */
		instance->ticker_id_head = ticker->next;

		/* Ticker will be restarted if periodic or to be re-scheduled */
		if ((ticker->ticks_periodic != 0U) ||
		    TICKER_RESCHEDULE_PENDING(ticker)) {
#if !defined(CONFIG_BT_TICKER_LOW_LAT)
			if (TICKER_RESCHEDULE_PENDING(ticker)) {
				/* Set the re-scheduled node to now. Will be
				 * collision resolved after all nodes are
				 * restarted
				 */
				ticker->ticks_to_expire = ticks_elapsed;

				/* Reset ticker state, so that its put
				 * back in requested state later down
				 * in the code.
				 */
				ticker->req = ticker->ack;
			} else {
				uint16_t lazy_periodic;
				uint32_t count;
				uint16_t lazy;

				/* If not skipped, apply lazy_periodic */
				if (!ticker->lazy_current) {
					lazy_periodic = ticker->lazy_periodic;
				} else {
					lazy_periodic = 0U;

					/* Reset ticker state, so that its put
					 * back in requested state later down
					 * in the code.
					 */
					ticker->req = ticker->ack;
				}

				/* Reload ticks_to_expire with at least one
				 * period.
				 */
				ticks_to_expire = 0U;
				count = 1 + lazy_periodic;
				while (count--) {
					ticks_to_expire +=
						ticker->ticks_periodic;

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
					ticks_to_expire +=
						ticker_remainder_inc(ticker);
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */
				}

				/* Skip intervals that have elapsed w.r.t.
				 * current ticks.
				 */
				lazy = 0U;

				if (0) {
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
				} else if (!ticker->must_expire) {
#else
				} else {
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */
					while (ticks_to_expire <
					       ticks_latency) {
						ticks_to_expire +=
							ticker->ticks_periodic;

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
						ticks_to_expire +=
						  ticker_remainder_inc(ticker);
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

						lazy++;
					}
				}

				/* Use the calculated ticks to expire and
				 * laziness.
				 */
				ticker->ticks_to_expire = ticks_to_expire;
				ticker->lazy_current += (lazy_periodic + lazy);
			}

			ticks_to_expire_prep(ticker, instance->ticks_current,
					     ((ticks_previous + ticks_expired) &
					      HAL_TICKER_CNTR_MASK));
#else /* CONFIG_BT_TICKER_LOW_LAT */
			uint32_t count;
			uint16_t lazy;

			/* Prepare for next interval */
			ticks_to_expire = 0U;
			count = 1 + ticker->lazy_periodic;
			while (count--) {
				ticks_to_expire += ticker->ticks_periodic;
				ticks_to_expire += ticker_remainder_inc(ticker);
			}

			/* Skip intervals that have elapsed w.r.t. current
			 * ticks.
			 */
			lazy = 0U;

			/* Schedule to a tick in the future */
			while (ticks_to_expire < ticks_latency) {
				ticks_to_expire += ticker->ticks_periodic;
				ticks_to_expire += ticker_remainder_inc(ticker);
				lazy++;
			}

			/* Use the calculated ticks to expire and laziness. */
			ticker->ticks_to_expire = ticks_to_expire;
			ticker->lazy_current = ticker->lazy_periodic + lazy;

			ticks_to_expire_prep(ticker, instance->ticks_current,
					     ((ticks_previous + ticks_expired) &
					      HAL_TICKER_CNTR_MASK));

			/* Reset force state of the node */
			ticker->force = 0U;
#endif /* CONFIG_BT_TICKER_LOW_LAT */

			/* Add to insert list */
			ticker->next = *insert_head;
			*insert_head = id_expired;

			/* set schedule status of node as restarting. */
			ticker->req++;
		} else {
#if !defined(CONFIG_BT_TICKER_LOW_LAT)
			/* A single-shot ticker in requested or skipped due to
			 * collision shall generate a operation function
			 * callback with failure status.
			 */
			if (state && ((state == 1U) || skip_collision) &&
			    ticker->fp_op_func) {
				ticker->fp_op_func(TICKER_STATUS_FAILURE,
						   ticker->op_context);
			}
#endif /* !CONFIG_BT_TICKER_LOW_LAT */

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
static inline uint32_t ticker_job_op_start(struct ticker_instance *instance,
					   struct ticker_node *ticker,
					   struct ticker_user_op *user_op,
					   uint32_t ticks_current)
{
	struct ticker_user_op_start *start = (void *)&user_op->params.start;

#if defined(CONFIG_BT_TICKER_LOW_LAT)
	/* Must expire is not supported in compatibility mode */
	LL_ASSERT(start->lazy < TICKER_LAZY_MUST_EXPIRE_KEEP);
#else
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	if (start->lazy != TICKER_LAZY_MUST_EXPIRE_KEEP) {
		/* Update the must_expire state */
		ticker->must_expire =
			(start->lazy == TICKER_LAZY_MUST_EXPIRE) ? 1U : 0U;
	}
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */
#endif /* CONFIG_BT_TICKER_LOW_LAT */

#if defined(CONFIG_BT_TICKER_EXT)
	ticker->ext_data = start->ext_data;

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	if (ticker->ext_data) {
		ticker->ext_data->other_expire_info = NULL;
		if (ticker->ext_data->expire_info_id != TICKER_NULL) {
			uint32_t status;

			status = ticker_alloc_expire_info(instance, user_op->id);
			if (status) {
				return status;
			}
		}
	}

	ticker_mark_expire_info_outdated(instance, user_op->id);
#else /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
	ARG_UNUSED(instance);
#endif /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
#else /* !CONFIG_BT_TICKER_EXT */
	ARG_UNUSED(instance);
#endif /* !CONFIG_BT_TICKER_EXT */

	ticker->ticks_periodic = start->ticks_periodic;

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
	ticker->remainder_periodic = start->remainder_periodic;

#if defined(CONFIG_BT_TICKER_START_REMAINDER)
	ticker->remainder_current = start->remainder_first;
#else /* !CONFIG_BT_TICKER_START_REMAINDER */
	ticker->remainder_current = 0U;
#endif /* !CONFIG_BT_TICKER_START_REMAINDER */
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

	ticker->lazy_periodic =
		(start->lazy < TICKER_LAZY_MUST_EXPIRE_KEEP) ? start->lazy :
							       0U;
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	ticker->ticks_slot = start->ticks_slot;
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */

	ticker->timeout_func = start->fp_timeout_func;
	ticker->context = start->context;
	ticker->ticks_to_expire = start->ticks_first;
	ticker->ticks_to_expire_minus = 0U;
	ticks_to_expire_prep(ticker, ticks_current, start->ticks_at_start);

	ticker->lazy_current = 0U;
	ticker->force = 1U;

	return TICKER_STATUS_SUCCESS;
}

#if !defined(CONFIG_BT_TICKER_LOW_LAT)
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
static inline uint8_t ticker_job_insert(struct ticker_instance *instance,
				      uint8_t id_insert,
				      struct ticker_node *ticker,
				      uint8_t *insert_head)
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

#if defined(CONFIG_BT_TICKER_EXT) && !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
/**
 * @brief Re-schedule ticker nodes within slot_window
 *
 * @details This function is responsible for re-scheduling ticker nodes
 * which have been marked for re-scheduling in ticker_worker. These nodes
 * have a non-zero ticks_slot_window configuration, which indicates a
 * valid range in which to re-schedule the node.
 * The function iterates over nodes, and handles one re-schedule at a
 * time. After a re-schedule, nodes are once again iterated until no more
 * nodes are marked for re-scheduling.
 *
 * @param instance      Pointer to ticker instance
 * @param ticks_elapsed Number of ticks elapsed since last ticker job
 *
 * @internal
 */
static uint8_t ticker_job_reschedule_in_window(struct ticker_instance *instance)
{
	struct ticker_node *nodes;
	uint8_t rescheduling;
	uint8_t rescheduled;

	nodes = &instance->nodes[0];

	/* Do until all pending re-schedules handled */
	rescheduling = 1U;
	rescheduled = 0U;
	while (rescheduling) {
		struct ticker_node *ticker_resched;
		uint32_t ticks_to_expire_offset;
		uint8_t ticker_id_resched_prev;
		struct ticker_ext  *ext_data;
		uint32_t ticks_start_offset;
		uint32_t window_start_ticks;
		uint32_t ticks_slot_window;
		uint8_t ticker_id_resched;
		uint32_t ticks_to_expire;
		uint8_t ticker_id_prev;
		uint8_t ticker_id_next;
		uint32_t ticks_slot;

		rescheduling = 0U;

		/* Find first pending re-schedule */
		ticker_id_resched_prev = TICKER_NULL;
		ticker_id_resched = instance->ticker_id_head;
		while (ticker_id_resched != TICKER_NULL) {
			ticker_resched = &nodes[ticker_id_resched];
			if (TICKER_RESCHEDULE_PENDING(ticker_resched)) {
				/* Pending reschedule found */
				break;
			}

			ticker_id_resched_prev = ticker_id_resched;
			ticker_id_resched = ticker_resched->next;
		}

		/* Exit if no tickers to be rescheduled */
		if (ticker_id_resched == TICKER_NULL) {
			break;
		}

		/* Ensure that resched ticker is expired */
		LL_ASSERT(ticker_resched->ticks_to_expire == 0U);

		/* Window start after intersection with already active node */
		window_start_ticks = instance->ticks_slot_previous +
				     HAL_TICKER_RESCHEDULE_MARGIN;

		/* If drift was applied to this node, this must be
		 * taken into consideration. Reduce the window with
		 * the amount of drift already applied.
		 *
		 * TODO: An improvement on this could be to only consider the
		 * drift (ADV => randomization) if re-sceduling fails. Then the
		 * ticker would have the best possible window to re-schedule in
		 * and not be restricted to ticks_slot_window - ticks_drift.
		 */
		ext_data = ticker_resched->ext_data;
		if (ext_data->ticks_drift < ext_data->ticks_slot_window) {
			ticks_slot_window = ext_data->ticks_slot_window -
					    ext_data->ticks_drift;
			/* Window available, proceed to calculate further
			 * drift
			 */
			ticker_id_next = ticker_resched->next;
		} else {
			/* Window has been exhausted - we can't reschedule */
			ticker_id_next = TICKER_NULL;

			/* Assignment will be unused when TICKER_NULL */
			ticks_slot_window = 0U;
		}

		/* Use ticker's reserved time ticks_slot, else for unreserved
		 * tickers use the reschedule margin as ticks_slot.
		 */
		if (ticker_resched->ticks_slot) {
			ticks_slot = ticker_resched->ticks_slot;
		} else {
			LL_ASSERT(TICKER_HAS_SLOT_WINDOW(ticker_resched));

			ticks_slot = HAL_TICKER_RESCHEDULE_MARGIN;
		}

		/* Try to find available slot for re-scheduling */
		ticks_to_expire_offset = 0U;
		ticks_start_offset = 0U;
		ticks_to_expire = 0U;
		while ((ticker_id_next != TICKER_NULL) &&
		       ((ticks_start_offset + ticks_slot) <=
			ticks_slot_window)) {
			struct ticker_node *ticker_next;
			uint32_t window_end_ticks;

			ticker_next = &nodes[ticker_id_next];
			ticks_to_expire_offset += ticker_next->ticks_to_expire;

			/* Calculate end of window. Since window may be aligned
			 * with expiry of next node, we add a margin
			 */
			if (ticks_to_expire_offset >
			    HAL_TICKER_RESCHEDULE_MARGIN) {
				window_end_ticks =
					MIN(ticks_slot_window,
					    ticks_start_offset +
					    ticks_to_expire_offset -
					    HAL_TICKER_RESCHEDULE_MARGIN);
			} else {
				/* Next expiry is too close - try the next
				 * node
				 */
				window_end_ticks = 0U;
			}

			/* Calculate new ticks_to_expire as end of window minus
			 * slot size.
			 */
			if (((window_start_ticks + ticks_slot) <=
			     ticks_slot_window) &&
			    (window_end_ticks >= (ticks_start_offset +
						 ticks_slot))) {
				if (!ticker_resched->ticks_slot ||
				    ext_data->is_drift_in_window) {
					/* Place at start of window */
					ticks_to_expire = window_start_ticks;
				} else {
					/* Place at end of window. This ensures
					 * that ticker with slot window and that
					 * uses ticks_slot does not take the
					 * interval of the colliding ticker.
					 */
					ticks_to_expire = window_end_ticks -
							  ticks_slot;
				}
			} else {
				/* No space in window - try the next node */
				ticks_to_expire = 0U;
			}

			/* Skip other pending re-schedule nodes and
			 * tickers with no reservation or not periodic
			 */
			if (TICKER_RESCHEDULE_PENDING(ticker_next) ||
			    !ticker_next->ticks_slot ||
			    !ticker_next->ticks_periodic) {
				ticker_id_next = ticker_next->next;

				continue;
			}

			/* Decide if the re-scheduling ticker node fits in the
			 * slot found - break if it fits
			 */
			if ((ticks_to_expire != 0U) &&
			    (ticks_to_expire >= window_start_ticks) &&
			    (ticks_to_expire <= (window_end_ticks -
						 ticks_slot))) {
				/* Re-scheduled node fits before this node */
				break;
			} else {
				/* Not inside the window */
				ticks_to_expire = 0U;
			}

			/* We din't find a valid slot for re-scheduling - try
			 * the next node
			 */
			ticks_start_offset += ticks_to_expire_offset;
			window_start_ticks  = ticks_start_offset +
					      ticker_next->ticks_slot +
					      HAL_TICKER_RESCHEDULE_MARGIN;
			ticks_to_expire_offset = 0U;

			if (!ticker_resched->ticks_slot ||
			    ext_data->is_drift_in_window) {
				if (!ticker_resched->ticks_slot ||
				    (window_start_ticks <= (ticks_slot_window -
							   ticks_slot))) {
					/* Try at the end of the next node */
					ticks_to_expire = window_start_ticks;
				}
			} else if (IS_ENABLED(CONFIG_BT_TICKER_EXT_SLOT_WINDOW_YIELD) &&
				   (ticker_resched->ticks_periodic <
				    ticker_next->ticks_periodic)) {
				uint32_t ticks_slot_with_margin = ticker_resched->ticks_slot +
								  HAL_TICKER_RESCHEDULE_MARGIN;

				/* Try to place it before the overlap and be
				 * rescheduled to its next periodic interval
				 * for collision resolution.
				 */
				if (ticks_start_offset > ticks_slot_with_margin) {
					ticks_to_expire = ticks_start_offset -
							  ticks_slot_with_margin;
				}
			} else {
				/* Try at the end of the slot window. This
				 * ensures that ticker with slot window and that
				 * uses ticks_slot does not take the interval of
				 * the colliding ticker.
				 */
				ticks_to_expire = ticks_slot_window -
						  ticks_slot;
			}

			ticker_id_next = ticker_next->next;
		}

		ext_data->ticks_drift += ticks_to_expire;

		/* Place the ticker node sorted by expiration time and adjust
		 * delta times
		 */
		ticker_id_next = ticker_resched->next;
		ticker_id_prev = TICKER_NULL;
		while (ticker_id_next != TICKER_NULL) {
			struct ticker_node *ticker_next;

			ticker_next = &nodes[ticker_id_next];
			if (ticks_to_expire > ticker_next->ticks_to_expire) {
				/* Node is after this - adjust delta */
				ticks_to_expire -= ticker_next->ticks_to_expire;
			} else {
				/* Node is before this one */
				ticker_next->ticks_to_expire -= ticks_to_expire;
				break;
			}
			ticker_id_prev = ticker_id_next;
			ticker_id_next = ticker_next->next;
		}

		ticker_resched->ticks_to_expire = ticks_to_expire;

		/* If the node moved in the list, insert it */
		if (ticker_id_prev != TICKER_NULL) {
			/* Remove node from its current position in list */
			if (ticker_id_resched_prev != TICKER_NULL) {
				/* Node was not at the head of the list */
				nodes[ticker_id_resched_prev].next =
					ticker_resched->next;
			} else {
				/* Node was at the head, move head forward */
				instance->ticker_id_head = ticker_resched->next;
			}

			/* Link inserted node */
			ticker_resched->next = nodes[ticker_id_prev].next;
			nodes[ticker_id_prev].next = ticker_id_resched;
		}

		/* Remove latency added in ticker_worker */
		ticker_resched->lazy_current--;

		/* Prevent repeated re-scheduling */
		ext_data->reschedule_state =
			TICKER_RESCHEDULE_STATE_DONE;

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
		ticker_mark_expire_info_outdated(instance, ticker_id_resched);
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

		/* Check for other pending re-schedules and set exit flag */
		rescheduling = 1U;
		rescheduled  = 1U;
	}

	return rescheduled;
}
#endif /* CONFIG_BT_TICKER_EXT && !CONFIG_BT_TICKER_SLOT_AGNOSTIC */
#else  /* CONFIG_BT_TICKER_LOW_LAT */

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
static inline uint8_t ticker_job_insert(struct ticker_instance *instance,
				      uint8_t id_insert,
				      struct ticker_node *ticker,
				      uint8_t *insert_head)
{
	struct ticker_node *node = &instance->nodes[0];
	uint8_t id_collide;
	uint16_t skip;

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
			uint16_t skip_collide;

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

			/* No. of times ticker has skipped its interval */
			if (ticker->lazy_current > ticker->lazy_periodic) {
				skip = ticker->lazy_current -
				       ticker->lazy_periodic;
			} else {
				skip = 0U;
			}

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
#endif /* CONFIG_BT_TICKER_LOW_LAT */

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
					  uint8_t insert_head)
{
	struct ticker_node *node;
	struct ticker_user *users;
	uint8_t count_user;

	node = &instance->nodes[0];
	users = &instance->users[0];
	count_user = instance->count_user;

	/* Iterate through all user ids */
	while (count_user--) {
		struct ticker_user_op *user_ops;
		struct ticker_user *user;
		uint8_t user_ops_first;

		user = &users[count_user];
		user_ops = (void *)&user->user_op[0];
		user_ops_first = user->first;
		/* Traverse user operation queue - first to middle (wrap) */
		while ((insert_head != TICKER_NULL) ||
		       (user_ops_first != user->middle)) {
			struct ticker_user_op *user_op;
			struct ticker_node *ticker;
			uint8_t id_insert;
			uint8_t status = TICKER_STATUS_SUCCESS;

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
				uint8_t first;

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

#if defined(CONFIG_BT_TICKER_PREFER_START_BEFORE_STOP)
				ticker->start_pending = 0U;
#endif /* CONFIG_BT_TICKER_PREFER_START_BEFORE_STOP */

				if (((ticker->req -
				      ticker->ack) & 0xff) != 0U) {
					ticker_job_op_cb(user_op,
							 TICKER_STATUS_FAILURE);
					continue;
				}

				/* Prepare ticker for start */
				status = ticker_job_op_start(instance, ticker, user_op,
						    instance->ticks_current);
			}

			if (!status) {
				/* Insert ticker node */
				status = ticker_job_insert(instance, id_insert, ticker,
							   &insert_head);
			}

			if (user_op) {
				ticker_job_op_cb(user_op, status);

				if (!IS_ENABLED(CONFIG_BT_TICKER_LOW_LAT) &&
				    (ticker->ticks_periodic == 0U) &&
				    user_op) {
					ticker->fp_op_func =
						user_op->fp_op_func;
					ticker->op_context =
						user_op->op_context;
				}
			}
		}

#if !defined(CONFIG_BT_TICKER_JOB_IDLE_GET) && \
	!defined(CONFIG_BT_TICKER_NEXT_SLOT_GET) && \
	!defined(CONFIG_BT_TICKER_PRIORITY_SET)
		user->first = user_ops_first;
#endif /* !CONFIG_BT_TICKER_JOB_IDLE_GET &&
	* !CONFIG_BT_TICKER_NEXT_SLOT_GET &&
	* !CONFIG_BT_TICKER_PRIORITY_SET
	*/

	}
}

#if defined(CONFIG_BT_TICKER_JOB_IDLE_GET) || \
	defined(CONFIG_BT_TICKER_NEXT_SLOT_GET) || \
	defined(CONFIG_BT_TICKER_PRIORITY_SET)
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
#if defined(CONFIG_BT_TICKER_NEXT_SLOT_GET)
	case TICKER_USER_OP_TYPE_SLOT_GET:
		ticker_by_next_slot_get(instance,
					uop->params.slot_get.ticker_id,
					uop->params.slot_get.ticks_current,
					uop->params.slot_get.ticks_to_expire,
#if defined(CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH)
					uop->params.slot_get.fp_match_op_func,
					uop->params.slot_get.match_op_context,
#else
					NULL, NULL,
#endif
#if defined(CONFIG_BT_TICKER_REMAINDER_GET)
					uop->params.slot_get.remainder,
#else /* !CONFIG_BT_TICKER_REMAINDER_GET */
					NULL,
#endif /* !CONFIG_BT_TICKER_REMAINDER_GET */
#if defined(CONFIG_BT_TICKER_LAZY_GET)
					uop->params.slot_get.lazy);
#else /* !CONFIG_BT_TICKER_LAZY_GET */
					NULL);
#endif /* !CONFIG_BT_TICKER_LAZY_GET */
		__fallthrough;
#endif /* CONFIG_BT_TICKER_NEXT_SLOT_GET */

#if defined(CONFIG_BT_TICKER_JOB_IDLE_GET) || \
	defined(CONFIG_BT_TICKER_NEXT_SLOT_GET)
	case TICKER_USER_OP_TYPE_IDLE_GET:
		uop->status = TICKER_STATUS_SUCCESS;
		fp_op_func = uop->fp_op_func;
		break;
#endif /* CONFIG_BT_TICKER_JOB_IDLE_GET ||
	* CONFIG_BT_TICKER_NEXT_SLOT_GET
	*/

#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC) && \
	defined(CONFIG_BT_TICKER_PRIORITY_SET)
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
#endif /* !CONFIG_BT_TICKER_LOW_LAT &&
	* !CONFIG_BT_TICKER_SLOT_AGNOSTIC
	* CONFIG_BT_TICKER_PRIORITY_SET
	*/

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
	uint8_t count_user;

	users = &instance->users[0];
	count_user = instance->count_user;
	/* Traverse user operation queue - first to last (with wrap) */
	while (count_user--) {
		struct ticker_user_op *user_op;
		struct ticker_user *user;

		user = &users[count_user];
		user_op = &user->user_op[0];
		while (user->first != user->last) {
			uint8_t first;

			ticker_job_op_inquire(instance, &user_op[user->first]);

			first = user->first + 1;
			if (first == user->count_user_op) {
				first = 0U;
			}
			user->first = first;
		}
	}
}
#endif /* CONFIG_BT_TICKER_JOB_IDLE_GET ||
	* CONFIG_BT_TICKER_NEXT_SLOT_GET ||
	* CONFIG_BT_TICKER_PRIORITY_SET
	*/

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
static inline uint8_t
ticker_job_compare_update(struct ticker_instance *instance,
			  uint8_t ticker_id_old_head)
{
	struct ticker_node *ticker;
	uint32_t ticks_to_expire;
	uint32_t ctr_curr;
	uint32_t ctr_prev;
	uint32_t cc;
	uint32_t i;

	if (instance->ticker_id_head == TICKER_NULL) {
		if (cntr_stop() == 0) {
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
			instance->ticks_slot_previous = 0U;
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

#if !defined(CONFIG_BT_TICKER_CNTR_FREE_RUNNING)
			/* Stopped counter value will be used as ticks_current
			 * for calculation to start new tickers.
			 */
			instance->ticks_current = cntr_cnt_get();
#endif /* !CONFIG_BT_TICKER_CNTR_FREE_RUNNING */
		}

		return 0U;
	}

	/* Check if this is the first update. If so, start the counter */
	if (ticker_id_old_head == TICKER_NULL) {
#if !defined(CONFIG_BT_TICKER_CNTR_FREE_RUNNING)
		uint32_t ticks_current;

		ticks_current = cntr_cnt_get();
#endif /* !CONFIG_BT_TICKER_CNTR_FREE_RUNNING */

		if (cntr_start() == 0) {
#if !defined(CONFIG_BT_TICKER_CNTR_FREE_RUNNING)
			/* Stopped counter value will be used as ticks_current
			 * for calculation to start new tickers.
			 * FIXME: We do not need to synchronize here, instead
			 *        replace with check to ensure the counter value
			 *        has not since that synchronization when the
			 *        counter with in stopped state.
			 */
			instance->ticks_current = ticks_current;
#endif /* !CONFIG_BT_TICKER_CNTR_FREE_RUNNING */
		}
	}

	ticker = &instance->nodes[instance->ticker_id_head];
	ticks_to_expire = ticker->ticks_to_expire;

	/* If ticks_to_expire is zero, then immediately trigger the worker.
	 */
	if (!ticks_to_expire) {
		return 1U;
	}

	/* Iterate few times, if required, to ensure that compare is
	 * correctly set to a future value. This is required in case
	 * the operation is pre-empted and current h/w counter runs
	 * ahead of compare value to be set.
	 */
	i = 10U;
	ctr_curr = cntr_cnt_get();
	do {
		uint32_t ticks_elapsed;
		uint32_t ticks_diff;

		LL_ASSERT(i);
		i--;

		cc = instance->ticks_current;
		ticks_diff = ticker_ticks_diff_get(ctr_curr, cc);
		if (ticks_diff >= ticks_to_expire) {
			return 1U;
		}

		ticks_elapsed = ticks_diff + HAL_TICKER_CNTR_CMP_OFFSET_MIN +
				HAL_TICKER_CNTR_SET_LATENCY;
		cc += MAX(ticks_elapsed, ticks_to_expire);
		cc &= HAL_TICKER_CNTR_MASK;
		instance->trigger_set_cb(cc);

		ctr_prev = ctr_curr;
		ctr_curr = cntr_cnt_get();
	} while ((ticker_ticks_diff_get(ctr_curr, ctr_prev) +
		  HAL_TICKER_CNTR_CMP_OFFSET_MIN) >
		  ticker_ticks_diff_get(cc, ctr_prev));

	return 0U;
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
	uint8_t flag_compare_update;
	uint8_t ticker_id_old_head;
	uint8_t compare_trigger;
	uint32_t ticks_previous;
	uint32_t ticks_elapsed;
	uint8_t flag_elapsed;
	uint8_t insert_head;
	uint32_t ticks_now;
	uint8_t pending;

	DEBUG_TICKER_JOB(1);

	/* Defer job, as worker is running */
	if (instance->worker_trigger) {
		DEBUG_TICKER_JOB(0);
		return;
	}

	/* Defer job, as job is already running */
	if (instance->job_guard) {
		instance->sched_cb(TICKER_CALL_ID_JOB, TICKER_CALL_ID_JOB, 1,
				   instance);
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

	/* Get current ticks, used in managing updates and expired tickers */
	ticks_now = cntr_cnt_get();

#if defined(CONFIG_BT_TICKER_CNTR_FREE_RUNNING)
	if (ticker_id_old_head == TICKER_NULL) {
		/* No tickers active, synchronize to the free running counter so
		 * that any new ticker started can have its ticks_to_expire
		 * relative to current free running counter value.
		 *
		 * Both current tick (new value) and previous tick (previously
		 * stored when all tickers stopped) is assigned to ticks_now.
		 * All new tickers are started from this synchronized value as
		 * the anchor/reference value.
		 *
		 * Note, this if clause is an overhead wherein the check is
		 * performed for every ticker_job() iteration!
		 */
		instance->ticks_current = ticks_now;
		ticks_previous = ticks_now;
	}
#endif /* !CONFIG_BT_TICKER_CNTR_FREE_RUNNING */

	/* Manage user operations (updates and deletions) in ticker list */
	pending = ticker_job_list_manage(instance, ticks_now, ticks_elapsed,
					 &insert_head);

	/* Detect change in head of the list */
	if (instance->ticker_id_head != ticker_id_old_head) {
		flag_compare_update = 1U;
	}

	/* Handle expired tickers */
	if (flag_elapsed) {
		ticker_job_worker_bh(instance, ticks_now, ticks_previous,
				     ticks_elapsed, &insert_head);

		/* Detect change in head of the list */
		if (instance->ticker_id_head != ticker_id_old_head) {
			flag_compare_update = 1U;
		}

		/* Handle insertions */
		ticker_job_list_insert(instance, insert_head);

#if defined(CONFIG_BT_TICKER_EXT) && !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC) &&\
	!defined(CONFIG_BT_TICKER_LOW_LAT)
		/* Re-schedule any pending nodes with slot_window */
		if (ticker_job_reschedule_in_window(instance)) {
			flag_compare_update = 1U;
		}
#endif /* CONFIG_BT_TICKER_EXT */
	} else {
		/* Handle insertions */
		ticker_job_list_insert(instance, insert_head);
	}

	/* Detect change in head of the list */
	if (instance->ticker_id_head != ticker_id_old_head) {
		flag_compare_update = 1U;
	}

#if defined(CONFIG_BT_TICKER_JOB_IDLE_GET) || \
	defined(CONFIG_BT_TICKER_NEXT_SLOT_GET) || \
	defined(CONFIG_BT_TICKER_PRIORITY_SET)
	/* Process any list inquiries */
	if (!pending) {
		/* Handle inquiries */
		ticker_job_list_inquire(instance);
	}
#else  /* !CONFIG_BT_TICKER_JOB_IDLE_GET &&
	* !CONFIG_BT_TICKER_NEXT_SLOT_GET &&
	* !CONFIG_BT_TICKER_PRIORITY_SET
	*/
	ARG_UNUSED(pending);
#endif /* !CONFIG_BT_TICKER_JOB_IDLE_GET &&
	* !CONFIG_BT_TICKER_NEXT_SLOT_GET &&
	* !CONFIG_BT_TICKER_PRIORITY_SET
	*/

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	if (instance->expire_infos_outdated) {
		ticker_job_update_expire_infos(instance);
	}
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	/* update compare if head changed */
	if (flag_compare_update) {
		compare_trigger = ticker_job_compare_update(instance,
							    ticker_id_old_head);
	} else {
		compare_trigger = 0U;
	}

	/* Permit worker to run */
	instance->job_guard = 0U;

	/* trigger worker if deferred */
	cpu_dmb();
	if (instance->worker_trigger || compare_trigger) {
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
uint8_t ticker_init(uint8_t instance_index, uint8_t count_node, void *node,
		  uint8_t count_user, void *user, uint8_t count_op, void *user_op,
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

#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC) && \
	defined(CONFIG_BT_TICKER_PRIORITY_SET)
	while (count_node--) {
		instance->nodes[count_node].priority = 0;
	}
#endif /* !CONFIG_BT_TICKER_LOW_LAT &&
	* !CONFIG_BT_TICKER_SLOT_AGNOSTIC
	* CONFIG_BT_TICKER_PRIORITY_SET
	*/

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
#if defined(CONFIG_BT_TICKER_CNTR_FREE_RUNNING)
	/* We will synchronize in ticker_job on first ticker start */
	instance->ticks_current = 0U;
#else /* !CONFIG_BT_TICKER_CNTR_FREE_RUNNING */
	/* Synchronize to initialized (in stopped state) counter value */
	instance->ticks_current = cntr_cnt_get();
#endif /* !CONFIG_BT_TICKER_CNTR_FREE_RUNNING */
	instance->ticks_elapsed_first = 0U;
	instance->ticks_elapsed_last = 0U;

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	instance->ticker_id_slot_previous = TICKER_NULL;
	instance->ticks_slot_previous = 0U;
#endif /* !CONFIG_BT_TICKER_SLOT_AGNOSTIC */

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	for (int i = 0; i < TICKER_EXPIRE_INFO_MAX; i++) {
		instance->expire_infos[i].ticker_id = TICKER_NULL;
		instance->expire_infos[i].last = 1;
	}
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	return TICKER_STATUS_SUCCESS;
}

/**
 * @brief Check if ticker instance is initialized
 *
 * @param instance_index Index of ticker instance
 *
 * @return true if ticker instance is initialized, false otherwise
 */
bool ticker_is_initialized(uint8_t instance_index)
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
void ticker_trigger(uint8_t instance_index)
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
 * @param ticks_periodic     Number of ticks for a periodic ticker node. If 0,
 *			     ticker node is treated as one-shot
 * @param remainder_periodic Periodic ticks fraction
 * @param lazy		     Number of periods to skip (latency). A value of 1
 *			     causes skipping every other timeout
 * @param ticks_slot	     Slot reservation ticks for node (air-time)
 * @param ticks_slot_window  Window in which the slot reservation may be
 *			     re-scheduled to avoid collision. Set to 0 for
 *			     legacy behavior
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
#if defined(CONFIG_BT_TICKER_EXT)
uint8_t ticker_start(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		   uint32_t ticks_anchor, uint32_t ticks_first, uint32_t ticks_periodic,
		   uint32_t remainder_periodic, uint16_t lazy, uint32_t ticks_slot,
		   ticker_timeout_func fp_timeout_func, void *context,
		   ticker_op_func fp_op_func, void *op_context)
{
	return ticker_start_ext(instance_index, user_id, ticker_id,
				ticks_anchor, ticks_first, ticks_periodic,
				remainder_periodic, lazy, ticks_slot,
				fp_timeout_func, context,
				fp_op_func, op_context,
				NULL);
}

static uint8_t start_us(uint8_t instance_index, uint8_t user_id,
			uint8_t ticker_id, uint32_t ticks_anchor,
			uint32_t ticks_first, uint32_t remainder_first,
			uint32_t ticks_periodic, uint32_t remainder_periodic,
			uint16_t lazy, uint32_t ticks_slot,
			ticker_timeout_func fp_timeout_func, void *context,
			ticker_op_func fp_op_func, void *op_context,
			struct ticker_ext *ext_data);

uint8_t ticker_start_us(uint8_t instance_index, uint8_t user_id,
			uint8_t ticker_id, uint32_t ticks_anchor,
			uint32_t ticks_first, uint32_t remainder_first,
			uint32_t ticks_periodic, uint32_t remainder_periodic,
			uint16_t lazy, uint32_t ticks_slot,
			ticker_timeout_func fp_timeout_func, void *context,
			ticker_op_func fp_op_func, void *op_context)
{
	return start_us(instance_index, user_id, ticker_id, ticks_anchor,
			ticks_first, remainder_first,
			ticks_periodic, remainder_periodic,
			lazy, ticks_slot,
			fp_timeout_func, context,
			fp_op_func, op_context,
			NULL);
}

uint8_t ticker_start_ext(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		       uint32_t ticks_anchor, uint32_t ticks_first,
		       uint32_t ticks_periodic, uint32_t remainder_periodic,
		       uint16_t lazy, uint32_t ticks_slot,
		       ticker_timeout_func fp_timeout_func, void *context,
		       ticker_op_func fp_op_func, void *op_context,
		       struct ticker_ext *ext_data)
{
	return start_us(instance_index, user_id, ticker_id, ticks_anchor,
			ticks_first, 0U, ticks_periodic, remainder_periodic,
			lazy, ticks_slot,
			fp_timeout_func, context,
			fp_op_func, op_context,
			ext_data);
}

static uint8_t start_us(uint8_t instance_index, uint8_t user_id,
			uint8_t ticker_id, uint32_t ticks_anchor,
			uint32_t ticks_first, uint32_t remainder_first,
			uint32_t ticks_periodic, uint32_t remainder_periodic,
			uint16_t lazy, uint32_t ticks_slot,
			ticker_timeout_func fp_timeout_func, void *context,
			ticker_op_func fp_op_func, void *op_context,
			struct ticker_ext *ext_data)

#else /* !CONFIG_BT_TICKER_EXT */
uint8_t ticker_start(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		   uint32_t ticks_anchor, uint32_t ticks_first, uint32_t ticks_periodic,
		   uint32_t remainder_periodic, uint16_t lazy, uint32_t ticks_slot,
		   ticker_timeout_func fp_timeout_func, void *context,
		   ticker_op_func fp_op_func, void *op_context)
{
	return ticker_start_us(instance_index, user_id,
			       ticker_id, ticks_anchor,
			       ticks_first, 0U,
			       ticks_periodic, remainder_periodic,
			       lazy, ticks_slot,
			       fp_timeout_func, context,
			       fp_op_func, op_context);
}

uint8_t ticker_start_us(uint8_t instance_index, uint8_t user_id,
			uint8_t ticker_id, uint32_t ticks_anchor,
			uint32_t ticks_first, uint32_t remainder_first,
			uint32_t ticks_periodic, uint32_t remainder_periodic,
			uint16_t lazy, uint32_t ticks_slot,
			ticker_timeout_func fp_timeout_func, void *context,
			ticker_op_func fp_op_func, void *op_context)
#endif /* !CONFIG_BT_TICKER_EXT */

{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	uint8_t last;

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

#if defined(CONFIG_BT_TICKER_REMAINDER_SUPPORT)
	user_op->params.start.remainder_periodic = remainder_periodic;

#if defined(CONFIG_BT_TICKER_START_REMAINDER)
	user_op->params.start.remainder_first = remainder_first;
#else /* !CONFIG_BT_TICKER_START_REMAINDER */
	ARG_UNUSED(remainder_first);
#endif /* !CONFIG_BT_TICKER_START_REMAINDER */
#endif /* CONFIG_BT_TICKER_REMAINDER_SUPPORT */

#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	user_op->params.start.ticks_slot = ticks_slot;
#endif
	user_op->params.start.lazy = lazy;
#if defined(CONFIG_BT_TICKER_EXT)
	user_op->params.start.ext_data = ext_data;
#endif
	user_op->params.start.fp_timeout_func = fp_timeout_func;
	user_op->params.start.context = context;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	/* Make sure transaction is completed before committing */
	cpu_dmb();
	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}

#if defined(CONFIG_BT_TICKER_UPDATE)
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
 * @param must_expire	     Disable, enable or ignore the must-expire state.
 *			     A value of 0 means no change, 1 means disable and
 *			     2 means enable.
 *
 * @return TICKER_STATUS_BUSY if update was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to run
 * before exiting ticker_update
 */
uint8_t ticker_update(uint8_t instance_index, uint8_t user_id,
		       uint8_t ticker_id, uint32_t ticks_drift_plus,
		       uint32_t ticks_drift_minus, uint32_t ticks_slot_plus,
		       uint32_t ticks_slot_minus, uint16_t lazy, uint8_t force,
		       ticker_op_func fp_op_func, void *op_context)
#if defined(CONFIG_BT_TICKER_EXT)
#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
{
	return ticker_update_ext(instance_index, user_id, ticker_id,
				 ticks_drift_plus, ticks_drift_minus,
				 ticks_slot_plus, ticks_slot_minus, lazy,
				 force, fp_op_func, op_context, 0U, ticker_id);
}

uint8_t ticker_update_ext(uint8_t instance_index, uint8_t user_id,
			   uint8_t ticker_id, uint32_t ticks_drift_plus,
			   uint32_t ticks_drift_minus,
			   uint32_t ticks_slot_plus, uint32_t ticks_slot_minus,
			   uint16_t lazy, uint8_t force,
			   ticker_op_func fp_op_func, void *op_context,
			   uint8_t must_expire, uint8_t expire_info_id)
#else /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
{
	return ticker_update_ext(instance_index, user_id, ticker_id,
				 ticks_drift_plus, ticks_drift_minus,
				 ticks_slot_plus, ticks_slot_minus, lazy,
				 force, fp_op_func, op_context, 0U);
}

uint8_t ticker_update_ext(uint8_t instance_index, uint8_t user_id,
			   uint8_t ticker_id, uint32_t ticks_drift_plus,
			   uint32_t ticks_drift_minus,
			   uint32_t ticks_slot_plus, uint32_t ticks_slot_minus,
			   uint16_t lazy, uint8_t force,
			   ticker_op_func fp_op_func, void *op_context,
			   uint8_t must_expire)
#endif /* !CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
#endif /* CONFIG_BT_TICKER_EXT */
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	uint8_t last;

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
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC)
	user_op->params.update.ticks_slot_plus = ticks_slot_plus;
	user_op->params.update.ticks_slot_minus = ticks_slot_minus;
#endif /* CONFIG_BT_TICKER_SLOT_AGNOSTIC */
	user_op->params.update.lazy = lazy;
	user_op->params.update.force = force;
#if defined(CONFIG_BT_TICKER_EXT)
#if !defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC) && !defined(CONFIG_BT_TICKER_LOW_LAT)
	user_op->params.update.must_expire = must_expire;
#endif /* CONFIG_BT_TICKER_EXT && !CONFIG_BT_TICKER_SLOT_AGNOSTIC && !CONFIG_BT_TICKER_LOW_LAT */
#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	user_op->params.update.expire_info_id = expire_info_id;
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */
#endif /* CONFIG_BT_TICKER_EXT */
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	/* Make sure transaction is completed before committing */
	cpu_dmb();
	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}
#endif /* CONFIG_BT_TICKER_UPDATE */

/**
 * @brief Yield a ticker node with supplied absolute ticks reference
 *
 * @details Creates a new user operation of type TICKER_USER_OP_TYPE_YIELD_ABS
 * and schedules the ticker_job.
 *
 * @param instance_index     Index of ticker instance
 * @param user_id	     Ticker user id. Used for indexing user operations
 *			     and mapping to mayfly caller id
 * @param ticks_at_yield     Absolute tick count at ticker yield request
 * @param fp_op_func	     Function pointer of user operation completion
 *			     function
 * @param op_context	     Context passed in operation completion call
 *
 * @return TICKER_STATUS_BUSY if stop was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to run
 * before exiting ticker_stop
 */
uint8_t ticker_yield_abs(uint8_t instance_index, uint8_t user_id,
			  uint8_t ticker_id, uint32_t ticks_at_yield,
			  ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	uint8_t last;

	user = &instance->users[user_id];

	last = user->last + 1;
	if (last >= user->count_user_op) {
		last = 0U;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_YIELD_ABS;
	user_op->id = ticker_id;
	user_op->params.yield.ticks_at_yield = ticks_at_yield;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	/* Make sure transaction is completed before committing */
	cpu_dmb();
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
uint8_t ticker_stop(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
		  ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	uint8_t last;

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

	/* Make sure transaction is completed before committing */
	cpu_dmb();
	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}

/**
 * @brief Stop a ticker node with supplied absolute ticks reference
 *
 * @details Creates a new user operation of type TICKER_USER_OP_TYPE_STOP_ABS
 * and schedules the ticker_job.
 *
 * @param instance_index     Index of ticker instance
 * @param user_id	     Ticker user id. Used for indexing user operations
 *			     and mapping to mayfly caller id
 * @param ticks_at_stop      Absolute tick count at ticker stop request
 * @param fp_op_func	     Function pointer of user operation completion
 *			     function
 * @param op_context	     Context passed in operation completion call
 *
 * @return TICKER_STATUS_BUSY if stop was successful but not yet completed.
 * TICKER_STATUS_FAILURE is returned if there are no more user operations
 * available, and TICKER_STATUS_SUCCESS is returned if ticker_job gets to run
 * before exiting ticker_stop
 */
uint8_t ticker_stop_abs(uint8_t instance_index, uint8_t user_id,
			 uint8_t ticker_id, uint32_t ticks_at_stop,
			 ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	uint8_t last;

	user = &instance->users[user_id];

	last = user->last + 1;
	if (last >= user->count_user_op) {
		last = 0U;
	}

	if (last == user->first) {
		return TICKER_STATUS_FAILURE;
	}

	user_op = &user->user_op[user->last];
	user_op->op = TICKER_USER_OP_TYPE_STOP_ABS;
	user_op->id = ticker_id;
	user_op->params.yield.ticks_at_yield = ticks_at_stop;
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	/* Make sure transaction is completed before committing */
	cpu_dmb();
	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}

#if defined(CONFIG_BT_TICKER_NEXT_SLOT_GET)
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
uint8_t ticker_next_slot_get(uint8_t instance_index, uint8_t user_id,
			      uint8_t *ticker_id, uint32_t *ticks_current,
			      uint32_t *ticks_to_expire,
			      ticker_op_func fp_op_func, void *op_context)
{
#if defined(CONFIG_BT_TICKER_LAZY_GET) || \
	defined(CONFIG_BT_TICKER_REMAINDER_GET) || \
	defined(CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH)
	return ticker_next_slot_get_ext(instance_index, user_id, ticker_id,
					ticks_current, ticks_to_expire, NULL,
					NULL, NULL, NULL, fp_op_func,
					op_context);
}

uint8_t ticker_next_slot_get_ext(uint8_t instance_index, uint8_t user_id,
				  uint8_t *ticker_id, uint32_t *ticks_current,
				  uint32_t *ticks_to_expire,
				  uint32_t *remainder, uint16_t *lazy,
				  ticker_op_match_func fp_match_op_func,
				  void *match_op_context,
				  ticker_op_func fp_op_func, void *op_context)
{
#endif /* CONFIG_BT_TICKER_LAZY_GET ||
	* CONFIG_BT_TICKER_REMAINDER_GET ||
	* CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH
	*/
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	uint8_t last;

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
#if defined(CONFIG_BT_TICKER_REMAINDER_GET)
	user_op->params.slot_get.remainder = remainder;
#endif /* CONFIG_BT_TICKER_REMAINDER_GET */
#if defined(CONFIG_BT_TICKER_LAZY_GET)
	user_op->params.slot_get.lazy = lazy;
#endif /* CONFIG_BT_TICKER_LAZY_GET */
#if defined(CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH)
	user_op->params.slot_get.fp_match_op_func = fp_match_op_func;
	user_op->params.slot_get.match_op_context = match_op_context;
#endif /* CONFIG_BT_TICKER_NEXT_SLOT_GET_MATCH */
	user_op->status = TICKER_STATUS_BUSY;
	user_op->fp_op_func = fp_op_func;
	user_op->op_context = op_context;

	/* Make sure transaction is completed before committing */
	cpu_dmb();
	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}
#endif /* CONFIG_BT_TICKER_NEXT_SLOT_GET */

#if defined(CONFIG_BT_TICKER_JOB_IDLE_GET)
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
uint8_t ticker_job_idle_get(uint8_t instance_index, uint8_t user_id,
			  ticker_op_func fp_op_func, void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	uint8_t last;

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

	/* Make sure transaction is completed before committing */
	cpu_dmb();
	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}
#endif /* CONFIG_BT_TICKER_JOB_IDLE_GET */

#if !defined(CONFIG_BT_TICKER_LOW_LAT) && \
	!defined(CONFIG_BT_TICKER_SLOT_AGNOSTIC) && \
	defined(CONFIG_BT_TICKER_PRIORITY_SET)
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
uint8_t ticker_priority_set(uint8_t instance_index, uint8_t user_id, uint8_t ticker_id,
			  int8_t priority, ticker_op_func fp_op_func,
			  void *op_context)
{
	struct ticker_instance *instance = &_instance[instance_index];
	struct ticker_user_op *user_op;
	struct ticker_user *user;
	uint8_t last;

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

	/* Make sure transaction is completed before committing */
	cpu_dmb();
	user->last = last;

	instance->sched_cb(instance->caller_id_get_cb(user_id),
			   TICKER_CALL_ID_JOB, 0, instance);

	return user_op->status;
}
#endif /* !CONFIG_BT_TICKER_LOW_LAT &&
	* !CONFIG_BT_TICKER_SLOT_AGNOSTIC &&
	* CONFIG_BT_TICKER_PRIORITY_SET
	*/

/**
 * @brief Schedule ticker job
 *
 * @param instance_index Index of ticker instance
 * @param user_id	 Ticker user id. Maps to mayfly caller id
 */
void ticker_job_sched(uint8_t instance_index, uint8_t user_id)
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
uint32_t ticker_ticks_now_get(void)
{
	return cntr_cnt_get();
}

/**
 * @brief Get difference between two tick counts
 *
 * @details Subtract two counts and truncate to correct HW dependent counter
 * bit width
 *
 * @param ticks_now Highest tick count (now)
 * @param ticks_old Tick count to subtract from ticks_now
 */
uint32_t ticker_ticks_diff_get(uint32_t ticks_now, uint32_t ticks_old)
{
	return ((ticks_now - ticks_old) & HAL_TICKER_CNTR_MASK);
}
