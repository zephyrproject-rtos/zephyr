/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/ztest.h>

static bool attachment_processed(struct net_if *iface)
{
	bool processed;

	net_if_lock(iface);
	processed = iface->attachment_notified && !iface->attachment_pending;
	net_if_unlock(iface);
	return processed;
}

ZTEST(net_context, test_attachment_work_without_event_delivery)
{
	struct net_if *iface = net_if_get_default();

	zassert_not_null(iface);
	zassert_true(net_if_is_up(iface));
	net_if_lock(iface);
	iface->attachment_notified = false;
	net_if_unlock(iface);
	net_if_attachment_changed(iface);
	zassert_true(WAIT_FOR(attachment_processed(iface), 100000, k_msleep(1)));

	/* Reports inside the interval must remain deferred even without events. */
	net_if_attachment_changed(iface);
	k_msleep(50);
	zassert_false(attachment_processed(iface));
	zassert_true(WAIT_FOR(attachment_processed(iface), 1100000, k_msleep(1)));

	/* Unsigned elapsed time must also handle a 32-bit timestamp wrap. */
	net_if_lock(iface);
	iface->attachment_last_notify = k_uptime_get_32() - 1000U;
	net_if_unlock(iface);
	net_if_attachment_changed(iface);
	zassert_true(WAIT_FOR(attachment_processed(iface), 100000, k_msleep(1)));
}
