/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mdns_offload_test, LOG_LEVEL_INF);

/* Advertise a DNS-SD service so the host can resolve the responder's
 * hostname (and thus its A/AAAA records) through the offloaded sockets.
 */
DNS_SD_REGISTER_TCP_SERVICE(mdns_offload, CONFIG_NET_HOSTNAME, "_zephyr", "local", DNS_SD_EMPTY_TXT,
			    4242);

int main(void)
{
	/* The offloaded interface comes up and mirrors the host addresses at
	 * boot; the mDNS responder then runs on its own, without application
	 * interaction. Signal readiness to the host-side test harness.
	 */
	LOG_INF("mDNS offload responder ready");

	return 0;
}
