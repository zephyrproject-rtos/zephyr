/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the mDNS conformance suite.
 *
 * The responder needs nothing from the application beyond being enabled, so
 * this only reports that it is ready and then stays out of the way.
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/mdns.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdns_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>

int main(void)
{
	struct net_if *iface = net_if_get_default();

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	/* net_config has already brought the interface up and set its
	 * addresses: it waits for both before main() runs.
	 */

	LOG_INF("mDNS responder ready");

	while (true) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
