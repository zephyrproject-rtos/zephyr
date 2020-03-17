/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/onoff.h>

#define SERVICE_CONFIG_FLAGS \
	(ONOFF_START_SLEEPS  \
	 | ONOFF_STOP_SLEEPS \
	 | ONOFF_RESET_SLEEPS)

#define SERVICE_REFS_MAX UINT16_MAX

#define ONOFF_CLIENT_INACTIVE 0
#define ONOFF_CLIENT_REQUEST 1
#define ONOFF_CLIENT_RELEASE 2
#define ONOFF_CLIENT_RESET 3

#define ONOFF_CLIENT_TYPE_MASK (BIT_MASK(ONOFF_CLIENT_TYPE_BITS) << ONOFF_CLIENT_TYPE_POS)

static u32_t get_client_type(const struct onoff_client *cli)
{
	return (cli->notify.flags >> ONOFF_CLIENT_TYPE_POS)
	       & BIT_MASK(ONOFF_CLIENT_TYPE_BITS);
}

static void set_client_type(struct onoff_client *cli,
			    u32_t type)
{
	u32_t flags = cli->notify.flags;

	flags &= ~ONOFF_CLIENT_TYPE_MASK;
	flags |= (type << ONOFF_CLIENT_TYPE_POS) & ONOFF_CLIENT_TYPE_MASK;
	cli->notify.flags = flags;
}

static void set_service_state(struct onoff_manager *mgr,
			      u32_t state)
{
	mgr->flags &= ~ONOFF_STATE_MASK;
	mgr->flags |= (state & ONOFF_STATE_MASK);
}

static int validate_args(const struct onoff_manager *mgr,
			 struct onoff_client *cli)
{
	if ((mgr == NULL) || (cli == NULL)) {
		return -EINVAL;
	}

	int rv = async_notify_validate(&cli->notify);

	if ((rv == 0)
	    && ((cli->notify.flags & ~BIT_MASK(ONOFF_CLIENT_EXTENSION_POS)) != 0)) {
		rv = -EINVAL;
	}

	return rv;
}

int onoff_manager_init(struct onoff_manager *mgr,
		       const struct onoff_transitions *transitions)
{
	if (transitions->flags & ~SERVICE_CONFIG_FLAGS) {
		return -EINVAL;
	}

	if ((transitions->start == NULL) || (transitions->stop == NULL)) {
		return -EINVAL;
	}

	*mgr = (struct onoff_manager)ONOFF_MANAGER_INITIALIZER(transitions);

	return 0;
}

static void notify_monitors(struct onoff_manager *mgr,
			    u32_t state,
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
		       u32_t state,
		       int res)
{
	set_client_type(cli, ONOFF_CLIENT_INACTIVE);

	onoff_client_callback cb =
		(onoff_client_callback)async_notify_finalize(&cli->notify, res);

	if (cb) {
		cb(mgr, cli, state, res, cli->user_data);
	}
}

static void notify_all(struct onoff_manager *mgr,
		       sys_slist_t *list,
		       u32_t state,
		       int res)
{
	notify_monitors(mgr, state, res);

	while (!sys_slist_is_empty(list)) {
		sys_snode_t *node = sys_slist_get_not_empty(list);
		struct onoff_client *cli =
			CONTAINER_OF(node,
				     struct onoff_client,
				     node);

		notify_one(mgr, cli, state, res);
	}
}

static void onoff_stop_notify(struct onoff_manager *mgr,
			      int res);

static void onoff_start_notify(struct onoff_manager *mgr,
			       int res)
{
	bool stop = false;
	unsigned int refs = 0U;
	sys_snode_t *node;
	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	sys_slist_t clients = mgr->clients;

	/* All clients registered at the time a start completes must
	 * be request clients.  Accrue a reference count for each of
	 * them.
	 */
	SYS_SLIST_FOR_EACH_NODE(&clients, node) {
		struct onoff_client *cli = CONTAINER_OF(node, struct onoff_client, node);

		(void)cli;
		__ASSERT_NO_MSG(get_client_type(cli) == ONOFF_CLIENT_REQUEST);
		refs += 1U;
	}

	/* If the start failed log an error and leave the rest of the
	 * state in place for diagnostics.
	 *
	 * If the start succeeded record a reference for all clients
	 * and set the state to ON.  There must be at least one client
	 * left to receive the result.
	 *
	 * In either case reset the client queue and notify all
	 * clients of operation completion.
	 */
	if (res < 0) {
		mgr->flags &= ~ONOFF_FLAG_TRANSITION;
		mgr->flags |= ONOFF_HAS_ERROR;
	} else {
		set_service_state(mgr, ONOFF_STATE_ON);

		/* Update the reference count, or fail if the count
		 * would overflow.
		 */
		if (mgr->refs > (SERVICE_REFS_MAX - refs)) {
			mgr->flags |= ONOFF_HAS_ERROR;
		} else {
			mgr->refs += refs;
		}

		stop = (mgr->refs == 0);
		if (stop
		    && (k_is_in_isr() || k_is_pre_kernel())
		    && ((mgr->flags & ONOFF_STOP_SLEEPS) != 0U)) {
			mgr->flags |= ONOFF_HAS_ERROR;
			stop = false;
		}
	}

	sys_slist_init(&mgr->clients);

	u32_t state = mgr->flags & ONOFF_STATE_MASK;

	k_spin_unlock(&mgr->lock, key);

	notify_all(mgr, &clients, state, res);
	if (stop) {
		__ASSERT_NO_MSG(mgr->transitions->stop != NULL);
		mgr->transitions->stop(mgr, onoff_stop_notify);
	}
}

int onoff_request(struct onoff_manager *mgr,
		  struct onoff_client *cli)
{
	bool add_client = false;        /* add client to pending list */
	bool start = false;             /* invoke start transition */
	bool notify = false;            /* do client notification */
	int rv = validate_args(mgr, cli);

	if (rv < 0) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	if (get_client_type(cli) != ONOFF_CLIENT_INACTIVE) {
		rv = -EINVAL;
		goto out;
	}

	if ((mgr->flags & ONOFF_HAS_ERROR) != 0) {
		rv = -EIO;
		goto out;
	}

	/* Reject if this would overflow the reference count. */
	if (mgr->refs == SERVICE_REFS_MAX) {
		rv = -EAGAIN;
		goto out;
	}

	u32_t state = mgr->flags & ONOFF_STATE_MASK;

	switch (state) {
	case ONOFF_STATE_TO_OFF:
		/* Queue to start after release */
		add_client = true;
		rv = 3;
		break;
	case ONOFF_STATE_OFF:
		/* Reject if in a non-thread context and start could
		 * wait.
		 */
		if ((k_is_in_isr() || k_is_pre_kernel())
		    && ((mgr->flags & ONOFF_START_SLEEPS) != 0U)) {
			rv = -EWOULDBLOCK;
			break;
		}

		/* Start with first request while off */
		__ASSERT_NO_MSG(mgr->refs == 0);
		set_service_state(mgr, ONOFF_STATE_TO_ON);
		start = true;
		add_client = true;
		rv = 2;
		break;
	case ONOFF_STATE_TO_ON:
		/* Already starting, just queue it */
		add_client = true;
		rv = 1;
		break;
	case ONOFF_STATE_ON:
		/* Just increment the reference count */
		notify = true;
		break;
	default:
		rv = -EINVAL;
		break;
	}

out:
	if (add_client) {
		set_client_type(cli, ONOFF_CLIENT_REQUEST);
		sys_slist_append(&mgr->clients, &cli->node);
	} else if (notify) {
		mgr->refs += 1;
	}

	state = mgr->flags & ONOFF_STATE_MASK;
	k_spin_unlock(&mgr->lock, key);

	if (start) {
		__ASSERT_NO_MSG(mgr->transitions->start != NULL);
		notify_monitors(mgr, state, 0);
		mgr->transitions->start(mgr, onoff_start_notify);
	} else if (notify) {
		notify_one(mgr, cli, state, 0);
	}

	return rv;
}

static void onoff_stop_notify(struct onoff_manager *mgr,
			      int res)
{
	bool fail_restart = false;
	bool start = false;
	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	sys_slist_t req_clients;
	sys_slist_t rel_clients;

	/* Separate any remaining clients into request clients and
	 * release clients, decrementing the reference count for the
	 * release clients.
	 */
	sys_slist_init(&req_clients);
	sys_slist_init(&rel_clients);
	while (!sys_slist_is_empty(&mgr->clients)) {
		sys_snode_t *node = sys_slist_get_not_empty(&mgr->clients);
		struct onoff_client *cli = CONTAINER_OF(node, struct onoff_client, node);

		if (get_client_type(cli) == ONOFF_CLIENT_RELEASE) {
			sys_slist_append(&rel_clients, node);
			mgr->refs -= 1U;
		} else {
			__ASSERT_NO_MSG(get_client_type(cli) == ONOFF_CLIENT_REQUEST);
			sys_slist_append(&req_clients, node);
		}
	}

	__ASSERT_NO_MSG(mgr->refs == 0);

	/* If the stop operation failed log an error, leave the
	 * rest of the state in place, and notify all clients.
	 *
	 * If the stop succeeded mark the service as off.  If there
	 * are request clients then synthesize a transition to on
	 * (which fails if the start cannot be initiated).
	 */
	if (res < 0) {
		mgr->flags &= ~ONOFF_FLAG_TRANSITION;
		mgr->flags |= ONOFF_HAS_ERROR;
		sys_slist_merge_slist(&rel_clients, &req_clients);
	} else {
		set_service_state(mgr, ONOFF_STATE_OFF);

		if (!sys_slist_is_empty(&req_clients)) {

			/* We need to restart, which requires that we
			 * be able to start.
			 */
			fail_restart = ((k_is_in_isr() || k_is_pre_kernel())
					&& ((mgr->flags & ONOFF_START_SLEEPS) != 0U));
			if (!fail_restart) {
				mgr->clients = req_clients;
				set_service_state(mgr, ONOFF_STATE_TO_ON);
				start = true;
			}
		}
	}

	u32_t state = mgr->flags & ONOFF_STATE_MASK;

	k_spin_unlock(&mgr->lock, key);

	/* Notify all the release clients of the result */
	notify_all(mgr, &rel_clients, state, res);

	/* Handle synthesized start (failure or initiate) */
	if (fail_restart) {
		notify_all(mgr, &req_clients, state, -EWOULDBLOCK);
	} else if (start) {
		mgr->transitions->start(mgr, onoff_start_notify);
	}
}

int onoff_release(struct onoff_manager *mgr,
		  struct onoff_client *cli)
{
	bool stop = false;      /* invoke stop transition */
	bool notify = false;    /* do client notification */
	int rv = validate_args(mgr, cli);

	if (rv < 0) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	if (get_client_type(cli) != ONOFF_CLIENT_INACTIVE) {
		rv = -EINVAL;
		goto out;
	}

	if ((mgr->flags & ONOFF_HAS_ERROR) != 0) {
		rv = -EIO;
		goto out;
	}

	u32_t state = mgr->flags & ONOFF_STATE_MASK;

	switch (state) {
	case ONOFF_STATE_ON:
		/* Stay on if release leaves a client. */
		if (mgr->refs > 1U) {
			notify = true;
			rv = 1;
			break;
		}

		/* Reject if in non-thread context but stop could
		 * wait
		 */
		if ((k_is_in_isr() || k_is_pre_kernel())
		    && ((mgr->flags & ONOFF_STOP_SLEEPS) != 0)) {
			rv = -EWOULDBLOCK;
			break;
		}

		stop = true;

		set_service_state(mgr, ONOFF_STATE_TO_OFF);
		set_client_type(cli, ONOFF_CLIENT_RELEASE);
		sys_slist_append(&mgr->clients, &cli->node);
		rv = 2;

		break;
	case ONOFF_STATE_TO_ON:
		rv = -EBUSY;
		break;
	case ONOFF_STATE_OFF:
	case ONOFF_STATE_TO_OFF:
		rv = -EALREADY;
		break;
	default:
		rv = -EINVAL;
	}

out:
	if (notify) {
		mgr->refs -= 1U;
	}

	state = mgr->flags & ONOFF_STATE_MASK;
	k_spin_unlock(&mgr->lock, key);

	if (stop) {
		__ASSERT_NO_MSG(mgr->transitions->stop != NULL);
		notify_monitors(mgr, state, 0);
		mgr->transitions->stop(mgr, onoff_stop_notify);
	} else if (notify) {
		notify_one(mgr, cli, state, 0);
	}

	return rv;
}

static void onoff_reset_notify(struct onoff_manager *mgr,
			       int res)
{
	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	sys_slist_t clients = mgr->clients;

	/* If the reset failed clear the transition flag but otherwise
	 * leave the state unchanged.
	 *
	 * If it was successful clear the reference count and all
	 * flags except capability flags (sets to ONOFF_STATE_OFF).
	 */
	if (res < 0) {
		mgr->flags &= ~ONOFF_FLAG_TRANSITION;
	} else {
		__ASSERT_NO_MSG(mgr->refs == 0U);
		mgr->refs = 0U;
		mgr->flags &= SERVICE_CONFIG_FLAGS;
	}

	sys_slist_init(&mgr->clients);

	u32_t state = mgr->flags & ONOFF_STATE_MASK;

	k_spin_unlock(&mgr->lock, key);

	notify_all(mgr, &clients, state, res);
}

int onoff_reset(struct onoff_manager *mgr,
		struct onoff_client *cli)
{
	if (mgr->transitions->reset == NULL) {
		return -ENOTSUP;
	}

	bool reset = false;
	int rv = validate_args(mgr, cli);

	if (rv < 0) {
		return rv;
	}

	/* Reject if in a non-thread context and reset could wait. */
	if ((k_is_in_isr() || k_is_pre_kernel())
	    && ((mgr->flags & ONOFF_RESET_SLEEPS) != 0U)) {
		return -EWOULDBLOCK;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	if (get_client_type(cli) != ONOFF_CLIENT_INACTIVE) {
		rv = -EINVAL;
		goto out;
	}

	if ((mgr->flags & ONOFF_HAS_ERROR) == 0) {
		rv = -EALREADY;
		goto out;
	}

	if ((mgr->flags & ONOFF_FLAG_TRANSITION) == 0) {
		reset = true;
		mgr->flags |= ONOFF_FLAG_TRANSITION;
	}

out:
	if (rv >= 0) {
		set_client_type(cli, ONOFF_CLIENT_RESET);
		sys_slist_append(&mgr->clients, &cli->node);
	}

	k_spin_unlock(&mgr->lock, key);

	if (reset) {
		mgr->transitions->reset(mgr, onoff_reset_notify);
	}

	return rv;
}

int onoff_cancel(struct onoff_manager *mgr,
		 struct onoff_client *cli)
{
	int rv = validate_args(mgr, cli);

	if (rv < 0) {
		return rv;
	}

	rv = -EALREADY;
	k_spinlock_key_t key = k_spin_lock(&mgr->lock);
	u32_t state = mgr->flags & ONOFF_STATE_MASK;

	(void)state;
	if (sys_slist_find_and_remove(&mgr->clients, &cli->node)) {
		switch (get_client_type(cli)) {
		case ONOFF_CLIENT_REQUEST:
			/* Requests can be cancelled whether transitioning to on or off.
			 * They're cancelled immediately.
			 */
			__ASSERT_NO_MSG((state == ONOFF_STATE_TO_ON)
					|| (state == ONOFF_STATE_TO_OFF));
			rv = 0;
			break;
		case ONOFF_CLIENT_RELEASE:
			__ASSERT_NO_MSG(state == ONOFF_STATE_TO_OFF);

			/* A release can only be present when
			 * transitioning to off, and when cancelled it
			 * converts to a request that goes back on the
			 * client list to be completed later.
			 */
			set_client_type(cli, ONOFF_CLIENT_REQUEST);
			mgr->refs -= 1;
			rv = 1;
			sys_slist_append(&mgr->clients, &cli->node);
			break;
		case ONOFF_CLIENT_RESET:
			__ASSERT_NO_MSG((state & ONOFF_HAS_ERROR) != 0);
			rv = 0;
			break;
		default:
			break;
		}
	}

	k_spin_unlock(&mgr->lock, key);

	if (rv == 0) {
		set_client_type(cli, ONOFF_CLIENT_INACTIVE);
	}

	return rv;
}

int onoff_monitor_register(struct onoff_manager *mgr,
			   struct onoff_monitor *mon)
{
	if ((mgr == NULL) || (mon == NULL)) {
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

	if ((mgr == NULL) || (mon == NULL)) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&mgr->lock);

	if (sys_slist_find_and_remove(&mgr->monitors, &mon->node)) {
		rv = 0;
	}

	k_spin_unlock(&mgr->lock, key);

	return rv;
}
