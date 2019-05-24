/* Full-stack IoT client example. */

/*
 * Copyright (c) 2018 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>

LOG_MODULE_REGISTER(net_google_iot_mqtt, LOG_LEVEL_INF);

#include <zephyr.h>

#include "dhcp.h"
#include "protocol.h"

#include <net/sntp.h>
#include <net/net_config.h>
#include <net/net_event.h>

/* This comes from newlib. */
#include <time.h>
#include <inttypes.h>

s64_t time_base;

void do_sntp(struct addrinfo *addr)
{
	struct sntp_ctx ctx;
	int rc;
	s64_t stamp;
	struct sntp_time sntp_time;
	char time_str[sizeof("1970-01-01T00:00:00")];

	LOG_INF("Sending NTP request for current time:");

	/* Initialize sntp */
	rc = sntp_init(&ctx, addr->ai_addr, sizeof(struct sockaddr_in));
	if (rc < 0) {
		LOG_ERR("Unable to init sntp context: %d", rc);
		return;
	}

	rc = sntp_query(&ctx, K_FOREVER, &sntp_time);
	if (rc == 0) {
		stamp = k_uptime_get();
		time_base = sntp_time.seconds * MSEC_PER_SEC - stamp;

		/* Convert time to make sure. */
		time_t now = sntp_time.seconds;
		struct tm now_tm;

		gmtime_r(&now, &now_tm);
		strftime(time_str, sizeof(time_str), "%FT%T", &now_tm);
		LOG_INF("  Acquired time: %s", log_strdup(time_str));

	} else {
		LOG_ERR("  Failed to acquire SNTP, code %d\n", rc);
	}

	sntp_close(&ctx);
}

/*
 * TODO: These need to be configurable.
 */
#define MBEDTLS_NETWORK_TIMEOUT 30000

static void show_addrinfo(struct addrinfo *addr)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	void *a;

top:
	LOG_DBG("  flags   : %d", addr->ai_flags);
	LOG_DBG("  family  : %d", addr->ai_family);
	LOG_DBG("  socktype: %d", addr->ai_socktype);
	LOG_DBG("  protocol: %d", addr->ai_protocol);
	LOG_DBG("  addrlen : %d", (int)addr->ai_addrlen);

	/* Assume two words. */
	LOG_DBG("   addr[0]: 0x%lx", ((uint32_t *)addr->ai_addr)[0]);
	LOG_DBG("   addr[1]: 0x%lx", ((uint32_t *)addr->ai_addr)[1]);

	if (addr->ai_next != 0) {
		addr = addr->ai_next;
		goto top;
	}

	a = &net_sin(addr->ai_addr)->sin_addr;

	LOG_INF("  Got %s",
		log_strdup(net_addr_ntop(addr->ai_family, a,
		hr_addr, sizeof(hr_addr))));

}

/*
 * Things that make sense in a demo app that would need to be more
 * robust in a real application:
 *
 * - DHCP happens once.  If it fails, or we change networks, the
 *   network will just stop working.
 *
 */
void main(void)
{
	char time_ip[NET_IPV6_ADDR_LEN];
	static struct addrinfo hints;
	struct addrinfo *haddr;
	int res;
	int cnt = 0;

	LOG_INF("Main entered");

	app_dhcpv4_startup();

	LOG_INF("Should have DHCPv4 lease at this point.");

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = 0;
	while ((res = getaddrinfo("time.google.com", "123", &hints,
				  &haddr)) && cnt < 3) {
		LOG_ERR("Unable to get address for NTP server, retrying");
		cnt++;
	}

	if (res != 0) {
		LOG_ERR("Unable to get address of NTP server, exiting %d", res);
		return;
	}


	LOG_INF("DNS resolved for time.google.com:123");
	time_base = 0;
	inet_ntop(AF_INET, &net_sin(haddr->ai_addr)->sin_addr, time_ip,
			haddr->ai_addrlen);
	show_addrinfo(haddr);
	do_sntp(haddr);
	freeaddrinfo(haddr);

	/* early return if we failed to acquire time */
	if (time_base == 0) {
		LOG_ERR("Failed to get NTP time");
		return;
	}

	mqtt_startup("mqtt.googleapis.com", 8883);
}
