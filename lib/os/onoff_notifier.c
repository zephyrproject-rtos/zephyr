/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/onoff.h>
#include <stdio.h>

#define BUSY_FLAG	BIT(1)
#define IN_RESET_FLAG	BIT(2)
#define IN_ERR_FLAG	BIT(3)

void started_callback(struct onoff_manager *mp,
		      struct onoff_client *cli,
		      void *user_data,
		      int res)
{
	int notres;
	struct onoff_notifier *np =
		CONTAINER_OF(cli, struct onoff_notifier, cli);

	if (res == -ECANCELED) {
		notres = 0;
	} else if (res < 0) {
		notres = res;
		np->state |= IN_ERR_FLAG;
	} else {
		notres = 1;
	}

	np->callback(np, notres);
}

void stopped_callback(struct onoff_manager *mp,
		      struct onoff_client *cli,
		      void *user_data,
		      int res)
{
	int notres;
	struct onoff_notifier *np =
		CONTAINER_OF(cli, struct onoff_notifier, cli);

	if (res == -ECANCELED) {
		notres = 1;
	} else if (res < 0) {
		np->state |= IN_ERR_FLAG;
		notres = res;
	} else {
		notres = 0;
	}

	np->callback(np, notres);
}

void reset_callback(struct onoff_manager *mp,
		      struct onoff_client *cli,
		      void *user_data,
		      int res)
{
	struct onoff_notifier *np =
		CONTAINER_OF(cli, struct onoff_notifier, cli);

	np->state &= ~IN_ERR_FLAG;
	np->state &= ~IN_RESET_FLAG;

	np->callback(np, (res < 0) ? res : 0);
}

int onoff_notifier_request(struct onoff_notifier *np)
{
	int rc;

	if (np->state & IN_RESET_FLAG) {
		return -EWOULDBLOCK;
	}

	if (np->state & IN_ERR_FLAG) {
		return -EIO;
	}
	if (np->state & BUSY_FLAG) {
		return -EALREADY;
	}
	np->state |= BUSY_FLAG;

	rc = onoff_cancel(np->onoff, &np->cli);
	if (rc == 0) {
		return 0;
	}

	onoff_client_init_callback(&np->cli, started_callback, NULL);
	rc = onoff_request(np->onoff, &np->cli);

	return (rc >= 0) ? 0 : rc;
}

int onoff_notifier_release(struct onoff_notifier *np)
{
	int rc;

	if (np->state & IN_RESET_FLAG) {
		return -EWOULDBLOCK;
	}

	if (np->state & IN_ERR_FLAG) {
		return -EIO;
	}

	if (!(np->state & BUSY_FLAG)) {
		return -EALREADY;
	}
	np->state &= ~BUSY_FLAG;

	rc = onoff_cancel(np->onoff, &np->cli);
	if (rc == 0) {
		return 0;
	}

	onoff_client_init_callback(&np->cli, stopped_callback, NULL);
	rc = onoff_release(np->onoff, &np->cli);
	return (rc >= 0) ? 0 : rc;
}

int onoff_notifier_reset(struct onoff_notifier *np)
{
	int rc = -EALREADY;

	if (!(np->state & IN_ERR_FLAG)) {
		return -EALREADY;
	}

	np->state |= IN_RESET_FLAG;

	if (onoff_has_error(np->onoff)) {
		onoff_client_init_callback(&np->cli, reset_callback, NULL);
		rc = onoff_reset(np->onoff, &np->cli);
	}

	if (rc == -EALREADY) {
		np->state &= ~IN_ERR_FLAG;
		np->callback(np, 0);
		rc = 0;
	}

	return rc;
}