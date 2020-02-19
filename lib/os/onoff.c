/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/onoff.h>


#define SERVICE_CONFIG_FLAGS	     \
	(ONOFF_SERVICE_START_SLEEPS  \
	 | ONOFF_SERVICE_STOP_SLEEPS \
	 | ONOFF_SERVICE_RESET_SLEEPS)

#define SERVICE_REFS_MAX UINT16_MAX

#define SERVICE_STATE_OFF 0
#define SERVICE_STATE_ON ONOFF_SERVICE_INTERNAL_BASE
#define SERVICE_STATE_TRANSITION (ONOFF_SERVICE_INTERNAL_BASE << 1)
#define SERVICE_STATE_TO_ON (SERVICE_STATE_TRANSITION | SERVICE_STATE_ON)
#define SERVICE_STATE_TO_OFF (SERVICE_STATE_TRANSITION | SERVICE_STATE_OFF)

#define SERVICE_STATE_MASK (SERVICE_STATE_ON | SERVICE_STATE_TRANSITION)

static void set_service_state(struct onoff_service *srv,
			      u32_t state)
{
	srv->flags &= ~SERVICE_STATE_MASK;
	srv->flags |= (state & SERVICE_STATE_MASK);
}

static int validate_args(const struct onoff_service *srv,
			 struct onoff_client *cli)
{
	if ((srv == NULL) || (cli == NULL)) {
		return -EINVAL;
	}

	int rv = sys_notify_validate(&cli->notify);

	if ((rv == 0)
	    && ((cli->notify.flags & SYS_NOTIFY_EXTENSION_MASK) != 0)) {
		rv = -EINVAL;
	}

	return rv;
}

int onoff_service_init(struct onoff_service *srv,
		       const struct onoff_service_transitions *transitions)
{
	if (transitions->flags & ~SERVICE_CONFIG_FLAGS) {
		return -EINVAL;
	}

	if ((transitions->start == NULL) || (transitions->stop == NULL)) {
		return -EINVAL;
	}

	*srv = (struct onoff_service)ONOFF_SERVICE_INITIALIZER(transitions);

	return 0;
}

static void notify_one(struct onoff_service *srv,
		       struct onoff_client *cli,
		       int res)
{
	onoff_client_callback cb =
		(onoff_client_callback)sys_notify_finalize(&cli->notify, res);

	if (cb) {
		cb(srv, cli, cli->user_data, res);
	}
}

static void notify_all(struct onoff_service *srv,
		       sys_slist_t *list,
		       int res)
{
	while (!sys_slist_is_empty(list)) {
		sys_snode_t *node = sys_slist_get_not_empty(list);
		struct onoff_client *cli =
			CONTAINER_OF(node,
				     struct onoff_client,
				     node);

		notify_one(srv, cli, res);
	}
}

static void onoff_start_notify(struct onoff_service *srv,
			       int res)
{
	k_spinlock_key_t key = k_spin_lock(&srv->lock);
	sys_slist_t clients = srv->clients;

	/* Can't have a queued releaser during start */
	__ASSERT_NO_MSG(srv->releaser == NULL);

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
		srv->flags &= ~SERVICE_STATE_TRANSITION;
		srv->flags |= ONOFF_SERVICE_HAS_ERROR;
	} else {
		sys_snode_t *node;
		unsigned int refs = 0U;

		set_service_state(srv, SERVICE_STATE_ON);

		SYS_SLIST_FOR_EACH_NODE(&clients, node) {
			refs += 1U;
		}

		/* Update the reference count, or fail if the count
		 * would overflow.
		 */
		if (srv->refs > (SERVICE_REFS_MAX - refs)) {
			srv->flags |= ONOFF_SERVICE_HAS_ERROR;
		} else {
			srv->refs += refs;
		}
		__ASSERT_NO_MSG(srv->refs > 0U);
	}

	sys_slist_init(&srv->clients);

	k_spin_unlock(&srv->lock, key);

	notify_all(srv, &clients, res);
}

int onoff_request(struct onoff_service *srv,
		  struct onoff_client *cli)
{
	bool add_client = false;        /* add client to pending list */
	bool start = false;             /* invoke start transition */
	bool notify = false;            /* do client notification */
	int rv = validate_args(srv, cli);

	if (rv < 0) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&srv->lock);

	if ((srv->flags & ONOFF_SERVICE_HAS_ERROR) != 0) {
		rv = -EIO;
		goto out;
	}

	/* Reject if this would overflow the reference count. */
	if (srv->refs == SERVICE_REFS_MAX) {
		rv = -EAGAIN;
		goto out;
	}

	u32_t state = srv->flags & SERVICE_STATE_MASK;

	switch (state) {
	case SERVICE_STATE_TO_OFF:
		/* Queue to start after release */
		__ASSERT_NO_MSG(srv->releaser != NULL);
		add_client = true;
		rv = 3;
		break;
	case SERVICE_STATE_OFF:
		/* Reject if in a non-thread context and start could
		 * wait.
		 */
		if ((k_is_in_isr() || k_is_pre_kernel())
		    && ((srv->flags & ONOFF_SERVICE_START_SLEEPS) != 0U)) {
			rv = -EWOULDBLOCK;
			break;
		}

		/* Start with first request while off */
		__ASSERT_NO_MSG(srv->refs == 0);
		set_service_state(srv, SERVICE_STATE_TO_ON);
		start = true;
		add_client = true;
		rv = 2;
		break;
	case SERVICE_STATE_TO_ON:
		/* Already starting, just queue it */
		add_client = true;
		rv = 1;
		break;
	case SERVICE_STATE_ON:
		/* Just increment the reference count */
		notify = true;
		break;
	default:
		rv = -EINVAL;
		break;
	}

out:
	if (add_client) {
		sys_slist_append(&srv->clients, &cli->node);
	} else if (notify) {
		srv->refs += 1;
	}

	k_spin_unlock(&srv->lock, key);

	if (start) {
		__ASSERT_NO_MSG(srv->transitions->start != NULL);
		srv->transitions->start(srv, onoff_start_notify);
	} else if (notify) {
		notify_one(srv, cli, 0);
	}

	return rv;
}

static void onoff_stop_notify(struct onoff_service *srv,
			      int res)
{
	bool notify_clients = false;
	int client_res = res;
	bool start = false;
	k_spinlock_key_t key = k_spin_lock(&srv->lock);
	sys_slist_t clients = srv->clients;
	struct onoff_client *releaser = srv->releaser;

	/* If the stop operation failed log an error and leave the
	 * rest of the state in place.
	 *
	 * If it succeeded remove the last reference and transition to
	 * off.
	 *
	 * In either case remove the last reference, and notify all
	 * waiting clients of operation completion.
	 */
	if (res < 0) {
		srv->flags &= ~SERVICE_STATE_TRANSITION;
		srv->flags |= ONOFF_SERVICE_HAS_ERROR;
		notify_clients = true;
	} else if (sys_slist_is_empty(&clients)) {
		set_service_state(srv, SERVICE_STATE_OFF);
	} else if ((k_is_in_isr() || k_is_pre_kernel())
		   && ((srv->flags & ONOFF_SERVICE_START_SLEEPS) != 0U)) {
		set_service_state(srv, SERVICE_STATE_OFF);
		notify_clients = true;
		client_res = -EWOULDBLOCK;
	} else {
		set_service_state(srv, SERVICE_STATE_TO_ON);
		start = true;
	}

	__ASSERT_NO_MSG(releaser);
	srv->refs -= 1U;
	srv->releaser = NULL;
	__ASSERT_NO_MSG(srv->refs == 0);

	/* Remove the clients if there was an error or a delayed start
	 * couldn't be initiated, because we're resolving their
	 * operation with an error.
	 */
	if (notify_clients) {
		sys_slist_init(&srv->clients);
	}

	k_spin_unlock(&srv->lock, key);

	/* Notify the releaser.  If there was an error, notify any
	 * pending requests; otherwise if there are pending requests
	 * start the transition to ON.
	 */
	notify_one(srv, releaser, res);
	if (notify_clients) {
		notify_all(srv, &clients, client_res);
	} else if (start) {
		srv->transitions->start(srv, onoff_start_notify);
	}
}

int onoff_release(struct onoff_service *srv,
		  struct onoff_client *cli)
{
	bool stop = false;      /* invoke stop transition */
	bool notify = false;    /* do client notification */
	int rv = validate_args(srv, cli);

	if (rv < 0) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&srv->lock);

	if ((srv->flags & ONOFF_SERVICE_HAS_ERROR) != 0) {
		rv = -EIO;
		goto out;
	}

	u32_t state = srv->flags & SERVICE_STATE_MASK;

	switch (state) {
	case SERVICE_STATE_ON:
		/* Stay on if release leaves a client. */
		if (srv->refs > 1U) {
			notify = true;
			rv = 1;
			break;
		}

		/* Reject if in non-thread context but stop could
		 * wait
		 */
		if ((k_is_in_isr() || k_is_pre_kernel())
		    && ((srv->flags & ONOFF_SERVICE_STOP_SLEEPS) != 0)) {
			rv = -EWOULDBLOCK;
			break;
		}

		stop = true;

		set_service_state(srv, SERVICE_STATE_TO_OFF);
		srv->releaser = cli;
		rv = 2;

		break;
	case SERVICE_STATE_TO_ON:
		rv = -EBUSY;
		break;
	case SERVICE_STATE_OFF:
	case SERVICE_STATE_TO_OFF:
		rv = -EALREADY;
		break;
	default:
		rv = -EINVAL;
	}

out:
	if (notify) {
		srv->refs -= 1U;
	}

	k_spin_unlock(&srv->lock, key);

	if (stop) {
		__ASSERT_NO_MSG(srv->transitions->stop != NULL);
		srv->transitions->stop(srv, onoff_stop_notify);
	} else if (notify) {
		notify_one(srv, cli, 0);
	}

	return rv;
}

static void onoff_reset_notify(struct onoff_service *srv,
			       int res)
{
	k_spinlock_key_t key = k_spin_lock(&srv->lock);
	sys_slist_t clients = srv->clients;

	/* If the reset failed clear the transition flag but otherwise
	 * leave the state unchanged.
	 *
	 * If it was successful clear the reference count and all
	 * flags except capability flags (sets to SERVICE_STATE_OFF).
	 */
	if (res < 0) {
		srv->flags &= ~SERVICE_STATE_TRANSITION;
	} else {
		__ASSERT_NO_MSG(srv->refs == 0U);
		srv->refs = 0U;
		srv->flags &= SERVICE_CONFIG_FLAGS;
	}

	sys_slist_init(&srv->clients);

	k_spin_unlock(&srv->lock, key);

	notify_all(srv, &clients, res);
}

int onoff_service_reset(struct onoff_service *srv,
			struct onoff_client *cli)
{
	if (srv->transitions->reset == NULL) {
		return -ENOTSUP;
	}

	bool reset = false;
	int rv = validate_args(srv, cli);

	if (rv < 0) {
		return rv;
	}

	/* Reject if in a non-thread context and reset could wait. */
	if ((k_is_in_isr() || k_is_pre_kernel())
	    && ((srv->flags & ONOFF_SERVICE_RESET_SLEEPS) != 0U)) {
		return -EWOULDBLOCK;
	}

	k_spinlock_key_t key = k_spin_lock(&srv->lock);

	if ((srv->flags & ONOFF_SERVICE_HAS_ERROR) == 0) {
		rv = -EALREADY;
		goto out;
	}

	if ((srv->flags & SERVICE_STATE_TRANSITION) == 0) {
		reset = true;
		srv->flags |= SERVICE_STATE_TRANSITION;
	}

out:
	if (rv >= 0) {
		sys_slist_append(&srv->clients, &cli->node);
	}

	k_spin_unlock(&srv->lock, key);

	if (reset) {
		srv->transitions->reset(srv, onoff_reset_notify);
	}

	return rv;
}

int onoff_cancel(struct onoff_service *srv,
		 struct onoff_client *cli)
{
	int rv = validate_args(srv, cli);

	if (rv < 0) {
		return rv;
	}

	rv = -EALREADY;
	k_spinlock_key_t key = k_spin_lock(&srv->lock);
	u32_t state = srv->flags & SERVICE_STATE_MASK;

	/* Can't remove the last client waiting for the in-progress
	 * transition, as there would be nobody to receive the
	 * completion notification, which might indicate a service
	 * error.
	 */
	if (sys_slist_find_and_remove(&srv->clients, &cli->node)) {
		rv = 0;
		if (sys_slist_is_empty(&srv->clients)
		    && (state != SERVICE_STATE_TO_OFF)) {
			rv = -EWOULDBLOCK;
			sys_slist_append(&srv->clients, &cli->node);
		}
	} else if (srv->releaser == cli) {
		/* must be waiting for TO_OFF to complete */
		rv = -EWOULDBLOCK;
	}

	k_spin_unlock(&srv->lock, key);

	if (rv == 0) {
		notify_one(srv, cli, -ECANCELED);
	}

	return rv;
}
