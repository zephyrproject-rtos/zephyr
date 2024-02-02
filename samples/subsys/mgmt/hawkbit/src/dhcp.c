/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/kernel.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>

static struct net_mgmt_event_callback mgmt_cb;

/* Semaphore to indicate a lease has been acquired */
static K_SEM_DEFINE(got_address, 0, 1);

static void handler(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event,
		    struct net_if *iface)
{
	bool notified = false;

	if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
		return;
	}

	if (!notified) {
		k_sem_give(&got_address);
		notified = true;
	}
}

/**
 * Start a DHCP client, and wait for a lease to be acquired.
 */
void app_dhcpv4_startup(void)
{
	struct net_if *iface;

	net_mgmt_init_event_callback(&mgmt_cb, handler,
				     NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&mgmt_cb);

	iface = net_if_get_default();

	net_dhcpv4_start(iface);

	/* Wait for a lease */
	k_sem_take(&got_address, K_FOREVER);
}
