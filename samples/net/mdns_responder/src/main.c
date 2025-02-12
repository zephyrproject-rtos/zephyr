/* main.c - mDNS responder */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mdns_responder_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/net_core.h>

extern void service(void);

#if defined(CONFIG_NET_VLAN)
int init_vlan(void);
#else
static inline int init_vlan(void)
{
	return 0;
}
#endif /* CONFIG_NET_VLAN */

/*
 * Note that mDNS support requires no application interaction with zephyr,
 * beyond optional runtime hostname configuration calls and setting
 * CONFIG_MDNS_RESPONDER=y.
 *
 * The service() function provides an echo server to make it possible to
 * verify that the IP address resolved by mDNS is the system that this
 * code is running upon.
 */
int main(void)
{
	LOG_INF("Waiting mDNS queries...");
	init_vlan();
	service();
	return 0;
}
