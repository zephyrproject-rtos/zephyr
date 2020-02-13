/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/queued_operation.h>
#include <stdint.h>
#include <string.h>

/* States used in the manager state field. */
enum state {
	/* Service is not active.
	 *
	 * Transitions to STARTING on queued_operation_submit().
	 */
	ST_OFF = 0,

	/* Service is being started.
	 *
	 * This is a transient state while an associated on-off
	 * service request is incomplete.  Transitions to IDLE and
	 * reschedules on successful start, and ERROR on failure to
	 * start.
	 */
	ST_STARTING,

	/* Service is active with no known operations.
	 *
	 * This is a transient substate of an implicit ON state that
	 * encompasses IDLE, NOTIFYING, PROCESSING, and
	 * FINALIZING. The state can be observed only when the manager
	 * lock is held.  The machine will transition to NOTIFYING or
	 * STOPPING within the current mutex region.
	 */
	ST_IDLE,

	/* The manager is invoking process() to notify the service of
	 * a new operation or a transition to idle.
	 *
	 * Transitions to PROCESSING if an operation was passed and
	 * queued_operation_finalize() has not been invoked before
	 * process() returns to the manager.
	 *
	 * Transitions to IDLE if an operation was not passed and the
	 * manager queue remains empty when process() returns to the
	 * manager.
	 *
	 * Re-runs select in all other cases.
	 */
	ST_NOTIFYING,

	/* A new operation has been identified and the service
	 * process() function will be/is/has-been invoked on it.
	 *
	 * Transitions to FINALIZING when queued_operation_finalize()
	 * is invoked.
	 */
	ST_PROCESSING,

	/* An operation that was processing is being finalized.
	 *
	 * Re-selects after finalization and any containing notifying
	 * completes.
	 */
	ST_FINALIZING,

	/* Service is being started.
	 *
	 * This is a transient state while an associated on-off
	 * service request is pending.  Transitions to OFF or ERROR
	 * based on the results of a call to onoff_release().
	 */
	ST_STOPPING,

	/* Service is in an error state.
	 *
	 * This can be reached only as a consequence of a failure in
	 * the on-off service used to transition between OFF and IDLE.
	 * Transitions to RESETTING when queued_operation_reset() is
	 * invoked.
	 */
	ST_ERROR,

	/* Service is attempting to reset the underlying on-off service.
	 *
	 * Transitions to OFF when the reset attempt completes
	 * successfully, and to ERROR if the attempt fails.
	 */
	ST_RESETTING,
};

static inline bool state_is_error(uint32_t state)
{
	return (state == ST_ERROR) || (state == ST_RESETTING);
}

static void select_next_and_unlock(struct queued_operation_manager *mgr,
				   k_spinlock_key_t key);
static void start_and_unlock(struct queued_operation_manager *mgr,
			     k_spinlock_key_t key);

static inline void trivial_start_and_unlock(struct queued_operation_manager *mgr,
					    k_spinlock_key_t key)
{
	mgr->state = ST_IDLE;
	select_next_and_unlock(mgr, key);
}

static inline int op_get_priority(const struct queued_operation *op)
{
	return (int8_t)(op->notify.flags >> QUEUED_OPERATION_PRIORITY_POS);
}

static inline int op_set_priority(struct queued_operation *op,
				  int priority)
{
	int8_t prio = (int8_t)priority;
	uint32_t mask = (QUEUED_OPERATION_PRIORITY_MASK
		      << QUEUED_OPERATION_PRIORITY_POS);

	if (priority == QUEUED_OPERATION_PRIORITY_PREPEND) {
		prio = INT8_MIN;
	} else if (priority == QUEUED_OPERATION_PRIORITY_APPEND) {
		prio = INT8_MAX;
	} else if (prio != priority) {
		return -EINVAL;
	}

	op->notify.flags = (op->notify.flags & ~mask)
			   | (mask & (prio << QUEUED_OPERATION_PRIORITY_POS));

	return 0;
}

static inline void finalize_and_notify(struct queued_operation_manager *mgr,
				       struct queued_operation *op,
				       int res)
{
	sys_notify_generic_callback cb = sys_notify_finalize(&op->notify, res);

	if (cb != NULL) {
		mgr->vtable->callback(mgr, op, cb);
	}
}

/* React to the completion of an onoff transition, either from a
 * manager or directly.
 *
 * @param mgr the operation manager
 *
 * @param from either ST_STARTING or ST_STOPPING depending on
 * transition direction
 *
 * @param res the transition completion value, negative for error.
 */
static void settle_onoff(struct queued_operation_manager *mgr,
			 enum state from,
			 int res)
{
	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	__ASSERT_NO_MSG((mgr->state == from)
			|| (state_is_error(mgr->state)
			    && (ST_ERROR == from)));

	if (res >= 0) {
		if (from == ST_STARTING) {
			trivial_start_and_unlock(mgr, key);
			return;
		}

		/* Came from STOPPING or RESETTING so set to OFF.  If
		 * the underlying onoff service is in an error state
		 * that will be discovered when it is next used.
		 */
		mgr->state = ST_OFF;

		k_spin_unlock(&mgr->lock, key);
		return;
	}

	/* On transition failure mark service failed.  All unstarted
	 * operations are unlinked and completed as a service failure.
	 */
	sys_slist_t ops = mgr->operations;

	sys_slist_init(&mgr->operations);
	mgr->state = ST_ERROR;

	k_spin_unlock(&mgr->lock, key);

	struct queued_operation *op;

	SYS_SLIST_FOR_EACH_CONTAINER(&ops, op, node) {
		finalize_and_notify(mgr, op, -ENODEV);
	}
}

static void start_callback(struct onoff_manager *oomgr,
			   struct onoff_client *cli,
			   uint32_t state,
			   int res)
{
	struct queued_operation_manager *mgr
		= CONTAINER_OF(cli, struct queued_operation_manager, onoff_client);

	settle_onoff(mgr, ST_STARTING, res);
}

static void stop_and_unlock(struct queued_operation_manager *mgr,
			    k_spinlock_key_t key)
{
	__ASSERT_NO_MSG(mgr->state == ST_IDLE);

	if (mgr->onoff == NULL) {
		mgr->state = ST_OFF;
		k_spin_unlock(&mgr->lock, key);
		return;
	}

	mgr->state = ST_STOPPING;

	k_spin_unlock(&mgr->lock, key);

	settle_onoff(mgr, ST_STOPPING, onoff_release(mgr->onoff));
}

/* Enters while locked, and looks for a new operation to initiate.
 * The lock is released to process new operations, then retaken after
 * process() returns.
 */
static void select_next_and_unlock(struct queued_operation_manager *mgr,
				   k_spinlock_key_t key)
{
	bool loop = false;

	do {
		sys_snode_t *node = sys_slist_get(&mgr->operations);
		uint32_t state = mgr->state;

		__ASSERT_NO_MSG((state == ST_IDLE)
				|| (state == ST_FINALIZING));
		loop = false;
		if (node) {
			struct queued_operation *op
				= CONTAINER_OF(node, struct queued_operation, node);
			mgr->state = ST_NOTIFYING;
			mgr->current = op;

			k_spin_unlock(&mgr->lock, key);

			/* Notify the service, then check everything again
			 * because the operation might have completed or the
			 * queue might have changed while we were unlocked.
			 */
			mgr->vtable->process(mgr, op);

			/* Update the state to one of IDLE, PROCESSING, or
			 * leave it FINALIZING; loop if something needs to be
			 * done.
			 */
			key = k_spin_lock(&mgr->lock);

			state = mgr->state;

			/* If an operation finalized during notification we
			 * need to reselect because finalization couldn't do
			 * that, otherwise it's still running.
			 */
			loop = (state == ST_FINALIZING);
			if (!loop) {
				__ASSERT_NO_MSG(state == ST_NOTIFYING);
				mgr->state = ST_PROCESSING;
			}
		} else {
			__ASSERT_NO_MSG(state == ST_FINALIZING);
			mgr->state = ST_IDLE;
			mgr->current = NULL;
		}
	} while (loop);

	/* All done, transition to STOPPING if IDLE, and release the
	 * lock.
	 */
	if (mgr->state == ST_IDLE) {
		stop_and_unlock(mgr, key);
	} else {
		k_spin_unlock(&mgr->lock, key);
	}
}

static void start_and_unlock(struct queued_operation_manager *mgr,
			     k_spinlock_key_t key)
{
	struct onoff_manager *onoff = mgr->onoff;
	struct onoff_client *cli = &mgr->onoff_client;
	int rv = 0;

	if (onoff == NULL) {
		trivial_start_and_unlock(mgr, key);
		return;
	}

	mgr->state = ST_STARTING;
	k_spin_unlock(&mgr->lock, key);

	memset(cli, 0, sizeof(*cli));
	sys_notify_init_callback(&cli->notify, start_callback);

	rv = onoff_request(onoff, cli);

	if (rv < 0) {
		/* Synchronous failure, record the error state */
		settle_onoff(mgr, ST_STARTING, rv);
	}
}

int queued_operation_submit(struct queued_operation_manager *mgr,
			    struct queued_operation *op,
			    int priority)
{
	int validate_rv = -ENOTSUP;
	int rv = 0;

	__ASSERT_NO_MSG(mgr != NULL);
	__ASSERT_NO_MSG(mgr->vtable != NULL);
	__ASSERT_NO_MSG(mgr->vtable->process != NULL);
	__ASSERT_NO_MSG(op != NULL);

	/* Validation is optional; if present, use it. */
	if (mgr->vtable->validate) {
		validate_rv = mgr->vtable->validate(mgr, op);
		rv = validate_rv;
	}

	/* Set the priority, checking whether it's in range. */
	if (rv >= 0) {
		rv = op_set_priority(op, priority);
	}

	/* Reject callback notifications without translation
	 * function.
	 */
	if ((rv >= 0)
	    && sys_notify_uses_callback(&op->notify)
	    && (mgr->vtable->callback == NULL)) {
		rv = -ENOTSUP;
	}

	if (rv < 0) {
		goto out;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	sys_slist_t *list = &mgr->operations;
	uint32_t state = mgr->state;

	/* Preserve error state, or insert the item into the list. */
	if (state_is_error(state)) {
		rv = -ENODEV;
	} else if (priority == QUEUED_OPERATION_PRIORITY_PREPEND) {
		sys_slist_prepend(list, &op->node);
	} else if (priority == QUEUED_OPERATION_PRIORITY_APPEND) {
		sys_slist_append(list, &op->node);
	} else {
		struct queued_operation *prev = NULL;
		struct queued_operation *tmp;

		SYS_SLIST_FOR_EACH_CONTAINER(list, tmp, node) {
			if (priority < op_get_priority(tmp)) {
				break;
			}
			prev = tmp;
		}

		if (prev == NULL) {
			sys_slist_prepend(list, &op->node);
		} else {
			sys_slist_insert(list, &prev->node, &op->node);
		}
	}

	/* Initiate an operation only if we're off. */
	if (state == ST_OFF) {
		start_and_unlock(mgr, key);
	} else {
		__ASSERT_NO_MSG(state != ST_IDLE);
		k_spin_unlock(&mgr->lock, key);
	}

out:
	/* Preserve a service-specific success code on success */
	if ((rv >= 0) && (validate_rv >= 0)) {
		rv = validate_rv;
	}

	return rv;
}

void queued_operation_finalize(struct queued_operation_manager *mgr,
			       int res)
{
	__ASSERT_NO_MSG(mgr != NULL);

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	struct queued_operation *op = mgr->current;
	uint32_t state = mgr->state;
	bool processing = (state == ST_PROCESSING);

	__ASSERT_NO_MSG(op != NULL);
	__ASSERT_NO_MSG((state == ST_NOTIFYING)
			|| (state == ST_PROCESSING));

	mgr->state = ST_FINALIZING;

	k_spin_unlock(&mgr->lock, key);

	finalize_and_notify(mgr, op, res);

	/* If we were processing we need to reselect; if we were
	 * notifying we'll reselect when the notification completes.
	 */
	if (processing) {
		select_next_and_unlock(mgr, k_spin_lock(&mgr->lock));
	}
}

int queued_operation_cancel(struct queued_operation_manager *mgr,
			    struct queued_operation *op)
{
	__ASSERT_NO_MSG(mgr != NULL);
	__ASSERT_NO_MSG(op != NULL);

	int rv = 0;
	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	if (op == mgr->current) {
		rv = -EINPROGRESS;
	} else if (!sys_slist_find_and_remove(&mgr->operations, &op->node)) {
		rv = -EINVAL;
	}

	k_spin_unlock(&mgr->lock, key);

	if (rv == 0) {
		finalize_and_notify(mgr, op, -ECANCELED);
	}

	return rv;
}

int queued_operation_sync_submit(struct queued_operation_manager *qop_mgr,
			struct queued_operation *qop, int priority)
{
	int err;
	int ret;
	struct k_poll_signal signal = K_POLL_SIGNAL_INITIALIZER(signal);
	struct k_poll_event event = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
							K_POLL_MODE_NOTIFY_ONLY,
							&signal);

	sys_notify_init_signal(&qop->notify, &signal);
	err = queued_operation_submit(qop_mgr, qop, priority);
	if (err < 0) {
		return err;
	}

	err = k_poll(&event, 1, K_FOREVER);
	if (err < 0) {
		return err;
	}

	err = sys_notify_fetch_result(&qop->notify, &ret);
	if (err < 0) {
		return err;
	}

	return ret;
}

bool queued_operation_has_error(struct queued_operation_manager *mgr)
{
	__ASSERT_NO_MSG(mgr != NULL);

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	bool rv = state_is_error(mgr->state);

	k_spin_unlock(&mgr->lock, key);

	return rv;
}

static void reset_callback(struct onoff_manager *oomgr,
			   struct onoff_client *cli,
			   uint32_t state,
			   int res)
{
	struct queued_operation_manager *mgr
		= CONTAINER_OF(cli, struct queued_operation_manager, onoff_client);

	settle_onoff(mgr, ST_ERROR, res);
}

int queued_operation_reset(struct queued_operation_manager *mgr,
			   struct onoff_client *oocli)
{
	__ASSERT_NO_MSG(mgr != NULL);

	int rv = -ENOTSUP;

	if (mgr->onoff == NULL) {
		goto out;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	uint32_t state = mgr->state;

	if (!state_is_error(state)) {
		rv = -EALREADY;
		goto out_locked;
	}

	if (state == ST_ERROR) {
		struct onoff_client *cli = &mgr->onoff_client;

		memset(cli, 0, sizeof(*cli));
		sys_notify_init_callback(&cli->notify, reset_callback);

		mgr->state = ST_RESETTING;

		k_spin_unlock(&mgr->lock, key);

		rv = onoff_reset(mgr->onoff, cli);

		key = k_spin_lock(&mgr->lock);

		if (rv < 0) {
			mgr->state = ST_ERROR;
			goto out_locked;
		}
	}

out_locked:
	k_spin_unlock(&mgr->lock, key);

out:
	if ((rv >= 0) && (oocli != NULL)) {
		rv = onoff_reset(mgr->onoff, oocli);
	}

	return rv;
}
