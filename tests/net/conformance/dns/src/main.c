/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the DNS resolver conformance suite.
 *
 * The suite examines the queries the resolver puts on the wire, so this asks
 * for a name over and over rather than once: a suite can then start at any
 * point, collect as many queries as a test case needs, and still finish
 * quickly. One at a time, though, waiting out each lookup before starting the
 * next, which is how an application asks. Whether a query is answered is up to
 * the suite; the resolver does not retransmit, so an unanswered one simply
 * fails here and the next one follows.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/dns.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dns_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>

#define QUERY_NAME	"conformance.test"
/* One lookup at a time, the way an application asks: the next one starts only
 * after the last has been answered or given up on. Overlapping lookups would
 * be a different thing to test, and would keep the resolver from ever being
 * idle between them.
 */
#define QUERY_TIMEOUT	2000
#define QUERY_INTERVAL	K_MSEC(QUERY_TIMEOUT + 500)

static void result_cb(enum dns_resolve_status status, struct dns_addrinfo *info,
		      void *user_data)
{
	ARG_UNUSED(info);
	ARG_UNUSED(user_data);

	if (status == DNS_EAI_INPROGRESS || status == DNS_EAI_ALLDONE) {
		LOG_INF("Query for %s answered", QUERY_NAME);
	}
}

int main(void)
{
	struct net_if *iface = net_if_get_default();
	uint16_t dns_id;
	int ret;

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

	LOG_INF("DNS resolver ready");

	while (true) {
		ret = dns_get_addr_info(QUERY_NAME, DNS_QUERY_TYPE_A, &dns_id,
					result_cb, NULL, QUERY_TIMEOUT);
		if (ret < 0 && ret != -EAGAIN && ret != -ENOSPC) {
			LOG_DBG("Cannot start a query (%d)", ret);
		}

		k_sleep(QUERY_INTERVAL);
	}

	return 0;
}
