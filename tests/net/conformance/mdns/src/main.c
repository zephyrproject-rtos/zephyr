/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the mDNS conformance suite.
 *
 * The responder needs nothing from the application beyond being enabled, so
 * this only waits for the interface to come up and then stays out of the way.
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/mdns.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdns_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	/* The addresses are configured during initialisation, so the interface
	 * is usually up before main() runs. Only wait when it is not: waiting
	 * unconditionally would block until the timeout, because the event has
	 * already been and gone.
	 */
	if (!net_if_is_up(iface)) {
		(void)net_mgmt_event_wait_on_iface(iface, NET_EVENT_IF_UP, NULL,
						   NULL, NULL, K_SECONDS(10));
	}

	LOG_INF("mDNS responder ready");

	while (true) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
