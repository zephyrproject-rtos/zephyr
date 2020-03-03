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

#define ST_OFF 0
#define ST_ON ONOFF_INTERNAL_BASE
#define ST_TRANSITION (ONOFF_INTERNAL_BASE << 1)
#define ST_TO_ON (ST_TRANSITION | ST_ON)
#define ST_TO_OFF (ST_TRANSITION | ST_OFF)

#define ST_MASK (ST_ON | ST_TRANSITION)

static void set_service_state(struct onoff_manager *mp,
			      u32_t state)
{
	mp->flags &= ~ST_MASK;
	mp->flags |= (state & ST_MASK);
}

static int validate_args(const struct onoff_manager *mp,
			 struct onoff_client *cli)
{
	if ((mp == NULL) || (cli == NULL)) {
		return -EINVAL;
	}

	int rv = async_notify_validate(&cli->notify);

	if ((rv == 0)
	    && ((cli->notify.flags & ASYNC_NOTIFY_EXTENSION_MASK) != 0)) {
		rv = -EINVAL;
	}

	return rv;
}

int onoff_manager_init(struct onoff_manager *mp,
		       const struct onoff_transitions *transitions)
{
	if (transitions->flags & ~SERVICE_CONFIG_FLAGS) {
		return -EINVAL;
	}

	if ((transitions->start == NULL) || (transitions->stop == NULL)) {
		return -EINVAL;
	}

	*mp = (struct onoff_manager)ONOFF_MANAGER_INITIALIZER(transitions);

	return 0;
}

static void notify_one(struct onoff_manager *mp,
		       struct onoff_client *cli,
		       int res)
{
	onoff_client_callback cb =
		(onoff_client_callback)async_notify_finalize(&cli->notify, res);

	if (cb) {
		cb(mp, cli, cli->user_data, res);
	}
}

static void notify_all(struct onoff_manager *mp,
		       sys_slist_t *list,
		       int res)
{
	while (!sys_slist_is_empty(list)) {
		sys_snode_t *node = sys_slist_get_not_empty(list);
		struct onoff_client *cli =
			CONTAINER_OF(node,
				     struct onoff_client,
				     node);

		notify_one(mp, cli, res);
	}
}

static void onoff_start_notify(struct onoff_manager *mp,
			       int res)
{
	k_spinlock_key_t key = k_spin_lock(&mp->lock);
	sys_slist_t clients = mp->clients;

	/* Can't have a queued releaser during start */
	__ASSERT_NO_MSG(mp->releaser == NULL);

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
		mp->flags &= ~ST_TRANSITION;
		mp->flags |= ONOFF_HAS_ERROR;
	} else {
		sys_snode_t *node;
		unsigned int refs = 0U;

		set_service_state(mp, ST_ON);

		SYS_SLIST_FOR_EACH_NODE(&clients, node) {
			refs += 1U;
		}

		/* Update the reference count, or fail if the count
		 * would overflow.
		 */
		if (mp->refs > (SERVICE_REFS_MAX - refs)) {
			mp->flags |= ONOFF_HAS_ERROR;
		} else {
			mp->refs += refs;
		}
		__ASSERT_NO_MSG(mp->refs > 0U);
	}

	sys_slist_init(&mp->clients);

	k_spin_unlock(&mp->lock, key);

	notify_all(mp, &clients, res);
}

int onoff_request(struct onoff_manager *mp,
		  struct onoff_client *cli)
{
	bool add_client = false;        /* add client to pending list */
	bool start = false;             /* invoke start transition */
	bool notify = false;            /* do client notification */
	int rv = validate_args(mp, cli);

	if (rv < 0) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&mp->lock);

	if ((mp->flags & ONOFF_HAS_ERROR) != 0) {
		rv = -EIO;
		goto out;
	}

	/* Reject if this would overflow the reference count. */
	if (mp->refs == SERVICE_REFS_MAX) {
		rv = -EAGAIN;
		goto out;
	}

	u32_t state = mp->flags & ST_MASK;

	switch (state) {
	case ST_TO_OFF:
		/* Queue to start after release */
		__ASSERT_NO_MSG(mp->releaser != NULL);
		add_client = true;
		rv = 3;
		break;
	case ST_OFF:
		/* Reject if in a non-thread context and start could
		 * wait.
		 */
		if ((k_is_in_isr() || k_is_pre_kernel())
		    && ((mp->flags & ONOFF_START_SLEEPS) != 0U)) {
			rv = -EWOULDBLOCK;
			break;
		}

		/* Start with first request while off */
		__ASSERT_NO_MSG(mp->refs == 0);
		set_service_state(mp, ST_TO_ON);
		start = true;
		add_client = true;
		rv = 2;
		break;
	case ST_TO_ON:
		/* Already starting, just queue it */
		add_client = true;
		rv = 1;
		break;
	case ST_ON:
		/* Just increment the reference count */
		notify = true;
		break;
	default:
		rv = -EINVAL;
		break;
	}

out:
	if (add_client) {
		sys_slist_append(&mp->clients, &cli->node);
	} else if (notify) {
		mp->refs += 1;
	}

	k_spin_unlock(&mp->lock, key);

	if (start) {
		__ASSERT_NO_MSG(mp->transitions->start != NULL);
		mp->transitions->start(mp, onoff_start_notify);
	} else if (notify) {
		notify_one(mp, cli, 0);
	}

	return rv;
}

static void onoff_stop_notify(struct onoff_manager *mp,
			      int res)
{
	bool notify_clients = false;
	int client_res = res;
	bool start = false;
	k_spinlock_key_t key = k_spin_lock(&mp->lock);
	sys_slist_t clients = mp->clients;
	struct onoff_client *releaser = mp->releaser;

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
		mp->flags &= ~ST_TRANSITION;
		mp->flags |= ONOFF_HAS_ERROR;
		notify_clients = true;
	} else if (sys_slist_is_empty(&clients)) {
		set_service_state(mp, ST_OFF);
	} else if ((k_is_in_isr() || k_is_pre_kernel())
		   && ((mp->flags & ONOFF_START_SLEEPS) != 0U)) {
		set_service_state(mp, ST_OFF);
		notify_clients = true;
		client_res = -EWOULDBLOCK;
	} else {
		set_service_state(mp, ST_TO_ON);
		start = true;
	}

	__ASSERT_NO_MSG(releaser);
	mp->refs -= 1U;
	mp->releaser = NULL;
	__ASSERT_NO_MSG(mp->refs == 0);

	/* Remove the clients if there was an error or a delayed start
	 * couldn't be initiated, because we're resolving their
	 * operation with an error.
	 */
	if (notify_clients) {
		sys_slist_init(&mp->clients);
	}

	k_spin_unlock(&mp->lock, key);

	/* Notify the releaser.  If there was an error, notify any
	 * pending requests; otherwise if there are pending requests
	 * start the transition to ON.
	 */
	notify_one(mp, releaser, res);
	if (notify_clients) {
		notify_all(mp, &clients, client_res);
	} else if (start) {
		mp->transitions->start(mp, onoff_start_notify);
	}
}

int onoff_release(struct onoff_manager *mp,
		  struct onoff_client *cli)
{
	bool stop = false;      /* invoke stop transition */
	bool notify = false;    /* do client notification */
	int rv = validate_args(mp, cli);

	if (rv < 0) {
		return rv;
	}

	k_spinlock_key_t key = k_spin_lock(&mp->lock);

	if ((mp->flags & ONOFF_HAS_ERROR) != 0) {
		rv = -EIO;
		goto out;
	}

	u32_t state = mp->flags & ST_MASK;

	switch (state) {
	case ST_ON:
		/* Stay on if release leaves a client. */
		if (mp->refs > 1U) {
			notify = true;
			rv = 1;
			break;
		}

		/* Reject if in non-thread context but stop could
		 * wait
		 */
		if ((k_is_in_isr() || k_is_pre_kernel())
		    && ((mp->flags & ONOFF_STOP_SLEEPS) != 0)) {
			rv = -EWOULDBLOCK;
			break;
		}

		stop = true;

		set_service_state(mp, ST_TO_OFF);
		mp->releaser = cli;
		rv = 2;

		break;
	case ST_TO_ON:
		rv = -EBUSY;
		break;
	case ST_OFF:
	case ST_TO_OFF:
		rv = -EALREADY;
		break;
	default:
		rv = -EINVAL;
	}

out:
	if (notify) {
		mp->refs -= 1U;
	}

	k_spin_unlock(&mp->lock, key);

	if (stop) {
		__ASSERT_NO_MSG(mp->transitions->stop != NULL);
		mp->transitions->stop(mp, onoff_stop_notify);
	} else if (notify) {
		notify_one(mp, cli, 0);
	}

	return rv;
}

static void onoff_reset_notify(struct onoff_manager *mp,
			       int res)
{
	k_spinlock_key_t key = k_spin_lock(&mp->lock);
	sys_slist_t clients = mp->clients;

	/* If the reset failed clear the transition flag but otherwise
	 * leave the state unchanged.
	 *
	 * If it was successful clear the reference count and all
	 * flags except capability flags (sets to ST_OFF).
	 */
	if (res < 0) {
		mp->flags &= ~ST_TRANSITION;
	} else {
		__ASSERT_NO_MSG(mp->refs == 0U);
		mp->refs = 0U;
		mp->flags &= SERVICE_CONFIG_FLAGS;
	}

	sys_slist_init(&mp->clients);

	k_spin_unlock(&mp->lock, key);

	notify_all(mp, &clients, res);
}

int onoff_reset(struct onoff_manager *mp,
		struct onoff_client *cli)
{
	if (mp->transitions->reset == NULL) {
		return -ENOTSUP;
	}

	bool reset = false;
	int rv = validate_args(mp, cli);

	if (rv < 0) {
		return rv;
	}

	/* Reject if in a non-thread context and reset could wait. */
	if ((k_is_in_isr() || k_is_pre_kernel())
	    && ((mp->flags & ONOFF_RESET_SLEEPS) != 0U)) {
		return -EWOULDBLOCK;
	}

	k_spinlock_key_t key = k_spin_lock(&mp->lock);

	if ((mp->flags & ONOFF_HAS_ERROR) == 0) {
		rv = -EALREADY;
		goto out;
	}

	if ((mp->flags & ST_TRANSITION) == 0) {
		reset = true;
		mp->flags |= ST_TRANSITION;
	}

out:
	if (rv >= 0) {
		sys_slist_append(&mp->clients, &cli->node);
	}

	k_spin_unlock(&mp->lock, key);

	if (reset) {
		mp->transitions->reset(mp, onoff_reset_notify);
	}

	return rv;
}

int onoff_cancel(struct onoff_manager *mp,
		 struct onoff_client *cli)
{
	int rv = validate_args(mp, cli);

	if (rv < 0) {
		return rv;
	}

	rv = -EALREADY;
	k_spinlock_key_t key = k_spin_lock(&mp->lock);
	u32_t state = mp->flags & ST_MASK;

	/* Can't remove the last client waiting for the in-progress
	 * transition, as there would be nobody to receive the
	 * completion notification, which might indicate a service
	 * error.
	 */
	if (sys_slist_find_and_remove(&mp->clients, &cli->node)) {
		rv = 0;
		if (sys_slist_is_empty(&mp->clients)
		    && (state != ST_TO_OFF)) {
			rv = -EWOULDBLOCK;
			sys_slist_append(&mp->clients, &cli->node);
		}
	} else if (mp->releaser == cli) {
		/* must be waiting for TO_OFF to complete */
		rv = -EWOULDBLOCK;
	}

	k_spin_unlock(&mp->lock, key);

	if (rv == 0) {
		notify_one(mp, cli, -ECANCELED);
	}

	return rv;
}
