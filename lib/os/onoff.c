/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/onoff.h>
#include <stdio.h>

#define SERVICE_REFS_MAX UINT16_MAX

/* Confirm consistency of public flags with private flags */
BUILD_ASSERT((ONOFF_FLAG_ERROR | ONOFF_FLAG_ONOFF | ONOFF_FLAG_TRANSITION)
	     < BIT(3));

#define ONOFF_FLAG_PROCESSING BIT(3)
#define ONOFF_FLAG_COMPLETE BIT(4)
#define ONOFF_FLAG_RECHECK BIT(5)

/* These symbols in the ONOFF_FLAGS namespace identify bits in
 * onoff_manager::flags that indicate the state of the machine.  The
 * bits are manipulated by process_event() under lock, and actions
 * cued by bit values are executed outside of lock within
 * process_event().
 *
 * * ERROR indicates that the machine is in an error state.  When
 *   this bit is set ONOFF will be cleared.
 * * ONOFF indicates whether the target/current state is off (clear)
 *   or on (set).
 * * TRANSITION indicates whether a service transition function is in
 *   progress.  It combines with ONOFF to identify start and stop
 *   transitions, and with ERROR to identify a reset transition.
 * * PROCESSING indicates that the process_event() loop is active.  It
 *   is used to defer initiation of transitions and other complex
 *   state changes while invoking notifications associated with a
 *   state transition.  This bounds the  depth by limiting
 *   active process_event() call stacks to two instances.  State changes
 *   initiated by a nested call will be executed when control returns
 *   to the parent call.
 * * COMPLETE indicates that a transition completion notification has
 *   been received.  This flag is set in the notification, and cleared
 *   by process_events() which is invoked from the notification.  In
 *   the case of nested process_events() the processing is deferred to
 *   the top invocation.
 * * RECHECK indicates that a state transition has completed but
 *   process_events() must re-check the overall state to confirm no
 *   additional transitions are required.  This is used to simplify the
 *   logic when, for example, a request is received during a
 *   transition to off, which means that when the transition completes
 *   a transition to on must be initiated if the request is still
 *   present.  Transition to ON with no remaining requests similarly
 *   triggers a recheck.
 */

/* Identify the events that can trigger state changes, as well as an
 * internal state used when processing deferred actions.
 */
enum event_type {
	/* No-op event: used to process deferred changes.
	 *
	 * This event is local to the process loop.
	 */
	EVT_NOP,

	/* Completion of a service transition.
	 *
	 * This event is triggered by the transition notify callback.
	 * It can be received only when the machine is in a transition
	 * state (TO-ON, TO-OFF, or RESETTING).
	 */
	EVT_COMPLETE,

	/* Reassess whether a transition from a stable state is needed.
	 *
	 * This event causes:
	 * * a start from OFF when there are clients;
	 * * a stop from ON when there are no clients;
	 * * a reset from ERROR when there are clients.
	 *
	 * The client list can change while the manager lock is
	 * released (e.g. during client and monitor notifications and
	 * transition initiations), so this event records the
	 * potential for these state changes, and process_event() ...
	 *
	 */
	EVT_RECHECK,

	/* Transition to on.
	 *
	 * This is synthesized from EVT_RECHECK in a non-nested
	 * process_event() when state OFF is confirmed with a
	 * non-empty client (request) list.
	 */
	EVT_START,

	/* Transition to off.
	 *
	 * This is synthesized from EVT_RECHECK in a non-nested
	 * process_event() when state ON is confirmed with a
	 * zero reference count.
	 */
	EVT_STOP,

	/* Transition to resetting.
	 *
	 * This is synthesized from EVT_RECHECK in a non-nested
	 * process_event() when state ERROR is confirmed with a
	 * non-empty client (reset) list.
	 */
	EVT_RESET,
};

static void set_state(struct onoff_manager *mgr,
		      uint32_t state)
{
	mgr->flags = (state & ONOFF_STATE_MASK)
		     | (mgr->flags & ~ONOFF_STATE_MASK);
}

static int validate_args(const struct onoff_manager *mgr,
			 struct onoff_client *cli)
{
	if ((mgr == NULL) || (cli == NULL)) {
		return -EINVAL;
	}

	int rv = sys_notify_validate(&cli->notify);

	if ((rv == 0)
	    && ((cli->notify.flags
		 & ~BIT_MASK(ONOFF_CLIENT_EXTENSION_POS)) != 0)) {
		rv = -EINVAL;
	}

	return rv;
}

int onoff_manager_init(struct onoff_manager *mgr,
		       const struct onoff_transitions *transitions)
{
	if ((mgr == NULL)
	    || (transitions == NULL)
	    || (transitions->start == NULL)
	    || (transitions->stop == NULL)) {
		return -EINVAL;
	}

	*mgr = (struct onoff_manager)ONOFF_MANAGER_INITIALIZER(transitions);

	return 0;
}

static void notify_monitors(struct onoff_manager *mgr,
			    uint32_t state,
			    int res)
{
	sys_slist_t *mlist = &mgr->monitors;
	struct onoff_monitor *mon;
	struct onoff_monitor *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(mlist, mon, tmp, node) {
		mon->callback(mgr, mon, state, res);
	}
}

static void notify_one(struct onoff_manager *mgr,
		       struct onoff_client *cli,
		       uint32_t state,
		       int res)
{
	onoff_client_callback cb =
		(onoff_client_callback)sys_notify_finalize(&cli->notify, res);

	if (cb) {
		cb(mgr, cli, state, res);
	}
}

static void notify_all(struct onoff_manager *mgr,
		       sys_slist_t *list,
		       uint32_t state,
		       int res)
{
	while (!sys_slist_is_empty(list)) {
		sys_snode_t *node = sys_slist_get_not_empty(list);
		struct onoff_client *cli =
			CONTAINER_OF(node,
				     struct onoff_client,
				     node);

		notify_one(mgr, cli, state, res);
	}
}

static void process_event(struct onoff_manager *mgr,
			  int evt,
			  k_spinlock_key_t key);

static void transition_complete(struct onoff_manager *mgr,
				int res)
{
	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	mgr->last_res = res;
	process_event(mgr, EVT_COMPLETE, key);
}

/* Detect whether static state requires a transition. */
static int process_recheck(struct onoff_manager *mgr)
{
	int evt = EVT_NOP;
	uint32_t state = mgr->flags & ONOFF_STATE_MASK;

	if ((state == ONOFF_STATE_OFF)
	    && !sys_slist_is_empty(&mgr->clients)) {
		evt = EVT_START;
	} else if ((state == ONOFF_STATE_ON)
		   && (mgr->refs == 0U)) {
		evt = EVT_STOP;
	} else if ((state == ONOFF_STATE_ERROR)
		   && !sys_slist_is_empty(&mgr->clients)) {
		evt = EVT_RESET;
	} else {
		;
	}

	return evt;
}

/* Process a transition completion.
 *
 * If the completion requires notifying clients, the clients are moved
 * from the manager to the output list for notification.
 */
static void process_complete(struct onoff_manager *mgr,
			     sys_slist_t *clients,
			     int res)
{
	uint32_t state = mgr->flags & ONOFF_STATE_MASK;

	if (res < 0) {
		/* Enter ERROR state and notify all clients. */
		*clients = mgr->clients;
		sys_slist_init(&mgr->clients);
		set_state(mgr, ONOFF_STATE_ERROR);
	} else if ((state == ONOFF_STATE_TO_ON)
		   || (state == ONOFF_STATE_RESETTING)) {
		*clients = mgr->clients;
		sys_slist_init(&mgr->clients);

		if (state == ONOFF_STATE_TO_ON) {
			struct onoff_client *cp;

			/* Increment reference count for all remaining
			 * clients and enter ON state.
			 */
			SYS_SLIST_FOR_EACH_CONTAINER(clients, cp, node) {
				mgr->refs += 1U;
			}

			set_state(mgr, ONOFF_STATE_ON);
		} else {
			__ASSERT_NO_MSG(state == ONOFF_STATE_RESETTING);

			set_state(mgr, ONOFF_STATE_OFF);
		}
		if (process_recheck(mgr) != EVT_NOP) {
			mgr->flags |= ONOFF_FLAG_RECHECK;
		}
	} else if (state == ONOFF_STATE_TO_OFF) {
		/* Any active clients are requests waiting for this
		 * transition to complete.  Queue a RECHECK event to
		 * ensure we don't miss them if we don't unlock to
		 * tell anybody about the completion.
		 */
		set_state(mgr, ONOFF_STATE_OFF);
		if (process_recheck(mgr) != EVT_NOP) {
			mgr->flags |= ONOFF_FLAG_RECHECK;
		}
	} else {
		__ASSERT_NO_MSG(false);
	}
}

/* There are two points in the state machine where the machine is
 * unlocked to perform some external action:
 * * Initiation of an transition due to some event;
 * * Invocation of the user-specified callback when a stable state is
 *   reached or an error detected.
 *
 * Events received during these unlocked periods are recorded in the
 * state, but processing is deferred to the top-level invocation which
 * will loop to handle any events that occurred during the unlocked
 * regions.
 */
static void process_event(struct onoff_manager *mgr,
			  int evt,
			  k_spinlock_key_t key)
{
	sys_slist_t clients;
	uint32_t state = mgr->flags & ONOFF_STATE_MASK;
	int res = 0;
	bool processing = ((mgr->flags & ONOFF_FLAG_PROCESSING) != 0);

	__ASSERT_NO_MSG(evt != EVT_NOP);

	/* If this is a nested call record the event for processing in
	 * the top invocation.
	 */
	if (processing) {
		if (evt == EVT_COMPLETE) {
			mgr->flags |= ONOFF_FLAG_COMPLETE;
		} else {
			__ASSERT_NO_MSG(evt == EVT_RECHECK);

			mgr->flags |= ONOFF_FLAG_RECHECK;
		}

		goto out;
	}

	sys_slist_init(&clients);
	do {
		onoff_transition_fn transit = NULL;

		if (evt == EVT_RECHECK) {
			evt = process_recheck(mgr);
		}

		if (evt == EVT_NOP) {
			break;
		}

		res = 0;
		if (evt == EVT_COMPLETE) {
			res = mgr->last_res;
			process_complete(mgr, &clients, res);
			/* NB: This can trigger a RECHECK */
		} else if (evt == EVT_START) {
			__ASSERT_NO_MSG(state == ONOFF_STATE_OFF);
			__ASSERT_NO_MSG(!sys_slist_is_empty(&mgr->clients));

			transit = mgr->transitions->start;
			__ASSERT_NO_MSG(transit != NULL);
			set_state(mgr, ONOFF_STATE_TO_ON);
		} else if (evt == EVT_STOP) {
			__ASSERT_NO_MSG(state == ONOFF_STATE_ON);
			__ASSERT_NO_MSG(mgr->refs == 0);

			transit = mgr->transitions->stop;
			__ASSERT_NO_MSG(transit != NULL);
			set_state(mgr, ONOFF_STATE_TO_OFF);
		} else if (evt == EVT_RESET) {
			__ASSERT_NO_MSG(state == ONOFF_STATE_ERROR);
			__ASSERT_NO_MSG(!sys_slist_is_empty(&mgr->clients));

			transit = mgr->transitions->reset;
			__ASSERT_NO_MSG(transit != NULL);
			set_state(mgr, ONOFF_STATE_RESETTING);
		} else {
			__ASSERT_NO_MSG(false);
		}

		/* Have to unlock and do something if any of:
		 * * We changed state and there are monitors;
		 * * We completed a transition and there are clients to notify;
		 * * We need to initiate a transition.
		 */
		bool do_monitors = (state != (mgr->flags & ONOFF_STATE_MASK))
				   && !sys_slist_is_empty(&mgr->monitors);

		evt = EVT_NOP;
		if (do_monitors
		    || !sys_slist_is_empty(&clients)
		    || (transit != NULL)) {
			uint32_t flags = mgr->flags | ONOFF_FLAG_PROCESSING;

			mgr->flags = flags;
			state = flags & ONOFF_STATE_MASK;

			k_spin_unlock(&mgr->lock, key);

			if (do_monitors) {
				notify_monitors(mgr, state, res);
			}

			if (!sys_slist_is_empty(&clients)) {
				notify_all(mgr, &clients, state, res);
			}

			if (transit != NULL) {
				transit(mgr, transition_complete);
			}

			key = k_spin_lock(&mgr->lock);
			mgr->flags &= ~ONOFF_FLAG_PROCESSING;
		}

		/* Process deferred events.  Completion takes priority
		 * over recheck.
		 */
		if ((mgr->flags & ONOFF_FLAG_COMPLETE) != 0) {
			mgr->flags &= ~ONOFF_FLAG_COMPLETE;
			evt = EVT_COMPLETE;
		} else if ((mgr->flags & ONOFF_FLAG_RECHECK) != 0) {
			mgr->flags &= ~ONOFF_FLAG_RECHECK;
			evt = EVT_RECHECK;
		} else {
			;
		}

		state = mgr->flags & ONOFF_STATE_MASK;
	} while (evt != EVT_NOP);

out:
	k_spin_unlock(&mgr->lock, key);
}

int onoff_request(struct onoff_manager *mgr,
		  struct onoff_client *cli)
{
	bool add_client = false;        /* add client to pending list */
	bool start = false;             /* trigger a start transition */
	bool notify = false;            /* do client notification */
	int rv = validate_args(mgr, cli);

	if (rv < 0) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	uint32_t state = mgr->flags & ONOFF_STATE_MASK;

	/* Reject if this would overflow the reference count. */
	if (mgr->refs == SERVICE_REFS_MAX) {
		rv = -EAGAIN;
		goto out;
	}

	rv = state;
	if (state == ONOFF_STATE_ON) {
		/* Increment reference count, notify in exit */
		notify = true;
		mgr->refs += 1U;
	} else if ((state == ONOFF_STATE_OFF)
		   || (state == ONOFF_STATE_TO_OFF)
		   || (state == ONOFF_STATE_TO_ON)) {
		/* Start if OFF, queue client */
		start = (state == ONOFF_STATE_OFF);
		add_client = true;
	} else if (state == ONOFF_STATE_RESETTING) {
		rv = -ENOTSUP;
	} else {
		__ASSERT_NO_MSG(state == ONOFF_STATE_ERROR);
		rv = -EIO;
	}

out:
	if (add_client) {
		sys_slist_append(&mgr->clients, &cli->node);
	}

	if (start) {
		process_event(mgr, EVT_RECHECK, key);
	} else {
		k_spin_unlock(&mgr->lock, key);

		if (notify) {
			notify_one(mgr, cli, state, 0);
		}
	}

	return rv;
}

int onoff_release(struct onoff_manager *mgr)
{
	bool stop = false;      /* trigger a stop transition */

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	uint32_t state = mgr->flags & ONOFF_STATE_MASK;
	int rv = state;

	if (state != ONOFF_STATE_ON) {
		if (state == ONOFF_STATE_ERROR) {
			rv = -EIO;
		} else {
			rv = -ENOTSUP;
		}
		goto out;
	}

	__ASSERT_NO_MSG(mgr->refs > 0);
	mgr->refs -= 1U;
	stop = (mgr->refs == 0);

out:
	if (stop) {
		process_event(mgr, EVT_RECHECK, key);
	} else {
		k_spin_unlock(&mgr->lock, key);
	}

	return rv;
}

int onoff_reset(struct onoff_manager *mgr,
		struct onoff_client *cli)
{
	bool reset = false;
	int rv = validate_args(mgr, cli);

	if ((rv >= 0)
	    && (mgr->transitions->reset == NULL)) {
		rv = -ENOTSUP;
	}

	if (rv < 0) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	uint32_t state = mgr->flags & ONOFF_STATE_MASK;

	rv = state;

	if ((state & ONOFF_FLAG_ERROR) == 0) {
		rv = -EALREADY;
	} else {
		reset = (state != ONOFF_STATE_RESETTING);
		sys_slist_append(&mgr->clients, &cli->node);
	}

	if (reset) {
		process_event(mgr, EVT_RECHECK, key);
	} else {
		k_spin_unlock(&mgr->lock, key);
	}

	return rv;
}

int onoff_cancel(struct onoff_manager *mgr,
		 struct onoff_client *cli)
{
	if ((mgr == NULL) || (cli == NULL)) {
		return -EINVAL;
	}

	int rv = -EALREADY;
	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	uint32_t state = mgr->flags & ONOFF_STATE_MASK;

	if (sys_slist_find_and_remove(&mgr->clients, &cli->node)) {
		__ASSERT_NO_MSG((state == ONOFF_STATE_TO_ON)
				|| (state == ONOFF_STATE_TO_OFF)
				|| (state == ONOFF_STATE_RESETTING));
		rv = state;
	}

	k_spin_unlock(&mgr->lock, key);

	return rv;
}

int onoff_monitor_register(struct onoff_manager *mgr,
			   struct onoff_monitor *mon)
{
	if ((mgr == NULL)
	    || (mon == NULL)
	    || (mon->callback == NULL)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	sys_slist_append(&mgr->monitors, &mon->node);

	k_spin_unlock(&mgr->lock, key);

	return 0;
}

int onoff_monitor_unregister(struct onoff_manager *mgr,
			     struct onoff_monitor *mon)
{
	int rv = -EINVAL;

	if ((mgr == NULL)
	    || (mon == NULL)) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	if (sys_slist_find_and_remove(&mgr->monitors, &mon->node)) {
		rv = 0;
	}

	k_spin_unlock(&mgr->lock, key);

	return rv;
}

int onoff_sync_lock(struct onoff_sync_service *srv,
		    k_spinlock_key_t *keyp)
{
	*keyp = k_spin_lock(&srv->lock);
	return srv->count;
}

int onoff_sync_finalize(struct onoff_sync_service *srv,
			k_spinlock_key_t key,
			struct onoff_client *cli,
			int res,
			bool on)
{
	uint32_t state = ONOFF_STATE_ON;

	/* Clear errors visible when locked.  If they are to be
	 * preserved the caller must finalize with the previous
	 * error code.
	 */
	if (srv->count < 0) {
		srv->count = 0;
	}
	if (res < 0) {
		srv->count = res;
		state = ONOFF_STATE_ERROR;
	} else if (on) {
		srv->count += 1;
	} else {
		srv->count -= 1;
		/* state would be either off or on, but since
		 * callbacks are used only when turning on don't
		 * bother changing it.
		 */
	}

	int rv = srv->count;

	k_spin_unlock(&srv->lock, key);

	if (cli) {
		/* Detect service mis-use: onoff does not callback on transition
		 * to off, so no client should have been passed.
		 */
		__ASSERT_NO_MSG(on);
		notify_one(NULL, cli, state, res);
	}

	return rv;
}
