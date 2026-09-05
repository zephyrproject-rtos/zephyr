/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System under test for the SNTP client conformance suite.
 *
 * An ordinary SNTP client: ask the server for the time, set the system clock
 * from what comes back, and ask again a moment later. Nothing here exists for
 * the test. A client that did not set the clock would have had no reason to
 * ask in the first place, and asking once would leave a suite nothing to look
 * at after the first exchange.
 *
 * Setting the clock is also what lets the suite see the part of the client
 * that matters, which is which replies it is willing to believe. sntp_query()
 * stamps every request with the current real time, so the next request after a
 * reply says whether that reply was accepted, and the suite needs no channel
 * into the device to find out.
 *
 * The suite that drives it lives in the net-tools repository, under
 * ttcn3/suites/sntp.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sntp_conformance, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/clock.h>

/* Not port 123. The tester needs no privileges to bind this one, and it cannot
 * collide with a time daemon already running on the host. Has to match
 * tsp_sntp_port in the suite.
 */
#define SERVER_PORT	12123

/* How long one query is given. sntp_simple_addr() retransmits within that,
 * backing off as it goes, so this is the whole query rather than one message.
 */
#define QUERY_TIMEOUT	2000

/* How long to wait before the next query. Short, so that a test case never
 * waits long for a request to answer, but not so short that the queries run
 * into each other.
 */
#define QUERY_INTERVAL	K_MSEC(500)

static void set_clock(const struct sntp_time *ts)
{
	struct timespec tspec = {
		.tv_sec = (time_t)ts->seconds,
		.tv_nsec = (long)(((uint64_t)ts->fraction * NSEC_PER_SEC) >> 32),
	};
	int ret;

	ret = sys_clock_settime(SYS_CLOCK_REALTIME, &tspec);
	if (ret < 0) {
		LOG_ERR("Cannot set the clock (%d)", ret);
		return;
	}

	LOG_INF("Clock set to %llu", ts->seconds);
}

int main(void)
{
	struct net_if *iface = net_if_get_default();
	struct net_sockaddr_in server = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(SERVER_PORT),
	};
	struct sntp_time ts;
	int ret;

	if (iface == NULL) {
		LOG_ERR("No network interface");
		return -ENODEV;
	}

	if (net_addr_pton(NET_AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			  &server.sin_addr) < 0) {
		LOG_ERR("Cannot parse the server address");
		return -EINVAL;
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

	LOG_INF("SNTP client ready");

	while (true) {
		ret = sntp_simple_addr((struct net_sockaddr *)&server,
				       sizeof(server), QUERY_TIMEOUT, &ts);
		if (ret == 0) {
			set_clock(&ts);
		} else {
			LOG_DBG("No time from the server (%d)", ret);
		}

		k_sleep(QUERY_INTERVAL);
	}

	return 0;
}
