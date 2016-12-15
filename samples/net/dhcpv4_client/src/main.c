/* Networking DHCPv4 client */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if 1
#define SYS_LOG_DOMAIN "dhcpv4"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/nbuf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

static struct k_sem quit_lock;

static inline void quit(void)
{
	k_sem_give(&quit_lock);
}

static struct net_mgmt_event_callback mgmt_cb;

static void handler(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event,
		    struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		NET_INFO("NET_EVENT_IPV4_ADDR_ADD");
		quit();
	}
}

static inline void init_app(void)
{
	struct net_if *iface;

	NET_INFO("Run dhcpv4 client");

	iface = net_if_get_default();

	net_dhcpv4_start(iface);

	k_sem_init(&quit_lock, 0, UINT_MAX);

	net_mgmt_init_event_callback(&mgmt_cb, handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);
}

void main_fiber(void)
{
	k_sem_take(&quit_lock, K_FOREVER);
}


void main(void)
{
	NET_INFO("In main");

	init_app();

	main_fiber();
}
